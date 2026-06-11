#include "device_manager.h"
#include "peripherals.h"
#include "protocol.h"
#include <Preferences.h>

Preferences prefs;
DeviceConfig devices[MAX_DEVICES];
DeviceReadings deviceReadings[MAX_DEVICES];
int deviceCount = 0;
static int currentPollIdx = 0;

void loadDevicesFromPrefs() {
    prefs.begin("devices", false);
    deviceCount = prefs.getInt("count", 0);
    if (deviceCount > MAX_DEVICES) deviceCount = MAX_DEVICES;
    for (int i = 0; i < deviceCount; i++) {
        String key = "dev" + String(i);
        devices[i].addr = prefs.getString((key + "_addr").c_str(), "");
        devices[i].name = prefs.getString((key + "_name").c_str(), "");
        devices[i].interface = prefs.getString((key + "_if").c_str(), "HC12");
        devices[i].lastWateringSec = prefs.getULong((key + "_lastWat").c_str(), 0);
        devices[i].dailyTempSum = prefs.getFloat((key + "_tempSum").c_str(), 0);
        devices[i].dailyTempCount = prefs.getInt((key + "_tempCnt").c_str(), 0);
        devices[i].lastDayOfYear = prefs.getInt((key + "_lastDay").c_str(), getDayOfYear());
        devices[i].needsSave = false;
        
        String sKey = key + "_sc";
        devices[i].scheduleCount = prefs.getInt((sKey + "Cnt").c_str(), 0);
        if (devices[i].scheduleCount > MAX_SCHEDULES) devices[i].scheduleCount = MAX_SCHEDULES;
        for (int j = 0; j < devices[i].scheduleCount; j++) {
            String ruleKey = sKey + String(j);
            devices[i].schedules[j].intervalMinutes = prefs.getFloat((ruleKey + "Intv").c_str(), 1440.0);
            devices[i].schedules[j].tempThreshold = prefs.getFloat((ruleKey + "Thr").c_str(), 25.0);
            devices[i].schedules[j].tempCondition = prefs.getInt((ruleKey + "Cond").c_str(), 0);
            devices[i].schedules[j].targetHour = prefs.getInt((ruleKey + "Hr").c_str(), -1);
        }
        
        deviceReadings[i].tempAir = -999; deviceReadings[i].humAir = -999; deviceReadings[i].pressure = -999; deviceReadings[i].humSoil = -999;
        deviceReadings[i].pumpOn = false; deviceReadings[i].lastSeenMillis = 0; deviceReadings[i].isVerified = true; deviceReadings[i].lastWatSuccess = false;
    }
    prefs.end();
}

// force = true означает "сохранить прямо сейчас, не ждать таймера"
void saveDeviceToPrefs(int idx, bool force) {
    if (idx < 0 || idx >= MAX_DEVICES) return;
    
    // Если это не принудительное сохранение, просто поднимаем флаг.
    // Данные запишутся в Flash раз в 10 минут через flushDevicesToDisk()
    if (!force) {
        devices[idx].needsSave = true;
        return;
    }

    prefs.begin("devices", false);
    String key = "dev" + String(idx);
    prefs.putString((key + "_addr").c_str(), devices[idx].addr);
    prefs.putString((key + "_name").c_str(), devices[idx].name);
    prefs.putString((key + "_if").c_str(), devices[idx].interface);
    prefs.putULong((key + "_lastWat").c_str(), devices[idx].lastWateringSec);
    prefs.putFloat((key + "_tempSum").c_str(), devices[idx].dailyTempSum);
    prefs.putInt((key + "_tempCnt").c_str(), devices[idx].dailyTempCount);
    prefs.putInt((key + "_lastDay").c_str(), devices[idx].lastDayOfYear);
    
    String sKey = key + "_sc";
    prefs.putInt((sKey + "Cnt").c_str(), devices[idx].scheduleCount);
    for (int j = 0; j < devices[idx].scheduleCount; j++) {
        String ruleKey = sKey + String(j);
        prefs.putFloat((ruleKey + "Intv").c_str(), devices[idx].schedules[j].intervalMinutes);
        prefs.putFloat((ruleKey + "Thr").c_str(), devices[idx].schedules[j].tempThreshold);
        prefs.putInt((ruleKey + "Cond").c_str(), devices[idx].schedules[j].tempCondition);
        prefs.putInt((ruleKey + "Hr").c_str(), devices[idx].schedules[j].targetHour);
    }
    prefs.putInt("count", deviceCount);
    prefs.end();
    devices[idx].needsSave = false; // Сбрасываем флаг
}

