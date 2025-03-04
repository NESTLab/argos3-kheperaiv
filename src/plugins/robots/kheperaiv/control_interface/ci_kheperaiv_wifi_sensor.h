#ifndef CI_KHEPERAIV_WIFI_SENSOR_H
#define CI_KHEPERAIV_WIFI_SENSOR_H

/* To avoid dependency problems when including */
namespace argos {
    class CCI_KheperaIVWiFiSensor;
}

#include <argos3/core/control_interface/ci_sensor.h>
#include <argos3/core/utility/datatypes/byte_array.h>

namespace argos {

    class CCI_KheperaIVWiFiSensor : public CCI_Sensor {
    public:

        struct SMessage {
            std::string Address;
            CByteArray  Payload;
        };

    public:

        virtual ~CCI_KheperaIVWiFiSensor() {}

        /**
         * Fills the given message vector and flushes the internal message queue
         */
        virtual void GetMessages(std::vector<SMessage>& vec_messages) = 0;

        /**
         * Discards all the messages in the queue.
         */
        virtual void FlushMessages() = 0;

    protected:

        std::vector<SMessage> m_vecMsgQueue;

#ifdef ARGOS_WITH_LUA
        virtual void CreateLuaState(lua_State* pt_lua_state);

        virtual void ReadingsToLuaState(lua_State* pt_lua_state);
#endif
    };
}

#endif // CI_KHEPERAIV_WIFI_SENSOR_H