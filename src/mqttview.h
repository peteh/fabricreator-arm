#include <Arduino.h>
#include <PubSubClient.h>
#include <MqttDevice.h>
#include "platform.h"
#include "utils.h"
#include "RobotArm.h"
class MqttView
{
public:
    MqttView(PubSubClient *client, RobotArm *robotArm)
        : m_client(client),
          m_device(composeClientID().c_str(), "FABRI Creator Arm", SYSTEM_NAME, "maker_pt"),
          m_robotArm(robotArm)
    {

        m_device.setSWVersion(VERSION);
        for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
        {
            char objectId[9];
            snprintf(objectId, sizeof(objectId), "axis%d", i);
            char humanName[9];
            snprintf(humanName, sizeof(humanName), "Axis %d", i);
            m_joints[i] = new MqttNumber(&m_device, objectId, humanName);

            m_joints[i]->setMin(0);
            m_joints[i]->setMax(180);
        }
    }

    MqttDevice &getDevice()
    {
        return m_device;
    }

    const MqttNumber *getJoint(uint8_t index) const
    {
        return m_joints[index];
    }

    void publishJoint(uint8_t index, float value)
    {
        char state[9];
        snprintf(state, sizeof(state), "%f", value);
        publishMqttState(*m_joints[index], state);
    }

    
    void publishConfig()
    {
        for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
        {
            publishConfig(*m_joints[i]);
        }

        delay(1000);

        for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
        {
            publishJoint(i, m_robotArm->getAngle(i));
        }
    }
private:
    PubSubClient *m_client;

    MqttDevice m_device;
    MqttNumber *m_joints[RobotArm::NUM_JOINTS];
    RobotArm *m_robotArm;

    void publishConfig(MqttEntity &entity)
    {
        String payload = entity.getHomeAssistantConfigPayload();
        char topic[255];
        entity.getHomeAssistantConfigTopic(topic, sizeof(topic));
        if (!m_client->publish(topic, payload.c_str()))
        {
            log_e("Failed to publish config to %s", entity.getStateTopic());
        }
        entity.getHomeAssistantConfigTopicAlt(topic, sizeof(topic));
        if (!m_client->publish(topic, payload.c_str()))
        {
            log_e("Failed to publish config to %s", entity.getStateTopic());
        }
    }

    void publishMqttState(MqttEntity &entity, const char *state)
    {
        if (!m_client->publish(entity.getStateTopic(), state))
        {
            log_e("Failed to publish state to %s", entity.getStateTopic());
        }
    }
};