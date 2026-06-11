#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

// Инициализация обработки радио-буфера (вызывать в loop)
void processRadioBuffer();

// Отправка команды устройству с расчетом CRC
void sendPacketToRadio(const char* addrStr, const char* payload);

#endif