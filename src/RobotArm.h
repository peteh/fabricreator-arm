#pragma once

class RobotArm
{
public:
  static const int NUM_JOINTS = 6;
  RobotArm(uint8_t servo0Pin, uint8_t servo1Pin, uint8_t servo2Pin, uint8_t servo3Pin, uint8_t servo4Pin, uint8_t servo5Pin);
  
  void begin();
  void setAngle(uint8_t joint, float value);
  float getAngle(uint8_t joint) const;
  float getMaxVelocity(uint8_t joint) const;

  void moveTo(const float targetAngles[NUM_JOINTS]);

  
private:
  uint8_t m_servoPins[NUM_JOINTS];
  float m_servoPosDeg[NUM_JOINTS];

  // max velocities in degrees/second
  float m_maxVelocitiesDegPers[NUM_JOINTS];
  float mapRange(float input, float inMin, float inMax, float outMin, float outMax);
  uint32_t degreesToTicks(float degrees);

  const unsigned int LEDC_TIMER_BIT = 14;      // 14-bit timer resolution
  const unsigned int LEDC_BASE_FREQ = 50;       // 50Hz = 20ms period
  const unsigned int TIMER_MAX = 16383;    // Max value for 14-bit timer (2^14 - 1)
  const unsigned int CHANNEL_OFFSET = 2;
};
