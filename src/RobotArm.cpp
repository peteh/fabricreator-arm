#include <esp32-hal-ledc.h>
#include <driver/ledc.h>
#include <Arduino.h>
#include "RobotArm.h"
RobotArm::RobotArm(uint8_t servo0Pin, uint8_t servo1Pin, uint8_t servo2Pin, uint8_t servo3Pin, uint8_t servo4Pin, uint8_t servo5Pin)
{
    m_servoPins[0] = servo0Pin;
    m_servoPins[1] = servo1Pin;
    m_servoPins[2] = servo2Pin;
    m_servoPins[3] = servo3Pin;
    m_servoPins[4] = servo4Pin;
    m_servoPins[5] = servo5Pin;

    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        m_servoPosDeg[i] = 90;
        m_maxVelocitiesDegPers[i] = 10;
    }
}

void RobotArm::begin()
{
    // Initialize fade service
    ledc_fade_func_install(0);

    // Configure LEDC timer
    for (int i = 0; i < NUM_JOINTS; i++)
    {
        ledcSetup(CHANNEL_OFFSET + i, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
        ledcAttachPin(m_servoPins[i], CHANNEL_OFFSET + i);
    }
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        setAngle(i, m_servoPosDeg[i]);
    }
}

// Convert degrees to timer ticks
uint32_t RobotArm::degreesToTicks(float degrees)
{
    degrees = constrain(degrees, 0.0, 180.0);
    float us = map(degrees * 100, 0, 18000, 1000 * 100, 2000 * 100) / 100.0;
    return map(us, 0, 20000, 0, TIMER_MAX);
}

void RobotArm::setAngle(uint8_t joint, float degrees)
{
    if (joint >= NUM_JOINTS)
    {
        // TODO: Consider adding error logging or throwing an exception
        return;
    }

    // TODO: clamping
    m_servoPosDeg[joint] = degrees;
    uint32_t targetTicks = degreesToTicks(degrees);
    uint32_t fadeTimeMs = 20;
    // ledcWrite(LEDC_CHANNEL, targetTicks);
    ledc_set_fade_time_and_start(ledc_mode_t::LEDC_LOW_SPEED_MODE, (ledc_channel_t)(CHANNEL_OFFSET + joint),
                                 targetTicks, fadeTimeMs, ledc_fade_mode_t::LEDC_FADE_NO_WAIT);
}

float RobotArm::getAngle(uint8_t joint) const
{
    if (joint < NUM_JOINTS)
    {
        return m_servoPosDeg[joint];
    }
    return 0; // Return 0 for invalid joint index
}

float RobotArm::getMaxVelocity(uint8_t joint) const
{
    if (joint < NUM_JOINTS)
    {
        return m_maxVelocitiesDegPers[joint];
    }
    return 0; // Return 0 for invalid joint index
}
