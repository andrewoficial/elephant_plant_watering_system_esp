#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>
#include "advrt_parser.h"

#define MAX_DEVICES 10
#define MAX_SCHEDULES 5

struct ScheduleRule {
    float intervalMinutes;   
    float tempThreshold;     
    int8_t tempCondition;    // 0 = Любая, 1 = Выше порога, -1 = Ниже порога
    int8_t targetHour;       // Час полива (0-23). -1 = в любое время
};

struct DeviceConfig {
    String addr;                
    String name;                
    String interface;           
    
    unsigned long lastWateringSec; 
    float dailyTempSum;         
    int dailyTempCount;         
    int lastDayOfYear;          

    bool needsSave;             // ФЛАГ: нужно ли сбросить данные во Flash

    ScheduleRule schedules[MAX_SCHEDULES];
    int scheduleCount;
};

struct DeviceReadings {
    float tempAir;
    float humAir;
    float pressure;
    float humSoil;
    bool pumpOn;
    unsigned long lastSeenMillis;
    bool isVerified;
    String workTime;
    String lastWatTime;
    bool lastWatSuccess;
    String flagsStr;
};

extern DeviceConfig devices[MAX_DEVICES];
extern DeviceReadings deviceReadings[MAX_DEVICES];
extern int deviceCount;

void loadDevicesFromPrefs();
void saveDeviceToPrefs(int idx, bool force = false); // force = true для немедленного сохранения
int findDeviceIndex(String addr);
void updateDeviceData(String addr, String interface, AdvrtData& data);
void registerAliveDevice(int addrNum);
void pollNextDevice();
void clearAllDevices();
void cleanupUnverifiedDevices();
void updateDailyAverages();
float getDeviceDailyAverageTemp(int idx);
unsigned long getSecondsUntilNextWatering(int idx);
void checkSchedules();
void sendCommandToDevice(String addr, String command);
int getDayOfYear();
void flushDevicesToDisk(); // НОВАЯ: периодический сброс

#endif