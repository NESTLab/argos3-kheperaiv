#include "real_kheperaiv_wifi_actuator.h"

#include <argos3/core/utility/logging/argos_log.h>
#include <arpa/inet.h>

#include <cstdio>

/****************************************/
/****************************************/

void CRealKheperaIVWiFiActuator::Init(TConfigurationNode& t_node) {
    /* Parse XML configuration for multicast */
    std::string strMulticastAddr;
    GetNodeAttribute(t_node, "multicast_address", strMulticastAddr);
    uint16_t nMulticastPort;
    GetNodeAttribute(t_node, "multicast_port", nMulticastPort);
    /* Create socket for sending */
    m_nMulticastSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_nMulticastSocket < 0) {
        THROW_ARGOSEXCEPTION("socket() in wifi actuator failed:" << strerror(errno));
    }
    /* Set socket time-to-live */
    int nMulticastTTL = 1;
    if (setsockopt(m_nMulticastSocket, IPPROTO_IP, IP_MULTICAST_TTL, &nMulticastTTL, sizeof(nMulticastTTL)) < 0) {
        THROW_ARGOSEXCEPTION("setsockopt() in wifi actuator failed:" << strerror(errno));
    }
    memset(&m_tMulticastAddr, 0, sizeof(m_tMulticastAddr));
    m_tMulticastAddr.sin_family      = AF_INET;
    m_tMulticastAddr.sin_addr.s_addr = inet_addr(strMulticastAddr.c_str());
    m_tMulticastAddr.sin_port        = htons(nMulticastPort);
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiActuator::Destroy() {
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiActuator::Do(Real f_elapsed_time) {
    for (std::vector<CCI_KheperaIVWiFiSensor::SMessage>::iterator itM = m_vecMsgQueue.begin();
         itM != m_vecMsgQueue.end();
         ++itM) {
        /* First, send message size */
        uint32_t unToSend = itM->Payload.Size();
        SendDataMultiCast(reinterpret_cast<unsigned char*>(&unToSend), sizeof(unToSend));
        /* Second, send message payload */
        SendDataMultiCast(itM->Payload.ToCArray(), unToSend);
    }
    m_vecMsgQueue.clear();
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiActuator::SendToOne(const std::string& str_addr, const CByteArray& c_message) {
    THROW_ARGOSEXCEPTION("CRealKheperaIVWiFiActuator::SendToOne not implemented!");
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiActuator::SendToMany(const CByteArray& c_message) {
    m_vecMsgQueue.push_back({"", c_message});
}

/****************************************/
/****************************************/

void CRealKheperaIVWiFiActuator::SendToAll(const CByteArray& c_payload) {
    THROW_ARGOSEXCEPTION("CRealKheperaIVWiFiActuator::SendToAll not implemented!");
}

/****************************************/
/****************************************/

ssize_t CRealKheperaIVWiFiActuator::SendDataMultiCast(unsigned char* pt_buf, size_t un_size) {
    /* Send message payload */
    ssize_t nToSend  = un_size;
    ssize_t nTotSent = 0;
    ssize_t nSent;
    while (nToSend > 0) {
        nSent = sendto(m_nMulticastSocket,
                       pt_buf + nTotSent,
                       nToSend,
                       0,
                       reinterpret_cast<sockaddr*>(&m_tMulticastAddr),
                       sizeof(m_tMulticastAddr));
        if (nSent < 0) {
            LOGERR << "sendto() in wifi actuator failed" << strerror(errno) << std::endl;
            return -1;
        } else {
            nTotSent += nSent;
            nToSend -= nSent;
        }
    }
    return nTotSent;
}

/****************************************/
/****************************************/
