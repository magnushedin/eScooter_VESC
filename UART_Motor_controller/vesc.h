#ifndef VESC_H_
#define VESC_H_

#include <stdint.h>

typedef struct {
    unsigned char startByte;
    unsigned char payloadLen;
} VescHeaderType;

typedef struct {
    unsigned char rbCapacity;
    unsigned char rbWatermark;
    unsigned int rxOverflows;
    unsigned int txOverflows;
} VescSerialStatType;

static unsigned int createPacket(unsigned char *data, unsigned int dataLen, unsigned char *txBuf, unsigned int txBufLen);

void vescInit();
void vescSetBrakeCurrent(float current, HardwareSerial *serialPort);
void vescSetCurrent(float current, HardwareSerial *serialPort);
void vescSetHandbrake(float current, HardwareSerial *serialPort);
void vescSetRpm(int32_t rpm, HardwareSerial *serialPort);
void vescRequestData(uint32_t mask, HardwareSerial *serialPort);
int vescTryDecodePacket(unsigned char dataBuf[], unsigned int dataLen);
int vescRxDataAvailable();
void vescGetSerialStats(VescSerialStatType *stats);

#endif /* VESC_H_ */
