#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include "secrets.h"

#if defined(ESP32_RTOS) && defined(ESP32)
void ota_handle( void * parameter ) {
  for (;;) {
    ArduinoOTA.handle();
    delay(3500);
  }
}
#endif

WiFiManager wm;
HTTPClient http;

bool notificationSent = false;

int wifiSleepDelay = 2 * 60 * 1000; // 2 minutes
unsigned long wifiConnectedTimestamp = 0;

bool shouldSaveWifiConfig = false;

void saveWifiConfigCallback () {
  Serial.println("Should save wifi config");
  shouldSaveWifiConfig = true;
}

bool connectWifi() {
  Serial.println("Called connectWifi");
  bool res = true;
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.forceSleepWake();
    delay(500);
    WiFi.mode(WIFI_STA);
    WiFi.config(staticIP, gateway, subnet);
  
    wm.setClass("invert");
    wm.setSaveConfigCallback(saveWifiConfigCallback);
    wm.setSTAStaticIPConfig(staticIP, gateway, subnet);
    wm.setConfigPortalTimeout(120);
  
    res = wm.autoConnect(autoConnectAP, autoConnectPassword);
    if (!res) {
      Serial.println("Connection Failed! Rebooting...");
      delay(5000);
      ESP.restart();
    } else {  
      Serial.println("Connected to Wifi");
      wifiConnectedTimestamp = millis();
    }
  } else {
    Serial.println("Wifi was already connected");
  }
  return res;
}

boolean setupOTA(const char* nameprefix) {

  Serial.println("Called setupOTA");

  uint16_t maxlen = strlen(nameprefix) + 7;
  char *fullhostname = new char[maxlen];

  uint8_t mac[6];
  WiFi.macAddress(mac);

  snprintf(fullhostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
  ArduinoOTA.setHostname(fullhostname);
  delete[] fullhostname;

  if (!connectWifi()) {
    return false;
  }
  Serial.println("setupOTA after wifi connection");

  ArduinoOTA.setPassword(otaPassword);

  ArduinoOTA.onStart([]() {
	//NOTE: make .detach() here for all functions called by Ticker.h library - not to interrupt transfer process in any way.
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("\nAuth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("\nBegin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("\nConnect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("\nReceive Failed");
    else if (error == OTA_END_ERROR) Serial.println("\nEnd Failed");
  });

  ArduinoOTA.begin();

  Serial.println("OTA Initialized");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  #if defined(ESP32_RTOS) && defined(ESP32)
    xTaskCreate(
      ota_handle,          /* Task function. */
      "OTA_HANDLE",        /* String with name of task. */
      10000,            /* Stack size in bytes. */
      NULL,             /* Parameter passed as input of the task */
      1,                /* Priority of the task. */
      NULL);            /* Task handle. */
  #endif
  return true;
}

void onAlarmStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting Wifi after turning off for delay");
    if (!connectWifi()) {
      return;
    }
  }
  if (WiFi.status() == WL_CONNECTED && !notificationSent) {
    WiFiClient client;
    Serial.println("Sending notification");
    http.begin(client, apiServerUrl);
    int httpCode = http.GET();
    String payload = http.getString();
    Serial.println("Response code: " + String(httpCode) + ", payload: " + payload);
    http.end();
    notificationSent = true;
  }
}

void onNormalStatus() {
  notificationSent = false;
  unsigned long nowMillis = millis();
  if (WiFi.status() == WL_CONNECTED && (nowMillis - wifiConnectedTimestamp > wifiSleepDelay)) {
    Serial.println("Disabling Wifi after delay with no alarm");
    wifiConnectedTimestamp = 0;
    //WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(100);
  }
}
