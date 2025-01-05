#pragma once
#include <ESP32Servo.h>

class RobotArm
{
public:
  static const int NUM_JOINTS = 6;
  RobotArm(uint8_t servo0Pin, uint8_t servo1Pin, uint8_t servo2Pin, uint8_t servo3Pin, uint8_t servo4Pin, uint8_t servo5Pin);

  void setAngle(uint8_t joint, float value);
  float getAngle(uint8_t joint) const;

  void moveTo(const float targetAngles[NUM_JOINTS]);

  
private:
  uint8_t m_servoPins[NUM_JOINTS];

  Servo m_servos[NUM_JOINTS];
  float m_servoPos[NUM_JOINTS];

  // max velocities in degrees/second
  float m_maxVelocitiesDegPers[NUM_JOINTS];
  float mapRange(float input, float inMin, float inMax, float outMin, float outMax);
};
