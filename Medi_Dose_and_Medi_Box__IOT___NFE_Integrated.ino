#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

#if (defined(ARDUINO_AVR_UNO) || defined(ESP8266))   // Using a soft serial port
#include <SoftwareSerial.h>
SoftwareSerial softSerial(/*rx =*/2, /*tx =*/3);
#define FPSerial softSerial
#else
#define FPSerial Serial1
#endif

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
//////////////////////////////////////////////
#include <SPI.h>
#include <Adafruit_PN532.h>


#define PN532_SCK   13
#define PN532_MISO  12
#define PN532_MOSI  11
#define PN532_SS    10

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);


const int setButton = A0;
const int hourButton = A1;
const int minButton = A2;
const int nextButton = A3;
const int stopButton = 9;


const int ledPin[3] = {5, 6, 7};

int doseIndex = 0;  // 0: Morning, 1: Afternoon, 2: Night
bool setMode = false;

int doseHour[3] = {8, 14, 20};
int doseMin[3] = {0, 0, 0};

unsigned long lastDisplayTime = 0;
int displaySlotIndex = 0;

bool alarmActive = false;
int activeDoseIndex = -1;
int lastAlarmMinute = -1;  // <-- Add this line

void setup() {
  Wire.begin();
  rtc.begin();
  lcd.init();
  nfc.begin();
  lcd.backlight();
  Serial.begin(9600);
#if (defined ESP32)
  FPSerial.begin(9600, SERIAL_8N1, /*rx =*/D3, /*tx =*/D2);
#else
  FPSerial.begin(9600);
#endif
  pinMode(setButton, INPUT_PULLUP);
  pinMode(hourButton, INPUT_PULLUP);
  pinMode(minButton, INPUT_PULLUP);
  pinMode(nextButton, INPUT_PULLUP);
  pinMode(stopButton, INPUT_PULLUP);

  for (int i = 0; i < 3; i++) {
    pinMode(ledPin[i], OUTPUT);
    digitalWrite(ledPin[i], LOW);
  }
if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) { 
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
 
  
  myDFPlayer.volume(30); 
  lcd.setCursor(0, 0);
  lcd.print("Medicine Reminder");
   uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    while (1); // Halt if no board found
  }
   nfc.SAMConfig();
  delay(2000);
  lcd.clear();
}

void loop() {
  DateTime now = rtc.now();
  PN532();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10) lcd.print("0");
  lcd.prin(now.minute());
  lcd.print("         ");
  if (!setMode && millis() - lastDisplayTime > 2000) {
    lcd.setCursor(0, 1);
    lcd.print(slotName(displaySlotIndex));
    lcd.print(": ");
    if (doseHour[displaySlotIndex] < 10) lcd.print("0");
    lcd.print(doseHour[displaySlotIndex]);
    lcd.print(":");
    if (doseMin[displaySlotIndex] < 10) lcd.print("0");
    lcd.print(doseMin[displaySlotIndex]);
    lcd.print("     ");

    displaySlotIndex = (displaySlotIndex + 1) % 3;
    lastDisplayTime = millis();
  }
  for (int i = 0; i < 3; i++) {
    if (now.hour() == doseHour[i] && now.minute() == doseMin[i] && now.minute() != lastAlarmMinute) {
      ringAlarm(i);
      alarmActive = true;
      activeDoseIndex = i;
      lastAlarmMinute = now.minute(); // <-- only once per minute
    }
  }

  if (digitalRead(setButton) == LOW) {
    lcd.clear();
    setMode = !setMode;
    delay(300);
  }

  
  if (setMode) {
    lcd.setCursor(0, 1);
    lcd.print(slotName(doseIndex));
    lcd.print(": ");
    if (doseHour[doseIndex] < 10) lcd.print("0");
    lcd.print(doseHour[doseIndex]);
    lcd.print(":");
    if (doseMin[doseIndex] < 10) lcd.print("0");
    lcd.print(doseMin[doseIndex]);
    lcd.print("   ");

    if (digitalRead(hourButton) == LOW) {
      lcd.clear();
      doseHour[doseIndex] = (doseHour[doseIndex] + 1) % 24;
      delay(300);
    }

    if (digitalRead(minButton) == LOW) {
      lcd.clear();
      doseMin[doseIndex] = (doseMin[doseIndex] + 1) % 60;
      delay(300);
    }

    if (digitalRead(nextButton) == LOW) {
      lcd.clear();
      doseIndex = (doseIndex + 1) % 3;
      delay(300);
    }
  }
  if (digitalRead(stopButton) == LOW && activeDoseIndex != -1) {
    digitalWrite(ledPin[activeDoseIndex], LOW); 
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.rint("Thank you!");
    myDFPlayer.play(2);
    delay(2000);
    lcd.clear();
    activeDoseIndex = -1;
  }

  delay(100);
}

void ringAlarm(int index) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Take your ");
  lcd.setCursor(0, 1);
  lcd.print(slotName(index));
  lcd.print(" med!");
  digitalWrite(ledPin[index], HIGH);

  if (index == 0) {
    myDFPlayer.play(3); // Morning
    Serial.print("Morning");
    Serial.println("!");
  }
  else if (index == 1) {
    myDFPlayer.play(1); // Afternoon
    Serial.print("Afternoon");
    Serial.println("@");
  }
  else {
    myDFPlayer.play(4); // Night
    Serial.print("Night");
    Serial.println("#");
  }
}



String slotName(int index) {
  if (index == 0) return "Morning";
  else if (index == 1) return "Afternoon";
  else return "Night";
}
void PN532() {
  static unsigned long lastCheck = 0;
  static bool cardPresent = false;

  if (millis() - lastCheck < 500) return; 
  lastCheck = millis();

  uint8_t uid[7];
  uint8_t uidLength;
  nfc.setPassiveActivationRetries(0);  
  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50); // timeout = 50ms

  if (success && !cardPresent) {
    cardPresent = true;

    if (uidLength == 4) {
      uint32_t cardid = uid[0];
      cardid = (cardid << 8) | uid[1];
      cardid = (cardid << 8) | uid[2];
      cardid = (cardid << 8) | uid[3];

     if (cardid == 2207365403) {
        Serial.print("PRATEEK");
        Serial.println("$");
         lcd.clear);
         lcd.setCursor(0, 0);
         lcd.print("****WELCOME****");
         lcd.setCursor(0, 1);
         lcd.print("*** PRATEEK ***");
          myDFPlayer.play(5);  // Sound for Anand
        delay(2000);
      }
    }
  } 
  else if (!success && cardPresent)
  {
    cardPresent = false; 
  }
}
