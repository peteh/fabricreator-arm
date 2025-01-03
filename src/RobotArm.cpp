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
        m_servoPos[i] = 90;
        m_servos[i] = Servo();
        m_servos[i].attach(m_servoPins[i]);
        m_maxVelocitiesDegPers[i] = 10;
        setAngle(i, m_servoPos[i]);
    }
}

float RobotArm::mapRange(float input, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (input - inMin) * (outMax - outMin) / (inMax - inMin);
}

void RobotArm::setAngle(uint8_t joint, float value)
{
    if (joint >= NUM_JOINTS)
    {
        // TODO: Consider adding error logging or throwing an exception
        return;
    }
    if (value > 180.)
    {
        // Clamp to valid range instead of silently failing
        value = 180.;
    }
    m_servoPos[joint] = value;

    // instead of using .write() we map manually to microseconds as this gives 
    // a bit higher accuracy by having a higher resolution
    int command = this->mapRange(value, 0, 180, DEFAULT_uS_LOW, DEFAULT_uS_HIGH);
    m_servos[joint].writeMicroseconds(command);
    
}

float RobotArm::getAngle(uint8_t joint) const
{
    if (joint < NUM_JOINTS)
    {
        return m_servoPos[joint];
    }
    return 0; // Return 0 for invalid joint index
}

void RobotArm::moveTo(const float targetAngles[NUM_JOINTS])
{
    float stepIncrements[NUM_JOINTS];
    uint16_t maxSteps = 0;
    long stepDelayMs = 20;

    // Get current angles and determine the maximum number of steps
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        float secondsForThisJoint = (float)abs(targetAngles[i] - m_servoPos[i]) / m_maxVelocitiesDegPers[i];
        uint16_t stepsForThisJoint = secondsForThisJoint * 1000 / stepDelayMs;
        maxSteps = max(maxSteps, stepsForThisJoint);
    }

    if (maxSteps == 0)
    {
        // all joints already in position
        return;
    }

    // Calculate step increments for each joint
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        float totalMovement = targetAngles[i] - m_servoPos[i];
        stepIncrements[i] = totalMovement / maxSteps; // Movement per step
    }

    // Move all joints simultaneously
    for (uint16_t step = 0; step <= maxSteps; step++)
    {
        bool allReached = true;

        for (uint8_t i = 0; i < NUM_JOINTS; i++)
        {
            float nextAngle = m_servoPos[i] + step * stepIncrements[i];

            if (nextAngle != targetAngles[i])
            {
                allReached = false;
                this->setAngle(i, nextAngle);
            }
        }

        if (allReached)
        {
            // TODO: can this really happen?
            break; // Exit early if all joints have reached their targets
        }

        delay(stepDelayMs); // Control movement timing
    }

    // Ensure all joints reach their final positions
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        this->setAngle(i, targetAngles[i]);
    }
}