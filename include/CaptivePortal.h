#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <Arduino.h>

boolean connect_from_file(String filePath, int timeout, int retry);

void setup_portal(void);