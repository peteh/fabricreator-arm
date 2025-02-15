#include <Arduino.h>
#include <WiFi.h>
#include <mdns.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
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
#include "Motion.h"
#include "led_rgb.h"

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

Motion *g_motion = nullptr;
uint8_t g_motionNum = 0;

//LedRGB *g_led = new LedRGB(RGB_LED_PIN);


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

  log_i("Connecting to MQTT...");
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
  log_i("Message arrived [%s]", topic);
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

  delay(400);
  log_i("Booting....");
  log_i("Starting to mount LittleFS");
  if (!LittleFS.begin())
  {
    log_e("Failed to mount file system");
    delay(5000);
    if (!formatLittleFS())
    {
      log_e("Failed to format file system - hardware issues!");
      for (;;)
      {
        delay(100);
      }
    }
  }
  log_i("Finished Mounting");
  WiFi.setHostname(composeClientID().c_str());
  WiFi.mode(WIFI_STA);
#ifdef ESP32
  // select the AP with the strongest signal
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
#endif

  //g_led->begin();
  //g_led->setBackgroundLight(true);
  //g_led->update();

  

  // TODO: boot in ap mode if apnextboot is set
  // WiFi.begin(g_settings.getWiFiSettings().staSsid, g_settings.getWiFiSettings().staPassword);
  WiFi.begin(DEFAULT_STA_WIFI_SSID, DEFAULT_STA_WIFI_PASS);

  log_i("Connecting to wifi...");
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
    log_d(".");
    delay(500);
  }
  g_wifiConnected = true;
  //g_led->setBackgroundLight(false);
  g_lastWifiConnect = millis();

  log_i("Wifi connected!");
  log_i("IP address: %s", WiFi.localIP().toString().c_str());
  g_bssid = WiFi.BSSIDstr();

  g_robotArm = new RobotArm(SERVO_0_PIN, SERVO_1_PIN, SERVO_2_PIN, SERVO_3_PIN, SERVO_4_PIN, SERVO_5_PIN);
  g_robotArm->begin();
  g_server = new ApiServer(g_robotArm);
  g_mqttView = new MqttView(&client, g_robotArm);
  g_server->begin();
  //g_led->update();

  // MQTT initialization
  char configUrl[256];
  snprintf(configUrl, sizeof(configUrl), "http://%s/", WiFi.localIP().toString().c_str());
  g_mqttView->getDevice().setConfigurationUrl(configUrl);

  client.setBufferSize(1024);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  log_i("Boot done");
}

void loop()
{
  //g_led->update();
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
      log_w("Wifi could not connect in time, will force a restart");
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
  

  if (g_motion == nullptr || g_motion->isFinished())
  {
    // Clean up previous motion if it exists
    if (g_motion != nullptr)
    {
      //delete g_motion;
      g_motion = nullptr;
    }
    if (g_motionNum == 0)
    {
      float params[] = {180.f, 180.f, 180.f, 180.f, 180.f, 180.f};

      g_motion = new Motion(g_robotArm, params);
    }
    else if (g_motionNum == 1)
    {
      float params[] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
      g_motion = new Motion(g_robotArm, params);
    }
    else if (g_motionNum == 2)
    {
      float params[] = {90.f, 90.f, 90.f, 90.f, 90.f, 90.f};
      g_motion = new Motion(g_robotArm, params);
    }

    g_motionNum++;
    if (g_motionNum > 2)
    {
      g_motion = 0;
    }
  }
  // Only execute if motion exists
  if (g_motion != nullptr)
  {
    long start = millis();
    g_motion->execute();
    long time = millis()-start;
    log_e("Everything else time: %d", time);
  }
  delay(20);
}
