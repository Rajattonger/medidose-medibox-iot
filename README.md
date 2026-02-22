# MediDose & MediBox – IoT Smart Medication System 💊📡

MediDose & MediBox is an NFC-integrated wearable and IoT-based smart medicine box designed to improve medication adherence and enable real-time health monitoring.

## Overview
The system consists of two integrated components:

- **MediDose** – wearable health monitoring device (bracelet)
- **MediBox** – smart medicine dispenser with reminders

The system monitors vital parameters and provides automated medication alerts via audio, SMS, and display notifications.

## Features
- NFC-based patient & medicine identification
- Pulse rate & SpO₂ monitoring (MAX30100)
- Body temperature sensing
- Scheduled medicine reminders (audio alerts)
- SMS & email notifications
- LCD display interface
- IoT connectivity (ESP8266 / ESP32)

## Hardware Components
- Arduino UNO
- ESP8266 / ESP32
- MAX30100 Pulse Oximeter
- PN532 NFC Module
- DFPlayer Mini + Speaker
- RTC DS3231
- LCD I2C Display
- Temperature Sensor
- Smart Pillbox enclosure

## Project Structure
- `.ino` → Arduino IoT control code  
- `.mp3` → audio reminder files  

## Working
The MediDose wearable monitors patient vitals and transmits data to the MediBox system.  
MediBox schedules medication reminders, provides audio-visual alerts, and sends notifications to users or caregivers.

## Author
**Rajat Kumar**  
B.Tech Information Technology  
IoT & Embedded Systems Project
