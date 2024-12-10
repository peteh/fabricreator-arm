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
#include "platform.h"
#include "RobotArm.h"
#include "apiserver.h"
#include "config.h"
#include "mqttview.h"
#include "utils.h"

const uint8_t SERVO_0_PIN = 2;  // base
const uint8_t SERVO_1_PIN = 3;  // lower arm low gear
const uint8_t SERVO_2_PIN = 4;  // lower arm upper gear
const uint8_t SERVO_3_PIN = 5;  // claw turn
const uint8_t SERVO_4_PIN = 16; // claw pitch
const uint8_t SERVO_5_PIN = 17; // claw

const uint WIFI_DISCONNECT_FORCED_RESTART_S = 60;

RobotArm *g_robotArm;
MqttView *g_mqttView;
ApiServer *g_server;
WiFiClient net;
PubSubClient client(net);
bool g_wifiConnected = false;
bool g_mqttConnected = false;
unsigned long g_lastWifiConnect = 0;

String g_bssid = "";
const char *HOMEASSISTANT_STATUS_TOPIC = "homeassistant/status";
const char *HOMEASSISTANT_STATUS_TOPIC_ALT = "ha/status";

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

  for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
  {
    client.subscribe(g_mqttView->getJoint(i)->getCommandTopic(), 1);
  }

  g_mqttView->publishConfig();

  return true;
}

float parseValue(const char *data, unsigned int length)
{
  // TODO length check
  char temp[32];
  strncpy(temp, data, length);
  return strtof(temp, NULL);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  log_info("Message arrived [%s]", topic);
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  for (uint8_t i = 0; i < RobotArm::NUM_JOINTS; i++)
  {
    if (strcmp(topic, g_mqttView->getJoint(i)->getCommandTopic()) == 0)
    {
      float data = parseValue((char *)payload, length);
      g_robotArm->setAngle(i, data);
      break;
    }
  }

  // publish config when homeassistant comes online and needs the configuration again
  if (strcmp(topic, HOMEASSISTANT_STATUS_TOPIC) == 0 ||
      strcmp(topic, HOMEASSISTANT_STATUS_TOPIC_ALT) == 0)
  {
    if (strncmp((char *)payload, "online", length) == 0)
    {
      g_mqttView->publishConfig();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  // We allocate two timers for PWM Control
  // TODO: check if and why this is needed from the lib
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  // ESP32PWM::allocateTimer(3);

  delay(400);
  log_info("Starting to mount LittleFS");
  if (!LittleFS.begin())
  {
    log_error("Failed to mount file system");
    delay(5000);
    if (!formatLittleFS())
    {
      log_error("Failed to format file system - hardware issues!");
      for (;;)
      {
        delay(100);
      }
    }
  }
  log_info("Finished Mounting");
  WiFi.setHostname(composeClientID().c_str());
  WiFi.mode(WIFI_STA);
#ifdef ESP32
  // select the AP with the strongest signal
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
#endif

  // TODO: boot in ap mode if apnextboot is set
  // WiFi.begin(g_settings.getWiFiSettings().staSsid, g_settings.getWiFiSettings().staPassword);
  WiFi.begin(DEFAULT_STA_WIFI_SSID, DEFAULT_STA_WIFI_PASS);

  log_info("Connecting to wifi...");
  // TODO: really forever? What if we want to go back to autoconnect?
  long startTime = millis();
  while (!connectToWifi())
  {
    if (millis() - startTime > WIFI_CONNECTION_FAIL_TIMEOUT_S * 1000)
    {
      // we failed to connect to the wifi, force reboot in AP settings
      // Settings::GeneralSettings settings = g_settings.getGeneralSettings();
      // settings.forceAPnextBoot = true;
      // g_settings.setGeneralSettings(settings);
      // g_settings.save();
      ESP.restart();
    }
    log_debug(".");
    delay(500);
  }
  g_wifiConnected = true;
  g_lastWifiConnect = millis();

  log_info("Wifi connected!");
  log_info("IP address: %s", WiFi.localIP().toString().c_str());
  g_bssid = WiFi.BSSIDstr();


  g_robotArm = new RobotArm(SERVO_0_PIN, SERVO_1_PIN, SERVO_2_PIN, SERVO_3_PIN, SERVO_4_PIN, SERVO_5_PIN);
  g_server = new ApiServer(g_robotArm);
  g_mqttView = new MqttView(&client, g_robotArm);
  g_server->begin();

  // MQTT initialization
  char configUrl[256];
  snprintf(configUrl, sizeof(configUrl), "http://%s/", WiFi.localIP().toString().c_str());
  g_mqttView->getDevice().setConfigurationUrl(configUrl);

  client.setBufferSize(1024);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  log_info("Boot done");
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

  g_server->handleClient(); // Handling of incoming web requests
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
  delay(1);
}