// Вызывать из loop() раз в 10 минут!
void flushDevicesToDisk() {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].needsSave) {
            saveDeviceToPrefs(i, true); // Принудительно сбрасываем на диск
            Serial.printf("[FLUSH] Данные устройства %s сохранены\n", devices[i].addr.c_str());
        }
    }
}

int findDeviceIndex(String addr) { for (int i = 0; i < deviceCount; i++) { if (devices[i].addr == addr) return i; } return -1; }

void updateDeviceData(String addr, String interface, AdvrtData& data) {
    int idx = findDeviceIndex(addr);
    if (idx == -1 && deviceCount < MAX_DEVICES) {
        idx = deviceCount++; devices[idx].addr = addr; devices[idx].name = addr; devices[idx].interface = interface;
        devices[idx].lastWateringSec = 0; devices[idx].dailyTempSum = 0; devices[idx].dailyTempCount = 0; devices[idx].lastDayOfYear = getDayOfYear();
        devices[idx].scheduleCount = 1; devices[idx].schedules[0].intervalMinutes = 1440.0; devices[idx].schedules[0].tempThreshold = 25.0; devices[idx].schedules[0].tempCondition = 0; devices[idx].schedules[0].targetHour = -1;
    }
    if (idx >= 0) {
        deviceReadings[idx].tempAir = data.tempAir; deviceReadings[idx].humAir = data.humAir; deviceReadings[idx].pressure = data.pressure; deviceReadings[idx].humSoil = data.humSoil;
        deviceReadings[idx].lastSeenMillis = millis(); deviceReadings[idx].pumpOn = (data.flagsStr[2] == '1'); deviceReadings[idx].flagsStr = String(data.flagsStr); deviceReadings[idx].lastWatSuccess = data.lastWatStatus;
        char wt[30]; snprintf(wt, sizeof(wt), "%dг %dд %02dч %02dм %02dс", data.workY, data.workD, data.workH, data.workM, data.workS); deviceReadings[idx].workTime = String(wt);
        char lw[30]; snprintf(lw, sizeof(lw), "%dг %dд %02dч %02dм %02dс", data.lastWatY, data.lastWatD, data.lastWatH, data.lastWatM, data.lastWatS); deviceReadings[idx].lastWatTime = String(lw);
        if (!deviceReadings[idx].isVerified) { deviceReadings[idx].isVerified = true; saveDeviceToPrefs(idx, true); Serial.printf("[VERIFY] %s\n", addr.c_str()); }
    }
}

void registerAliveDevice(int addrNum) {
    if (addrNum == 0) return; char fullAddr[4]; snprintf(fullAddr, sizeof(fullAddr), "#%02d", addrNum); int idx = findDeviceIndex(fullAddr);
    if (idx == -1 && deviceCount < MAX_DEVICES) {
        idx = deviceCount++; devices[idx].addr = fullAddr; devices[idx].name = fullAddr; devices[idx].interface = "HC12";
        devices[idx].lastWateringSec = 0; devices[idx].dailyTempSum = 0; devices[idx].dailyTempCount = 0; devices[idx].lastDayOfYear = getDayOfYear();
        devices[idx].scheduleCount = 1; devices[idx].schedules[0].intervalMinutes = 1440.0; devices[idx].schedules[0].tempThreshold = 25.0; devices[idx].schedules[0].tempCondition = 0; devices[idx].schedules[0].targetHour = -1;
        deviceReadings[idx].tempAir = -999; deviceReadings[idx].humAir = -999; deviceReadings[idx].pressure = -999; deviceReadings[idx].humSoil = -999;
        deviceReadings[idx].pumpOn = false; deviceReadings[idx].isVerified = false; deviceReadings[idx].lastWatSuccess = false;
        Serial.printf("[DISCOVERY] %s\n", fullAddr);
    }
    if (idx >= 0) deviceReadings[idx].lastSeenMillis = millis();
}

void cleanupUnverifiedDevices() { for (int i = 0; i < deviceCount; i++) { if (!deviceReadings[i].isVerified && (millis() - deviceReadings[i].lastSeenMillis > 60000)) { for (int j = i; j < deviceCount - 1; j++) { devices[j] = devices[j+1]; deviceReadings[j] = deviceReadings[j+1]; } deviceCount--; i--; } } }

