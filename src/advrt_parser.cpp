#include "advrt_parser.h"

bool parseAdvrt(const char* payload, uint8_t len, AdvrtData& out) {
    if (len < 55) return false;
    
    char buf[56];
    strncpy(buf, payload, 55);
    buf[55] = 0;

    // Температура, влажность, давление, почва
    int tempRaw; sscanf(buf, "%5d", &tempRaw);
    int humRaw; sscanf(buf+5, "%3d", &humRaw);
    int pressRaw; sscanf(buf+8, "%4d", &pressRaw);
    int soilRaw; sscanf(buf+12, "%4d", &soilRaw);
    
    // Время работы
    sscanf(buf+16, "%2d", &out.workY);
    sscanf(buf+18, "%3d", &out.workD);
    sscanf(buf+21, "%2d", &out.workH);
    sscanf(buf+23, "%2d", &out.workM);
    sscanf(buf+25, "%2d", &out.workS);

    // Флаги (16 символов)
    strncpy(out.flagsStr, buf+27, 16);
    out.flagsStr[16] = 0;

    // Время последнего полива
    sscanf(buf+43, "%2d", &out.lastWatY);
    sscanf(buf+45, "%3d", &out.lastWatD);
    sscanf(buf+48, "%2d", &out.lastWatH);
    sscanf(buf+50, "%2d", &out.lastWatM);
    sscanf(buf+52, "%2d", &out.lastWatS);
    
    // Статус помпы при последнем поливе
    int pStat; 
    sscanf(buf+54, "%1d", &pStat);
    out.lastWatStatus = (pStat == 1);

    // Конвертация сырых данных в физические величины
    out.tempAir = tempRaw / 100.0;
    out.humAir = humRaw / 10.0;
    out.pressure = pressRaw / 10.0;
    out.humSoil = soilRaw / 10.0;

    return true;
}