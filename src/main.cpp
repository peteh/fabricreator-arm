#include <Arduino.h>
#include "RobotArm.h"

const uint8_t SERVO_0_PIN = 1; // base
const uint8_t SERVO_1_PIN = 1;
const uint8_t SERVO_2_PIN = 1;
const uint8_t SERVO_3_PIN = 1;
const uint8_t SERVO_4_PIN = 1;
const uint8_t SERVO_5_PIN = 1;
const uint8_t SERVO_6_PIN = 1; // claw

RobotArm *g_robotArm;

void setup()
{
  g_robotArm = new RobotArm(SERVO_0_PIN, SERVO_1_PIN, SERVO_2_PIN, SERVO_3_PIN, SERVO_4_PIN, SERVO_5_PIN, SERVO_6_PIN);
}

void loop()
{
  if (Serial.available() > 0) // When data is available to read
  {
    String input = Serial.readStringUntil('\n');    // Read a full line
    int servoIndex = input.substring(0, 1).toInt(); // Read the servo index
    int servoValue = input.substring(2).toInt();    // Read servo value

    switch (servoIndex)
    {
    case 1:
      g_robotArm->setAngle(0, servoValue);
      break;
    case 2:
      g_robotArm->setAngle(1, servoValue);
      break;
    case 3:
      g_robotArm->setAngle(2, servoValue);
      break;
    case 4:
      g_robotArm->setAngle(3, servoValue);
      break;
    case 5:
      g_robotArm->setAngle(4, servoValue);
      g_robotArm->setAngle(6, 180 - servoValue);
      break;
    case 6:
      g_robotArm->setAngle(5, servoValue);
      break;
    default:
      // Índice de servo inválido
      break;
    }
  }
}
