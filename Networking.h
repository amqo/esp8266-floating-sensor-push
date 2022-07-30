#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ESPAsyncWiFiManager.h>
#include "secrets.h"

AsyncWebServer server(80);
DNSServer dns;

AsyncWiFiManager wm(&server,&dns);;
HTTPClient http;

bool notificationSent = false;

int wifiSleepDelay = 2 * 60 * 1000; // 2 minutes
unsigned long wifiConnectedTimestamp = 0;

bool shouldSaveWifiConfig = false;

void saveWifiConfigCallback () {
  Serial.println("Should save wifi config");
  shouldSaveWifiConfig = true;
}

void disableWifi() {
  unsigned long nowMillis = millis();
  if (WiFi.status() == WL_CONNECTED && (nowMillis - wifiConnectedTimestamp > wifiSleepDelay)) {
    Serial.println("Disabling Wifi after delay with no alarm");
    wifiConnectedTimestamp = 0;
    WiFi.forceSleepBegin();
    delay(100);
  }
}

bool connectWifi() {
  Serial.println("Called connectWifi");
  bool res = true;
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.forceSleepWake();
    delay(500);
    WiFi.mode(WIFI_STA);
    WiFi.config(staticIP, gateway, subnet);
  
    wm.setSaveConfigCallback(saveWifiConfigCallback);
    wm.setSTAStaticIPConfig(staticIP, gateway, subnet);
    wm.setConfigPortalTimeout(120);
  
    res = wm.autoConnect(autoConnectAP, autoConnectPassword);
    if (!res) {
      Serial.println("Connection Failed! Rebooting...");
      delay(2000);
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

boolean setupOTA() {

  Serial.println("Called setupOTA");

  if (!connectWifi()) {
    return false;
  }
  Serial.println("setupOTA after wifi connection");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP8266.");
  });
  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP server started");
  
  return true;
}

void onAlarmStatus() {
  if (notificationSent) {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting Wifi after turning off for delay");
    if (!connectWifi()) {
      return;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    Serial.println("Sending notification");
    http.begin(client, apiServerUrl);
    int httpCode = http.GET();
    String payload = http.getString();
    Serial.println("Response code: " + String(httpCode) + ", payload: " + payload);
    http.end();
    if (httpCode != -1) {
      wifiConnectedTimestamp = 0;
      notificationSent = true;
      disableWifi();
    }
  }
}

void onNormalStatus() {
  notificationSent = false;
  disableWifi();
}
