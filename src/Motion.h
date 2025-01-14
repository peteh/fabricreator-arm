#pragma once
#include <Arduino.h>
#include "RobotArm.h"

class Motion
{
private:
    bool m_active;
    bool m_initialized;
    bool m_finished;
    float m_motionStartAngles[RobotArm::NUM_JOINTS];
    float m_motionTargetAngles[RobotArm::NUM_JOINTS];
    unsigned long m_motionStartTime = 0l;
    unsigned long m_motionPlannedTime = 0l;
    RobotArm* m_robotArm;

    void initialize();

public:
    Motion(RobotArm *robotArm, const float targetAngles[RobotArm::NUM_JOINTS]);
    bool execute();
    bool isFinished();
};
