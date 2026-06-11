#include "peripherals.h"
#include "wifi_config.h"
#include <WiFi.h>
#include <time.h>

Adafruit_BMP085 bmp;
RTC_DS3231 rtc;

// ========== Вспомогательные функции инициализации модулей ==========

void initHC12() {
    Serial.println("\n--- Инициализация HC-12 ---");
    pinMode(HC12_SET_PIN, OUTPUT);
    
    digitalWrite(HC12_SET_PIN, LOW);
    delay(100);
    
    Serial2.end();
    delay(50);
    Serial2.begin(9600, SERIAL_8N1, HC12_RX_PIN, HC12_TX_PIN);
    delay(50);
    
    while(Serial2.available()) Serial2.read();
    
    auto sendAT = [](const char* cmd, unsigned long timeout = 1000) -> String {
        Serial2.print(cmd);
        Serial2.print("\r\n");
        unsigned long start = millis();
        String resp = "";
        while (millis() - start < timeout) {
            while (Serial2.available()) {
                resp += (char)Serial2.read();
            }
        }
        resp.trim();
        return resp;
    };
    
    String ver = sendAT("AT+V");
    Serial.println("HC-12 Version: " + ver);
    String params = sendAT("AT+RX");
    Serial.println("HC-12 Params: " + params);
    
    digitalWrite(HC12_SET_PIN, HIGH);
    delay(100);
    
    String checkResp = sendAT("AT", 500);
    if (checkResp.indexOf("OK") != -1) {
        Serial.println("⚠️ ВНИМАНИЕ: HC-12 остался в AT-режиме!");
        digitalWrite(HC12_SET_PIN, LOW);
        delay(100);
        sendAT("AT");
        digitalWrite(HC12_SET_PIN, HIGH);
        delay(100);
    } else {
        Serial.println("✅ HC-12 успешно перешел в прозрачный режим");
    }
    
    while(Serial2.available()) Serial2.read();
}

void initRAK() {
    Serial.println("\n--- Инициализация RAK3172 ---");
    Serial1.begin(9600, SERIAL_8N1, RAK_RX_PIN, RAK_TX_PIN);
    delay(1500); // RAK загружается дольше
    while(Serial1.available()) Serial1.read();
    
    // Лямбда для отправки AT команд в RAK
    auto sendRAK = [](const char* cmd, unsigned long timeout = 1500) -> String {
        Serial1.print(cmd);
        Serial1.print("\r\n");
        unsigned long start = millis();
        String resp = "";
        while (millis() - start < timeout) {
            while (Serial1.available()) resp += (char)Serial1.read();
        }
        resp.trim();
        return resp;
    };
    
    // 1. Версия прошивки (без знака вопроса)
    String ver = sendRAK("AT+VER=?\r\n");
    Serial.println("RAK Version: " + ver);
    
    // 2. Текущий режим сети (0 = P2P, 1 = LoRaWAN)
    String nwm = sendRAK("AT+NWM=?\r\n");
    Serial.println("RAK Network Mode (0 = P2P_LORA, 1 = LoRaWAN, 2 = P2P_FSK): " + nwm);
    
    // 3. Вывод параметров в зависимости от режима
    if (nwm.indexOf("0") != -1) {
        // Если P2P режим
        String p2p = sendRAK("AT+P2P=?");
        Serial.println("RAK P2P Config: " + p2p);
    } else if (nwm.indexOf("1") != -1) {
        // Если LoRaWAN режим
        String deveui = sendRAK("AT+DEVEUI=?");
        Serial.println("RAK DevEUI: " + deveui);
        String appeui = sendRAK("AT+APPEUI=?");
        Serial.println("RAK AppEUI: " + appeui);
    }
    
    // Очистка буфера от возможных остатков
    while(Serial1.available()) Serial1.read();
}

// ========== Основная инициализация ==========
void initPeripherals() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    
    if (!bmp.begin()) Serial.println("BMP180 не найден!");
    else Serial.println("BMP180 OK");
    
    if (!rtc.begin()) Serial.println("DS3231 не найден!");
    else Serial.println("DS3231 OK");

    initHC12();
    initRAK();
}

// ========== WiFi ==========
bool connectToWiFi() {
    Serial.println("\n=== Подключение к WiFi ===");
    for (int i = 0; i < wifi_networks_count; i++) {
        const char* ssid = wifi_networks[i][0];
        const char* pass = wifi_networks[i][1];
        Serial.printf("Попытка подключения к %s... ", ssid);
        WiFi.begin(ssid, pass);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ Подключено!");
            Serial.print("IP-адрес: ");
            Serial.println(WiFi.localIP());
            return true;
        } else {
            Serial.println(" ❌ не удалось");
        }
    }
    Serial.println("Не удалось подключиться ни к одной сети.");
    return false;
}

// ========== NTP ==========
void syncRTCWithNTP() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    const char* ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 3 * 3600;
    const int daylightOffset_sec = 0;
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
        DateTime dt(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        rtc.adjust(dt);
        Serial.printf("NTP синхронизация: %04d-%02d-%02d %02d:%02d:%02d\n",
                      dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
    } else {
        Serial.println("Не удалось получить время по NTP");
    }
}

// ========== Локальные датчики ==========
void readLocalSensors(float &tempLocal, float &pressure) {
    if (bmp.begin()) {
        tempLocal = bmp.readTemperature();
        pressure = bmp.readPressure() / 100.0F;
    } else {
        tempLocal = -999;
        pressure = -999;
    }
}

DateTime getRTCNow() {
    return rtc.now();
}