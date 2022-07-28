#include "Networking.h"

int FloatSensor = D7;
int led = D6;
int tonePlayer = D8;
int buttonState = 1;

int normalDelay = 10000;
int alarmDelay = 2000;
int accDelay = 0;

// Set how often alarm goes off here
const byte alarmSeconds = 10;
const byte alarmMinutes = 0;
const byte alarmHours = 0;

#if defined (MOTEINO_M0)
  #if defined(SERIAL_PORT_USBVIRTUAL)
    #define Serial SERIAL_PORT_USBVIRTUAL // Required for Serial on Zero based boards
  #endif
#endif

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Booting...");
  
  pinMode(FloatSensor, INPUT_PULLUP);
  pinMode (led, OUTPUT);

  blinkLed();
  ArduinoOTA.setHostname("FloatingAP");
  setupOTA("");

  delay(500);
}

void loop() {
  ArduinoOTA.handle();
  onAwake();
}

void onAwake() {
  buttonState = digitalRead(FloatSensor);

  if (buttonState == LOW) {
    resetNotificationSent();
    blinkLed();
    Serial.println("WATER LEVEL - LOW");
    finalDelay(normalDelay);
  } else {
    digitalWrite(led, HIGH);
    playAlarm();
    Serial.println("WATER LEVEL - HIGH");
    sendNotification();
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
