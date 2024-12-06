#include "RobotArm.h"
RobotArm::RobotArm(uint8_t servo0Pin, uint8_t servo1Pin, uint8_t servo2Pin, uint8_t servo3Pin, uint8_t servo4Pin, uint8_t servo5Pin, uint8_t servo6Pin)
{
    m_servoPins[0] = servo0Pin;
    m_servoPins[1] = servo1Pin;
    m_servoPins[2] = servo2Pin;
    m_servoPins[3] = servo3Pin;
    m_servoPins[4] = servo4Pin;
    m_servoPins[5] = servo5Pin;
    m_servoPins[6] = servo6Pin;

    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        m_servoPos[i] = 90;
        m_servos[i] = Servo();
        m_servos[i].attach(m_servoPins[i]);
        m_maxVelocitiesDegPers[i] = 10;
        setAngle(i, m_servoPos[i]);
    }
}

void RobotArm::setAngle(uint8_t joint, uint8_t value)
{
    if (joint < NUM_JOINTS && value <= 180)
    {
        m_servoPos[joint] = value;
        m_servos[joint].write(value);
    }
    // TODO: warn illegal setting
}

uint8_t RobotArm::getAngle(uint8_t joint) const
{
    if (joint < NUM_JOINTS)
    {
        return m_servoPos[joint];
    }
    return 0; // Return 0 for invalid joint index
}

void RobotArm::moveTo(const uint8_t targetAngles[NUM_JOINTS]) 
{
    uint8_t currentAngles[NUM_JOINTS];
    float stepIncrements[NUM_JOINTS];
    uint16_t maxSteps = 0;
    long stepDelayMs = 20;

    // Get current angles and determine the maximum number of steps
    for (uint8_t i = 0; i < NUM_JOINTS; i++) {
        currentAngles[i] = getAngle(i);
        float secondsForThisJoint = (float)abs(targetAngles[i] - currentAngles[i]) / (float)m_maxVelocitiesDegPers[i];
        uint16_t stepsForThisJoint = secondsForThisJoint * 1000 / stepDelayMs;
        maxSteps = max(maxSteps, stepsForThisJoint);
    }

    if (maxSteps == 0)
    {
        // all joints already in position
        return;
    }

    // Calculate step increments for each joint
    for (uint8_t i = 0; i < NUM_JOINTS; i++) {
        int16_t totalMovement = targetAngles[i] - currentAngles[i];
        stepIncrements[i] = (float)totalMovement / maxSteps; // Movement per step
    }

    // Move all joints simultaneously
    for (uint16_t step = 0; step <= maxSteps; step++) {
        bool allReached = true;

        for (uint8_t i = 0; i < NUM_JOINTS; i++) {
            float targetStepAngle = currentAngles[i] + step * stepIncrements[i];
            uint8_t nextAngle = (uint8_t)round(targetStepAngle);

            if (nextAngle != targetAngles[i]) {
                allReached = false;
                this->setAngle(i, nextAngle);
            }
        }

        if (allReached) {
            // TODO: can this really happen? 
            break; // Exit early if all joints have reached their targets
        }

        delay(stepDelayMs); // Control movement timing
    }

    // Ensure all joints reach their final positions
    for (uint8_t i = 0; i < NUM_JOINTS; i++) {
        this->setAngle(i, targetAngles[i]);
    }
}