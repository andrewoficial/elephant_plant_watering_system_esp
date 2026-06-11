#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include <RTClib.h>

#define HC12_RX_PIN   16
#define HC12_TX_PIN   17
#define HC12_SET_PIN  5
#define RAK_RX_PIN    18
#define RAK_TX_PIN    19
#define I2C_SDA       21
#define I2C_SCL       22

extern Adafruit_BMP085 bmp;
extern RTC_DS3231 rtc;

void initPeripherals();
bool connectToWiFi();
void syncRTCWithNTP();
void readLocalSensors(float &tempLocal, float &pressure);
DateTime getRTCNow();

#endif