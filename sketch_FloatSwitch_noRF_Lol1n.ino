#include "Networking.h"

int FloatSensor = D7;
int led = D6;
int tonePlayer = D8;
int buttonState = 1;

int normalDelay = 10 * 1000;
int alarmDelay = 2 * 1000;
int accDelay = 0;

uint64_t sleepTimeMicroSeconds = 60e6;
bool setupFromSleep = false;

void setup() {
  Serial.begin(115200);
  while(!Serial) continue;

  if (ESP.getResetReason().indexOf("Sleep") > 0) {
    Serial.println("Setup from sleep");
    setupFromSleep = true;
  } else {
    Serial.println("Setup first time"); 
    setupFromSleep = false;
  }
  
  pinMode(FloatSensor, INPUT_PULLUP);
  pinMode (led, OUTPUT);

  blinkLed();
  setupOTA();

  delay(500);
}

void loop() {
  onAwake();
}

void onAwake() {
  buttonState = digitalRead(FloatSensor);

  if (buttonState == LOW) {
    bool doDeepSleep = onNormalStatus() || setupFromSleep;
    blinkLed();
    Serial.println("WATER LEVEL - LOW");
    if (doDeepSleep) {
      ESP.deepSleep(sleepTimeMicroSeconds);
    }
    finalDelay(normalDelay);
  } else {
    digitalWrite(led, HIGH);
    playAlarm();
    Serial.println("WATER LEVEL - HIGH");
    onAlarmStatus();
    finalDelay(alarmDelay);
  }
}

void blinkLed() {
  digitalWrite(led, HIGH);
  addDelay(100);
  digitalWrite(led, LOW);
  addDelay(100);
  digitalWrite(led, HIGH);
  addDelay(100);
  digitalWrite(led, LOW);
  
}

void playAlarm() {
  tone(tonePlayer, 2000, 500);
  addDelay(1000);
  tone(tonePlayer, 2000, 500);
}

void finalDelay(int totalDelay) {
  int pendingDelay = totalDelay - accDelay;
  delay(pendingDelay);
  accDelay = 0;
}

void addDelay(int delayAdded) {
  delay(delayAdded);
  accDelay += delayAdded;
}