void pollNextDevice() {
    if (deviceCount == 0) return;
    for (int i = 0; i < deviceCount; i++) {
        currentPollIdx = currentPollIdx % deviceCount; String addr = devices[currentPollIdx].addr; unsigned long lastSeen = deviceReadings[currentPollIdx].lastSeenMillis;
        if (addr == "#00") { currentPollIdx++; continue; }
        if (lastSeen > 0 && (millis() - lastSeen < 60000)) { sendPacketToRadio(addr.substring(1).c_str(), "ADVRT"); currentPollIdx++; return; }
        currentPollIdx++;
    }
}

void clearAllDevices() { prefs.begin("devices", false); prefs.clear(); prefs.end(); deviceCount = 0; Serial.println("[DB] База очищена!"); }
int getDayOfYear() { DateTime now = getRTCNow(); int day = now.day(), month = now.month(), year = now.year(); int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) daysInMonth[1] = 29; int dayOfYear = 0; for (int i = 1; i < month; i++) dayOfYear += daysInMonth[i - 1]; return dayOfYear + day; }

void updateDailyAverages() { 
    float localTemp, pressure; readLocalSensors(localTemp, pressure); if (localTemp == -999) return; int currentDay = getDayOfYear(); 
    for (int i = 0; i < deviceCount; i++) { 
        if (!deviceReadings[i].isVerified) continue; 
        if (currentDay != devices[i].lastDayOfYear) { devices[i].dailyTempSum = 0; devices[i].dailyTempCount = 0; devices[i].lastDayOfYear = currentDay; saveDeviceToPrefs(i, true); } // При смене дня сохраняем сразу
        devices[i].dailyTempSum += localTemp; devices[i].dailyTempCount++; 
        saveDeviceToPrefs(i); // Отложенное сохранение (буферизация)
    } 
}

float getDeviceDailyAverageTemp(int idx) { if (idx < 0 || idx >= deviceCount || !deviceReadings[idx].isVerified || devices[idx].dailyTempCount == 0) return -999; return devices[idx].dailyTempSum / devices[idx].dailyTempCount; }

unsigned long getSecondsUntilNextWatering(int idx) {
    if (idx < 0 || devices[idx].scheduleCount == 0) return UINT32_MAX;
    unsigned long nowSec = getRTCNow().unixtime(); unsigned long minNextSec = UINT32_MAX;
    for (int j = 0; j < devices[idx].scheduleCount; j++) {
        unsigned long nextSec = devices[idx].lastWateringSec + (unsigned long)(devices[idx].schedules[j].intervalMinutes * 60.0);
        if (nextSec > nowSec) { if (nextSec < minNextSec) minNextSec = nextSec; } else return 0;
    }
    return minNextSec - nowSec;
}

void checkSchedules() {
    DateTime now = getRTCNow(); unsigned long nowSec = now.unixtime(); int currentHour = now.hour();
    for (int i = 0; i < deviceCount; i++) {
        if (!deviceReadings[i].isVerified) continue;
        for (int j = 0; j < devices[i].scheduleCount; j++) {
            unsigned long lastSec = devices[i].lastWateringSec;
            float intervalSec = devices[i].schedules[j].intervalMinutes * 60.0;
            if (lastSec == 0 || (nowSec - lastSec) >= intervalSec) {
                bool shouldWater = true;
                if (devices[i].schedules[j].targetHour != -1) { if (currentHour != devices[i].schedules[j].targetHour) shouldWater = false; }
                if (shouldWater && devices[i].schedules[j].tempCondition != 0) {
                    float avgTemp = getDeviceDailyAverageTemp(i); if (avgTemp == -999) shouldWater = false;
                    else { if (devices[i].schedules[j].tempCondition == 1 && avgTemp <= devices[i].schedules[j].tempThreshold) shouldWater = false; if (devices[i].schedules[j].tempCondition == -1 && avgTemp >= devices[i].schedules[j].tempThreshold) shouldWater = false; }
                }
                if (shouldWater) {
                    Serial.printf("ПОЛИВ %s по правилу %d\n", devices[i].addr.c_str(), j);
                    sendCommandToDevice(devices[i].addr, "PUMP 0005");
                    devices[i].lastWateringSec = nowSec;
                    saveDeviceToPrefs(i, true);
                    break; 
                }
            }
        }
    }
}

void sendCommandToDevice(String addr, String command) { int idx = findDeviceIndex(addr); if (idx == -1) return; if (devices[idx].interface == "HC12") { sendPacketToRadio(addr.substring(1).c_str(), command.c_str()); } else if (devices[idx].interface == "RAK") { String cmd = addr + ":CMD:" + command; String hexCmd = ""; for (unsigned int i = 0; i < cmd.length(); i++) hexCmd += String((uint8_t)cmd[i], HEX); Serial1.println("AT+SEND=" + hexCmd); } }