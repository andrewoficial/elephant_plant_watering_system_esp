#include <Arduino.h>
#include "peripherals.h"
#include "device_manager.h"
#include "web_server.h"
#include "protocol.h"
#include <Preferences.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

     //РАСКОММЕНТИРОВАТЬ ДЛЯ ОЧИСТКИ ПАМЯТИ ОДИН РАЗ!
     //Preferences prefs; prefs.begin("devices", false); prefs.clear(); prefs.end();
     //Serial.println("Memory cleared!"); 

    Serial.println("\n=== ЗАПУСК МЕТЕОСТАНЦИИ (ADVRT PROTOCOL) ===");
    
    initPeripherals();
    
    if (connectToWiFi()) {
        syncRTCWithNTP();
    } else {
        Serial.println("Работа без WiFi, время не синхронизировано");
    }
    
    loadDevicesFromPrefs();
    setupWebServer();
    
    Serial.println("\n=== СИСТЕМА ГОТОВА ===\n");
    
    // Сразу отправляем POOL при старте, чтобы найти живые устройства
    sendPacketToRadio("00", "POOL");
    Serial.println("[POLL] Стартовый POOL отправлен");
}

void loop() {
    server.handleClient();
    
    // Обработка входящих данных через кольцевой буфер и CRC
    processRadioBuffer();
    
    // Чтение из RAK
    while (Serial1.available()) {
        String msg = Serial1.readString();
        Serial.print("[RAK] ");
        Serial.println(msg);
    }
    
    // Обновление среднесуточной температуры (раз в минуту)
    static unsigned long lastTempUpdate = 0;
    if (millis() - lastTempUpdate > 60000) {
        lastTempUpdate = millis();
        updateDailyAverages();
    }
    
    // Проверка расписания полива (раз в минуту)
    static unsigned long lastScheduleCheck = 0;
    if (millis() - lastScheduleCheck > 60000) {
        lastScheduleCheck = millis();
        checkSchedules();
    }
    
    static unsigned long lastCleanup = 0;
    if (millis() - lastCleanup > 30000) {
        lastCleanup = millis();
        cleanupUnverifiedDevices();
    }
    static unsigned long lastDiskFlush = 0;
    if (millis() - lastDiskFlush > 600000) { // 600 000 мс = 10 минут
        lastDiskFlush = millis();
        flushDevicesToDisk();
    }
    // Статус в сериал (раз в 30 сек)
    static unsigned long lastSerialOut = 0;
    if (millis() - lastSerialOut > 30000) {
        lastSerialOut = millis();
        float t, p;
        readLocalSensors(t, p);
        Serial.printf("Локально: %.1f°C, %.1f гПа | Устройств: %d\n", t, p, deviceCount);
    }

    // Широковещательный опрос POOL (раз в 30 секунд)
    static unsigned long lastPoolTime = 0;
    if (millis() - lastPoolTime > 10000) {
        lastPoolTime = millis();
        sendPacketToRadio("00", "POOL");
        //Serial.println("[POLL] Отправлен POOL"); // Раскомментировать для отладки
    }

    // Последовательный опрос известных устройств на ADVRT (раз в 2 секунды)
    static unsigned long lastPollTime = 0;
    if (millis() - lastPollTime > 15000) {
        lastPollTime = millis();
        pollNextDevice();
    }
    
    delay(10);
}