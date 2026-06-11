#include "web_server.h"
#include "device_manager.h"
#include "peripherals.h"
#include "html_page.h"

WebServer server(80);

String formatTime(DateTime dt) { char buf[20]; sprintf(buf, "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second()); return String(buf); }

String formatDuration(unsigned long secAgo) {
    if (secAgo == UINT32_MAX || secAgo > 86400*365) return "Нет расписания";
    if (secAgo < 60) return String(secAgo) + " сек";
    if (secAgo < 3600) return String(secAgo/60) + " мин";
    unsigned long hours = secAgo/3600;
    if (hours < 24) return String(hours) + " ч " + String((secAgo%3600)/60) + " мин";
    return String(hours/24) + " дн " + String(hours%24) + " ч";
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleAPI);
    server.on("/api/config", HTTP_ANY, handleConfig);
    server.on("/api/cmd", HTTP_POST, handleCmd);
    server.on("/api/clear", HTTP_POST, handleClearDevices);
    server.onNotFound([]() { if (server.method() == HTTP_OPTIONS) { server.send(204); } else { server.send(404); } });
    server.begin();
}

void handleRoot() { server.send(200, "text/html", index_html); }

void handleAPI() {
    float localTemp, localPressure; readLocalSensors(localTemp, localPressure);
    String json = "{\"time\":\"" + formatTime(getRTCNow()) + "\",\"localTemp\":" + String(localTemp, 1) + ",\"devices\":[";
    bool firstAdded = false;
    for (int i = 0; i < deviceCount; i++) {
        DeviceConfig &d = devices[i]; DeviceReadings &r = deviceReadings[i];
        unsigned long ago = (millis() - r.lastSeenMillis) / 1000;
        if (!r.isVerified || r.lastSeenMillis == 0 || ago > 300) continue;
        float avgTemp = getDeviceDailyAverageTemp(i); unsigned long nextWat = getSecondsUntilNextWatering(i);
        if (firstAdded) json += ","; firstAdded = true;
        json += "{\"addr\":\"" + d.addr + "\",\"name\":\"" + d.name + "\",\"interface\":\"" + d.interface + "\",";
        json += "\"tempAir\":" + String(r.tempAir, 1) + ",\"humAir\":" + String(r.humAir, 0) + ",\"pressure\":" + String(r.pressure, 1) + ",\"humSoil\":" + String(r.humSoil, 0) + ",";
        json += "\"pumpOn\":" + String(r.pumpOn ? "true" : "false") + ",\"lastSeenAgo\":" + String(ago) + ",\"avgDailyTemp\":" + String(avgTemp, 1) + ",";
        json += "\"workTime\":\"" + r.workTime + "\",\"lastWatTime\":\"" + r.lastWatTime + "\",\"lastWatSuccess\":" + String(r.lastWatSuccess ? "true" : "false") + ",\"flags\":\"" + r.flagsStr + "\",";
        json += "\"lastWateringSystem\":\"" + formatDuration(d.lastWateringSec == 0 ? 0 : (getRTCNow().unixtime() - d.lastWateringSec)) + "\",\"nextWateringIn\":\"" + formatDuration(nextWat) + "\",";
        json += "\"schedules\":[";
        for (int j = 0; j < d.scheduleCount; j++) {
            if (j > 0) json += ","; 
            json += "{\"id\":" + String(j) + ",\"intv\":" + String(d.schedules[j].intervalMinutes, 2) + ",\"thr\":" + String(d.schedules[j].tempThreshold, 1) + ",\"cond\":" + String(d.schedules[j].tempCondition) + ",\"hour\":" + String(d.schedules[j].targetHour) + "}";
        }
        json += "]}";
    }
    json += "]}"; server.send(200, "application/json", json);
}

// Вспомогательная функция для надежного извлечения чисел из JSON
float getJsonVal(String body, const char* key, float defVal) {
    String searchKey = String("\"") + key + "\":";
    int p1 = body.indexOf(searchKey);
    if (p1 == -1) return defVal;
    int startVal = p1 + searchKey.length();
    int endVal = body.indexOf(",", startVal);
    if (endVal == -1) endVal = body.indexOf("}", startVal);
    if (endVal == -1) return defVal;
    return body.substring(startVal, endVal).toFloat();
}

void handleConfig() {
    if (server.method() == HTTP_GET) { 
        server.send(200, "application/json", "[]"); 
    } 
    else if (server.method() == HTTP_POST) {
        if (!server.hasArg("plain")) { server.send(400); return; } 
        String body = server.arg("plain");
        
        String addr = ""; 
        int p1 = body.indexOf("\"addr\":\""); 
        if(p1 != -1) { int p2 = body.indexOf("\"", p1+8); addr = body.substring(p1+8, p2); }
        
        int idx = findDeviceIndex(addr); 
        if (idx == -1) { server.send(400); return; }
        
        String action = ""; 
        p1 = body.indexOf("\"action\":\""); 
        if(p1 != -1) { int p2 = body.indexOf("\"", p1+10); action = body.substring(p1+10, p2); }

        if (action == "setName") { 
            p1 = body.indexOf("\"name\":\""); 
            if(p1!=-1) { int p2 = body.indexOf("\"", p1+8); devices[idx].name = body.substring(p1+8, p2); } 
            saveDeviceToPrefs(idx, true); server.send(200); 
        }
        else if (action == "addSchedule") { 
            if (devices[idx].scheduleCount < MAX_SCHEDULES) { 
                int sc = devices[idx].scheduleCount++; 
                
                // ПРАВИЛЬНЫЙ ПАРСИНГ ИЗ JSON:
                devices[idx].schedules[sc].intervalMinutes = getJsonVal(body, "intv", 1440.0);
                devices[idx].schedules[sc].targetHour = (int8_t)getJsonVal(body, "hour", -1.0);
                devices[idx].schedules[sc].tempCondition = (int8_t)getJsonVal(body, "cond", 0.0);
                devices[idx].schedules[sc].tempThreshold = getJsonVal(body, "thr", 25.0);

                // Защита: если интервал 0 (ошибка парсинга), ставим 1 день
                if (devices[idx].schedules[sc].intervalMinutes <= 0) {
                    devices[idx].schedules[sc].intervalMinutes = 1440;
                }

                saveDeviceToPrefs(idx, true); 
                server.send(200); 
            } else server.send(400); 
        }
        else if (action == "delSchedule") { 
            int delId = getJsonVal(body, "delId", -1.0);
            if (delId >= 0 && delId < devices[idx].scheduleCount) { 
                for (int i = delId; i < devices[idx].scheduleCount - 1; i++) 
                    devices[idx].schedules[i] = devices[idx].schedules[i+1]; 
                devices[idx].scheduleCount--; 
                saveDeviceToPrefs(idx, true); server.send(200); 
            } else server.send(400); 
        }
        else if (action == "updateSchedule") { 
            int scId = getJsonVal(body, "scId", -1.0);
            if (scId >= 0 && scId < devices[idx].scheduleCount) {
                if (body.indexOf("\"intv\":") != -1) devices[idx].schedules[scId].intervalMinutes = getJsonVal(body, "intv", 1440.0);
                if (body.indexOf("\"thr\":") != -1) devices[idx].schedules[scId].tempThreshold = getJsonVal(body, "thr", 25.0);
                if (body.indexOf("\"cond\":") != -1) devices[idx].schedules[scId].tempCondition = (int8_t)getJsonVal(body, "cond", 0.0);
                if (body.indexOf("\"hour\":") != -1) devices[idx].schedules[scId].targetHour = (int8_t)getJsonVal(body, "hour", -1.0);
                
                saveDeviceToPrefs(idx, true); server.send(200);
            } else server.send(400); 
        }
        else server.send(400);
    }
}


void handleCmd() { if (server.hasArg("plain")) { String body = server.arg("plain"), addr = "", cmd = ""; int p1 = body.indexOf("\"addr\":\""); if(p1!=-1) { int p2 = body.indexOf("\"", p1+8); addr = body.substring(p1+8, p2); } p1 = body.indexOf("\"cmd\":\""); if(p1!=-1) { int p2 = body.indexOf("\"", p1+7); cmd = body.substring(p1+7, p2); } if (addr.length() > 0 && cmd.length() > 0) { sendCommandToDevice(addr, cmd); server.send(200); return; } } server.send(400); }
void handleClearDevices() { clearAllDevices(); server.send(200); }