#pragma once
#include <Arduino.h>

#define VERSION "2024.12.0"
#define SYSTEM_NAME "FABRI Creator ARM 4.0"

#ifdef ARDUINO_LOLIN_S3_MINI
#define SUPPORT_RGB_LED 1
#define RGB_LED_PIN 47
#endif

const char DEFAULT_AP_PASSWORD[] = "doormanadmin";
const long WIFI_CONNECTION_FAIL_TIMEOUT_S = 60;