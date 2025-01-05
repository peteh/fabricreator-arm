#pragma once
#include "RobotArm.h"

class Motion
{
private:
    bool m_active = false;
    bool m_initialized = false;
    float m_motionStartAngles[RobotArm::NUM_JOINTS];
    float m_motionTargetAngles[RobotArm::NUM_JOINTS];
    long m_motionStartTime = 0;
    long m_motionPlannedTime = 0;
    RobotArm* m_robotArm;

    void initialize();

public:
    Motion(RobotArm *robotArm, const float targetAngles[RobotArm::NUM_JOINTS]);
    bool execute();
};
