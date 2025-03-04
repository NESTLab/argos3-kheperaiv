#include "real_kheperaiv_wifi_sensor.h"

#include <argos3/core/utility/logging/argos_log.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <unistd.h>

/****************************************/
/****************************************/

CRealKheperaIVWiFiSensor::CRealKheperaIVWiFiSensor(knet_dev_t* pt_dspic) :
   CRealKheperaIVDevice(pt_dspic) {
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiSensor::Init(TConfigurationNode& t_node) {
    /* Parse XML configuration for multicast */
    std::string strMulticastAddr;
    GetNodeAttribute(t_node, "multicast_address", strMulticastAddr);
    uint16_t nMulticastPort;
    GetNodeAttribute(t_node, "multicast_port", nMulticastPort);
    GetNodeAttribute(t_node, "multicast_timeout", m_nMulticastTimeout);
    m_nMulticastTimeout *= 1000;
    /* Setup UDP socket */
    m_nMulticastSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_nMulticastSocket < 0) {
        // DEBUG_FUNCTION_EXIT;
        THROW_ARGOSEXCEPTION("socket() in wifi sensor failed: " << strerror(errno));
    }
    memset(&m_tMulticastAddr, 0, sizeof(m_tMulticastAddr));
    m_tMulticastAddr.sin_family      = AF_INET;
    m_tMulticastAddr.sin_port        = htons(nMulticastPort);
    m_tMulticastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    int nRes                         = bind(m_nMulticastSocket,
                                            reinterpret_cast<sockaddr*>(&m_tMulticastAddr),
                                            sizeof(m_tMulticastAddr));
    // DEBUG("Bound to IP address %s\n",
    //       inet_ntoa(m_tMulticastAddr.sin_addr));
    if (nRes < 0) {
        DEBUG_FUNCTION_EXIT;
        THROW_ARGOSEXCEPTION("bind() in wifi sensor failed: " << strerror(errno));
    }
    struct ip_mreq tIPMReq;
    tIPMReq.imr_multiaddr.s_addr = inet_addr(strMulticastAddr.c_str());
    tIPMReq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(m_nMulticastSocket,
                   IPPROTO_IP,
                   IP_ADD_MEMBERSHIP,
                   &tIPMReq,
                   sizeof(tIPMReq)) < 0) {
        // DEBUG_FUNCTION_EXIT;
        THROW_ARGOSEXCEPTION("setsockopt() in wifi sensor failed: " << strerror(errno));
    }
    /* Launch listening thread */
    nRes = pthread_create(&m_tListeningThread,
                          nullptr,
                          &CRealKheperaIVWiFiSensor::StartListeningThread,
                          this);
    if (nRes < 0) {
        // DEBUG_FUNCTION_EXIT;
        THROW_ARGOSEXCEPTION("pthread_create() in wifi sensor failed: " << strerror(errno));
    }
    m_tListeningMutex = PTHREAD_MUTEX_INITIALIZER;
    // DEBUG("Listening thread started\n");
    // DEBUG_FUNCTION_EXIT;
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiSensor::Destroy() {
    // DEBUG_FUNCTION_ENTER;
    /* Close the listening socket
     * This also forced recvfrom() to fail, and then a call to pthread_exit()
     * follows
     */
    close(m_nMulticastSocket);
    /* Wait for listening thread to finish */
    pthread_cancel(m_tListeningThread);
    LOG << "WiFi Listening thread stopped" << std::endl;
    // DEBUG_FUNCTION_EXIT;
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiSensor::GetMessages(std::vector<CCI_KheperaIVWiFiSensor::SMessage>& vec_messages) {
    pthread_mutex_lock(&m_tListeningMutex);
    vec_messages.swap(m_vecMsgQueue);
    m_vecMsgQueue.clear();
    pthread_mutex_unlock(&m_tListeningMutex);
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiSensor::FlushMessages() {
    // DEBUG_FUNCTION_ENTER;
    pthread_mutex_lock(&m_tListeningMutex);
    m_vecMsgQueue.clear();
    pthread_mutex_unlock(&m_tListeningMutex);
    // DEBUG_FUNCTION_EXIT;
}

/****************************************/
/****************************************/

void* CRealKheperaIVWiFiSensor::ListeningThread() {
    // DEBUG_FUNCTION_ENTER;
    // DEBUG("WiFi Listening thread started\n");
    /* Set up blocking listen with a timeout */
    struct timeval sTimeout;
    sTimeout.tv_sec  = 0;
    sTimeout.tv_usec = 100000;
    while (1) {
        struct sockaddr tSenderAddr;
        /* Receive message payload size */
        uint32_t unPayloadSize;
        ssize_t  nRecvd = ReceiveDataMultiCast(
            reinterpret_cast<unsigned char*>(&unPayloadSize),
            sizeof(unPayloadSize),
            &tSenderAddr,
            0);
        if (nRecvd < 0) {
            /* In case of error, it's fine to exit the thread */
            // DEBUG_FUNCTION_EXIT;
            pthread_exit(nullptr);
        } else if (nRecvd > 0) {
            if (unPayloadSize > PAYLOAD_LIMIT)
            {
                LOG << "[INFO] Rejecting corrupted unPayloadSize = " << unPayloadSize << std::endl;
                continue;
            }
            /* Receive message payload */
            unsigned char* punBuffer = new unsigned char[unPayloadSize];
            nRecvd                   = ReceiveDataMultiCast(
                punBuffer,
                unPayloadSize,
                &tSenderAddr,
                m_nMulticastTimeout);
            if (nRecvd > 0) {
                /* Add message to queue */
                pthread_mutex_lock(&m_tListeningMutex);
                m_vecMsgQueue.push_back({inet_ntoa(reinterpret_cast<sockaddr_in*>(&tSenderAddr)->sin_addr),
                                         CByteArray(punBuffer, unPayloadSize)});
                // DEBUG("m_vecMsgQueue.size() = %zu\n", m_vecMsgQueue.size());
                pthread_mutex_unlock(&m_tListeningMutex);
            }
            delete[] punBuffer;
        }
    }
    // DEBUG_FUNCTION_EXIT;
    return nullptr;
}

/****************************************/
/****************************************/

ssize_t CRealKheperaIVWiFiSensor::ReceiveDataMultiCast(unsigned char*   pt_buf,
                                                       size_t           un_size,
                                                       struct sockaddr* pt_sender_addr,
                                                       long int         n_timeout) {
    /* Sender address information */
    socklen_t tSenderAddrLen = sizeof(struct sockaddr);
    /* How much data was received */
    ssize_t nRecvd;
    ssize_t nTotRecvd = 0;
    ssize_t nBufLeft  = un_size;
    /* Receive timeout, 0 means no timeout */
    struct timeval sTimeout;
    sTimeout.tv_sec  = 0;
    sTimeout.tv_usec = n_timeout;
    setsockopt(m_nMulticastSocket,
               SOL_SOCKET,
               SO_RCVTIMEO,
               &sTimeout,
               sizeof(sTimeout));
    do {
        nRecvd = recvfrom(m_nMulticastSocket,
                          pt_buf + nTotRecvd,
                          nBufLeft,
                          0, // flags
                          pt_sender_addr,
                          &tSenderAddrLen);
        if (nRecvd < 0) {
            // DEBUG("recvfrom(): %s\n", strerror(errno));
            return -1;
        } else {
            nTotRecvd += nRecvd;
            nBufLeft -= nRecvd;
        }
    } while (nBufLeft > 0);
    return nTotRecvd;
}

/****************************************/
/****************************************/