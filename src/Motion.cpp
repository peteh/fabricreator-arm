#include <esplog.h>
#include "Motion.h"

Motion::Motion(RobotArm *robotArm, const float targetAngles[RobotArm::NUM_JOINTS])
    : m_robotArm(robotArm), m_active(false), m_initialized(false), m_finished(false)
{
    for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
    {
        m_motionStartAngles[i] = 0.f;
        m_motionTargetAngles[i] = targetAngles[i];
    }
}

void Motion::initialize()
{
    long motionTimeMs = 0.;
    // Get current angles and calculate how long motion should take
    for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
    {
        m_motionStartAngles[i] = m_robotArm->getAngle(i);
        long timeForJointMs = (float)abs(m_motionTargetAngles[i] - m_motionStartAngles[i]) / m_robotArm->getMaxVelocity(i) * 1000;

        motionTimeMs = max(motionTimeMs, timeForJointMs);
    }
    m_motionStartTime = millis();
    m_motionPlannedTime = motionTimeMs;
    log_e("Calculated motion time: %d ms", m_motionPlannedTime);
    if (m_motionPlannedTime < 10)
    {
        m_active = false;
        m_finished = true;
        return;
    }
    m_active = true;
    m_initialized = true;
}

bool Motion::isFinished()
{
    return m_finished;
}

bool Motion::execute()
{
    if (!m_initialized)
    {
        this->initialize();
    }

    if (!m_finished)
    {

        // Calculate step increments for each joinm_servoPost
        long currentMillis = millis() - m_motionStartTime;
        float motionPercentage = (float)currentMillis / (float)m_motionPlannedTime;
        if (motionPercentage < 0.f)
        {
            motionPercentage = 0.f;
        }
        if (motionPercentage >= 1.f)
        {
            motionPercentage = 1.f;
            m_active = false;
            // TODO: maybe do this after actually setting the last angle
            m_finished = true;
        }

        for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
        {
            
            float newAngle = m_motionStartAngles[i] + motionPercentage * (m_motionTargetAngles[i] - m_motionStartAngles[i]);
            m_robotArm->setAngle(i, newAngle);
            if (i == 1)
            {
                log_e("Current angle: %f", newAngle);
            }
        }
    }

    return m_finished;
}