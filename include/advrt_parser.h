#ifndef ADVRT_PARSER_H
#define ADVRT_PARSER_H

#include <Arduino.h>

struct AdvrtData {
    int addr;
    float tempAir;
    float humAir;
    float pressure;
    float humSoil;
    
    // Время работы
    int workY, workD, workH, workM, workS;
    
    // Последний полив
    int lastWatY, lastWatD, lastWatH, lastWatM, lastWatS;
    bool lastWatStatus;

    // Флаги (строка 16 символов)
    char flagsStr[17];
};

bool parseAdvrt(const char* payload, uint8_t len, AdvrtData& out);

#endif