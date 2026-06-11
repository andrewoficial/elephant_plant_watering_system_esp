#include "protocol.h"
#include "advrt_parser.h"
#include "device_manager.h"

// ================== CRC-16 Таблица ==================
static const uint16_t crc16_table[256] = {
  0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
  0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
  0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
  0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
  0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
  0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
  0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
  0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
  0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
  0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
  0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
  0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
  0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
  0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
  0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
  0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
  0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
  0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
  0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
  0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
  0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
  0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
  0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
  0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
  0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
  0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
  0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
  0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
  0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
  0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
  0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
  0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

uint16_t crc16(const uint8_t* data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  while (len--) {
    uint8_t idx = (crc ^ *data++) & 0xFF;
    crc = (crc >> 8) ^ crc16_table[idx];
  }
  return crc;
}

// ================== Кольцевой буфер приёма от радио ==================
#define RX_BUF_SIZE 256
static char rxBuf[RX_BUF_SIZE];
static uint16_t rxHead = 0;
static uint16_t rxTail = 0;

static void rxPush(char c) {
  uint16_t next = (rxHead + 1) % RX_BUF_SIZE;
  if (next != rxTail) {
    rxBuf[rxHead] = c;
    rxHead = next;
  }
}

static uint16_t rxAvailable() {
  return (rxHead - rxTail + RX_BUF_SIZE) % RX_BUF_SIZE;
}

static char rxPeek(uint16_t i) {
  return rxBuf[(rxTail + i) % RX_BUF_SIZE];
}

static void rxSkip(uint16_t n) {
  rxTail = (rxTail + n) % RX_BUF_SIZE;
}

static int findHash(uint16_t offset) {
  uint16_t avail = rxAvailable();
  for (uint16_t i = offset; i < avail; i++) {
    if (rxPeek(i) == '#') return i;
  }
  return -1;
}

// ================== Обработка буфера ==================
void processRadioBuffer() {
  // Чтение байт из Serial2 (HC-12)
  while (Serial2.available()) {
    char c = Serial2.read();
    //Serial.write(c); // <--- ДОБАВЛЕНО: Дублируем всё из HC-12 в терминал
    rxPush(c);
  }

  // Извлечение полного пакета
  while (rxAvailable() >= 9) {
    int hashPos = findHash(0);
    if (hashPos == -1) {
      rxSkip(rxAvailable());
      break;
    }
    if (hashPos > 0) rxSkip(hashPos);
    if (rxAvailable() < 5) break;

    char header[6];
    for (int i = 0; i < 5; i++) header[i] = rxPeek(i);
    header[5] = 0;

    char addrStr[3] = { header[1], header[2], 0 };
    char lenStr[3]  = { header[3], header[4], 0 };
    int addr = atoi(addrStr);
    int len  = atoi(lenStr);
    if (len <= 0 || len > 99) { rxSkip(1); continue; }

    uint16_t totalLen = 5 + len + 4;
    if (rxAvailable() < totalLen) break;

    char payload[100];
    for (int i = 0; i < len; i++) payload[i] = rxPeek(5 + i);
    payload[len] = 0;

    char crcStr[5];
    for (int i = 0; i < 4; i++) crcStr[i] = rxPeek(5 + len + i);
    crcStr[4] = 0;

    uint16_t rxCrc = (uint16_t)strtol(crcStr, NULL, 16);
    uint16_t calcCrc = crc16((const uint8_t*)payload, len);

    if (rxCrc == calcCrc) {
      //Serial.printf("\n[RX OK] #%s%02d%s%s\n", addrStr, len, payload, crcStr);
      
      // Проверяем тип пакета
      String payloadStr = String(payload);
      
      // 1. Ответ на POOL (обнаружение устройства)
      if (payloadStr.startsWith("ALIVE")) {
        registerAliveDevice(addr);
      } 
      // 2. Ответ ADVRT (телеметрия 55 байт)
      else if (len >= 55 && len <= 58 && (payload[0] == '+' || payload[0] == '-')) {
        AdvrtData data;
        if (parseAdvrt(payload, len, data)) {
          int addrNum = atoi(addrStr);
          char fullAddr[4];
          snprintf(fullAddr, sizeof(fullAddr), "#%02d", addrNum);
          updateDeviceData(fullAddr, "HC12", data);
        }
      } 
      // 3. Любой другой пакет
      else {
        Serial.printf("[PKG] Addr: %d, Payload: %s\n", addr, payload);
      }
    } else {
      Serial.printf("\n[CRC ERR] Got %s, Calc %04X\n", crcStr, calcCrc);
    }
    rxSkip(totalLen);
  }
}

// ================== Отправка пакета ==================
void sendPacketToRadio(const char* addrStr, const char* payload) {
  uint8_t payloadLen = strlen(payload);
  if (payloadLen == 0 || payloadLen > 99) return;

  char lenStr[3];
  snprintf(lenStr, sizeof(lenStr), "%02d", payloadLen);

  uint16_t crc = crc16((const uint8_t*)payload, payloadLen);
  char crcStr[5];
  sprintf(crcStr, "%04X", crc);

  Serial2.print('#');
  Serial2.print(addrStr);
  Serial2.print(lenStr);
  Serial2.print(payload);
  Serial2.print(crcStr);
  Serial2.print('\n');

  //Serial.printf("[TX] #%s%s%s%s\n", addrStr, lenStr, payload, crcStr);
}