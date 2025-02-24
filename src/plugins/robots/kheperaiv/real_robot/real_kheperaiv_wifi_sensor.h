#ifndef REAL_KHEPERAIV_WIFI_SENSOR_H
#define REAL_KHEPERAIV_WIFI_SENSOR_H

#include <argos3/plugins/robots/kheperaiv/control_interface/ci_kheperaiv_wifi_sensor.h>
#include <argos3/plugins/robots/kheperaiv/real_robot/real_kheperaiv_device.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace argos;

class CRealKheperaIVWiFiSensor : public CCI_KheperaIVWiFiSensor,
                                 public CRealKheperaIVDevice {
public:

    CRealKheperaIVWiFiSensor(knet_dev_t* pt_dspic);

    virtual ~CRealKheperaIVWiFiSensor() {}

    virtual void Init(TConfigurationNode& t_node);

    virtual void Destroy();

    virtual void Do(Real f_elapsed_time) {}

    virtual void GetMessages(std::vector<CCI_KheperaIVWiFiSensor::SMessage>& vec_messages);

    virtual void FlushMessages();

protected:

    ssize_t ReceiveDataMultiCast(unsigned char*   pt_buf,
                                 size_t           un_size,
                                 struct sockaddr* pt_sender_addr,
                                 long int         n_timeout);

protected:

    int                m_nMulticastSocket;
    struct sockaddr_in m_tMulticastAddr;
    long int           m_nMulticastTimeout;

    pthread_t       m_tListeningThread;
    pthread_mutex_t m_tListeningMutex;

private:

    void* ListeningThread();

    static void* StartListeningThread(void* pt_context) {
        return reinterpret_cast<CRealKheperaIVWiFiSensor*>(pt_context)->ListeningThread();
    }
};

#endif // REAL_KHEPERAIV_WIFI_SENSOR_H