#pragma once
#include <ESP32Servo.h>

#define NUM_JOINTS 7

class RobotArm
{
public:
  RobotArm(uint8_t servo0Pin, uint8_t servo1Pin, uint8_t servo2Pin, uint8_t servo3Pin, uint8_t servo4Pin, uint8_t servo5Pin, uint8_t servo6Pin);

  void setAngle(uint8_t joint, uint8_t value);
  uint8_t getAngle(uint8_t joint) const;

  void moveTo(const uint8_t targetAngles[NUM_JOINTS]);

private:
  uint8_t m_servoPins[NUM_JOINTS];

  Servo m_servos[NUM_JOINTS];
  uint8_t m_servoPos[NUM_JOINTS];

  // max velocities in degrees/second
  uint8_t m_maxVelocitiesDegPers[NUM_JOINTS];
};
