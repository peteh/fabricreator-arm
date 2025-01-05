#include "Motion.h"

Motion::Motion(RobotArm *robotArm, const float targetAngles[RobotArm::NUM_JOINTS])
    : m_robotArm(robotArm)
{
    for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
    {
        m_motionStartAngles[i] = 0;
        m_motionTargetAngles[i] = targetAngles[i];
    }
}

void Motion::initialize()
{
    long stepDelayMs = 20;
    long motionTimeMs = 0.;
    // Get current angles and calculate how long motion should take
    for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
    {
        m_motionStartAngles[i] = m_robotArm->getAngle(i);
        long timeForJointMs = (float)abs(m_motionTargetAngles[i] - m_motionStartAngles[i]) / m_robotArm->getMaxVelocity(i) * 1000;
        motionTimeMs = max(motionTimeMs, timeForJointMs);
    }
    m_motionStartTime = millis();
    m_active = true;
}

bool Motion::execute()
{
    if (m_motionStartTime == 0)
    {
        this->initialize();
    }

    // Calculate step increments for each joinm_servoPost
    long currentMillis = millis() - m_motionStartTime;
    float motionPercentage = (float)currentMillis / m_motionPlannedTime;
    if (motionPercentage < 0)
    {
        motionPercentage = 0.;
    }
    if (motionPercentage > 1.)
    {
        motionPercentage = 1.;
        m_active = false;
    }
    
    for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
    {

        float newAngle = m_motionStartAngles[i] + motionPercentage * (m_motionTargetAngles[i] - m_motionStartAngles[i]);
        m_robotArm->setAngle(i, newAngle);
    }
}