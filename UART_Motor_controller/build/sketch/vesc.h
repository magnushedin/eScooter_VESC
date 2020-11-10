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

void vescInit(HardwareSerial* port);
void vescSetBrakeCurrent(float current);
void vescSetCurrent(float current);
void vescSetHandbrake(float current);
void vescSetRpm(int32_t rpm);
void vescRequestData(uint32_t mask);
int vescTryDecodePacket(unsigned char dataBuf[], unsigned int dataLen);
int vescRxDataAvailable();
void vescGetSerialStats(VescSerialStatType *stats);

#endif /* VESC_H_ */
