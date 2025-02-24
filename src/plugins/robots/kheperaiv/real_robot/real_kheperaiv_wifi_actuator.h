#ifndef REAL_KHEPERAIV_WIFI_ACTUATOR_H
#define REAL_KHEPERAIV_WIFI_ACTUATOR_H

#include <argos3/plugins/robots/kheperaiv/control_interface/ci_kheperaiv_wifi_actuator.h>
#include <argos3/plugins/robots/kheperaiv/control_interface/ci_kheperaiv_wifi_sensor.h>
#include <argos3/plugins/robots/kheperaiv/real_robot/real_kheperaiv_device.h>
#include <netinet/in.h>

using namespace argos;

class CRealKheperaIVWiFiActuator : public CCI_KheperaIVWiFiActuator,
                                   public CRealKheperaIVDevice {
public:

    CRealKheperaIVWiFiActuator(knet_dev_t* pt_dspic);

    virtual ~CRealKheperaIVWiFiActuator() {}

    virtual void Init(TConfigurationNode& t_node);

    virtual void Destroy();

    virtual void Do(Real f_elapsed_time);

    virtual void SendToOne(const std::string& str_addr,
                           const CByteArray&  c_message);

    virtual void SendToMany(const CByteArray& c_message);

    virtual void SendToAll(const CByteArray& c_payload);

protected:

    ssize_t SendDataMultiCast(unsigned char* pt_buf, size_t un_size);

protected:

    int m_nMulticastSocket;

    struct sockaddr_in m_tMulticastAddr;
};

#endif // REAL_KHEPERAIV_WIFI_ACTUATOR_H