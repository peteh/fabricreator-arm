#include <Arduino.h>
#include <WiFi.h>
#include <mdns.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <esplog.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
// watch dog
#include <esp_task_wdt.h>
#include "RobotArm.h"
#include "apiserver.h"
#include "config.h"
#include "utils.h"

const uint8_t SERVO_0_PIN = 1; // base
const uint8_t SERVO_1_PIN = 1;
const uint8_t SERVO_2_PIN = 1;
const uint8_t SERVO_3_PIN = 1;
const uint8_t SERVO_4_PIN = 1;
const uint8_t SERVO_5_PIN = 1; // claw

const uint WIFI_DISCONNECT_FORCED_RESTART_S = 60;

RobotArm *g_robotArm;
ApiServer *server;
WiFiClient net;
PubSubClient client(net);
bool g_wifiConnected = false;
bool g_mqttConnected = false;
unsigned long g_lastWifiConnect = 0;

String g_bssid = "";

bool connectToWifi()
{
  return WiFi.status() == WL_CONNECTED;
}

bool connectToMqtt()
{
  if (client.connected())
  {
    return true;
  }

  log_info("Connecting to MQTT...");
  if (strlen(MQTT_USER) == 0)
  {
    if (!client.connect(composeClientID().c_str()))
    {
      return false;
    }
  }
  else
  {
    if (!client.connect(composeClientID().c_str(), MQTT_USER, MQTT_PASS))
    {
      return false;
    }
  }
  return true;
}

void setup()
{
  // We allocate two timers for PWM Control
  // TODO: check if and why this is needed from the lib
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  //ESP32PWM::allocateTimer(2);
  //ESP32PWM::allocateTimer(3);
  g_robotArm = new RobotArm(SERVO_0_PIN, SERVO_1_PIN, SERVO_2_PIN, SERVO_3_PIN, SERVO_4_PIN, SERVO_5_PIN);
}

void loop()
{
  bool wifiConnected = connectToWifi();
  if (!wifiConnected)
  {
    if (g_wifiConnected)
    {
      // we switched to disconnected
      // Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
      // generalSettings.wifiDisconnectCounter++;
      // g_settings.setGeneralSettings(generalSettings);
      // g_settings.save();
    }
    if (millis() - g_lastWifiConnect > WIFI_DISCONNECT_FORCED_RESTART_S * 1000)
    {
      log_warn("Wifi could not connect in time, will force a restart");
      ESP.restart();
    }
    g_wifiConnected = false;
    g_mqttConnected = false;
    delay(1000);
    return;
  }
  g_wifiConnected = true;
  g_lastWifiConnect = millis();

  server->handleClient(); // Handling of incoming web requests
  ArduinoOTA.handle();

  bool mqttConnected = connectToMqtt();
  if (!mqttConnected)
  {
    if (g_mqttConnected)
    {
      // we switched to disconnected
      // Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
      // generalSettings.mqttDisconnectCounter++;
      // g_settings.setGeneralSettings(generalSettings);
      // g_settings.save();
    }
    g_mqttConnected = false;
    delay(1000);
    return;
  }
  if (!g_mqttConnected)
  {
    // now we are successfully reconnected and publish our counters
    g_bssid = WiFi.BSSIDstr();
    // g_mqttView.publishDiagnostics(g_settings, g_bssid.c_str());
  }
  g_mqttConnected = true;

  client.loop();

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
