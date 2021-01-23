#include <HardwareSerial.h>
#include "vesc.h"

#include "datatypes.h"
#include "crc.h"
#include "buffer.h"
#include "ringbuf.h"

#include <string.h>
#include <Arduino.h>

#define MAX_PACKET_SIZE           (255)  // Support for shot packages only
#define VESC_HEADER_TRAILER_SIZE  (5)    // Start, len, crc1, crc2, stop

HardwareSerial* serialPort;
static RingbufType RxRingbuf;
static unsigned char RxBuffer[128];
static unsigned int serialTxOverflows;

//------------------------------------------------------------------------------
static unsigned int createPacket(unsigned char *data, unsigned int dataLen, unsigned char *txBuf, unsigned int txBufLen)
{
    int idx = 0;

	if (dataLen == 0 || dataLen + VESC_HEADER_TRAILER_SIZE > txBufLen || dataLen > MAX_PACKET_SIZE) {
		return 0;
	}

    // Header
    txBuf[idx++] = 2; // Start byte (short packet)
    txBuf[idx++] = dataLen;

    // Payload
	memcpy(txBuf + idx, data, dataLen);
	idx += dataLen;

    // Trailer with CRC
	unsigned short crc = crc16(data, dataLen);
	txBuf[idx++] = (uint8_t)(crc >> 8);
	txBuf[idx++] = (uint8_t)(crc & 0xFF);
	txBuf[idx++] = 3; // Stop byte

    return idx;
}

//------------------------------------------------------------------------------
void vescInit(void)
{
    // RX Serial buffer
    ringbufInit(&RxRingbuf, RxBuffer, sizeof(RxBuffer));
}

//------------------------------------------------------------------------------
void vescSetBrakeCurrent(float current, HardwareSerial *serialPort)
{
    unsigned char cmd[5];
    unsigned char txBuf[sizeof(cmd) + VESC_HEADER_TRAILER_SIZE];
    int32_t idx = 0;
    unsigned int packetLen;

    cmd[idx++] = COMM_SET_CURRENT_BRAKE;
    buffer_append_int32((uint8_t*)cmd, (int32_t)(current * 1000), &idx);

    packetLen = createPacket(cmd, idx, txBuf, sizeof(txBuf));

    if (serialPort->availableForWrite() >= packetLen) {
        serialPort->write(txBuf, packetLen);
    }
    else {
        serialTxOverflows++;
    }
}

//------------------------------------------------------------------------------
void vescSetCurrent(float current, HardwareSerial *serialPort)
{
    unsigned char cmd[5];
    unsigned char txBuf[sizeof(cmd) + VESC_HEADER_TRAILER_SIZE];
    int32_t idx = 0;
    unsigned int packetLen;

    cmd[idx++] = COMM_SET_CURRENT;
    buffer_append_int32((uint8_t*)cmd, (int32_t)(current * 1000), &idx);

    packetLen = createPacket(cmd, idx, txBuf, sizeof(txBuf));

    if (serialPort->availableForWrite() >= packetLen) {
        serialPort->write(txBuf, packetLen);
        //Serial.println("Sending data");
    }
    else {
        serialTxOverflows++;
        //Serial.println("buffer overflow");
    }
}

//------------------------------------------------------------------------------
void vescSetHandbrake(float current, HardwareSerial *serialPort)
{
    unsigned char cmd[5];
    unsigned char txBuf[sizeof(cmd) + VESC_HEADER_TRAILER_SIZE];
    int32_t idx = 0;
    unsigned int packetLen;

    cmd[idx++] = COMM_SET_HANDBRAKE;
    buffer_append_float32((uint8_t*)cmd, current, 1e3, &idx);

    packetLen = createPacket(cmd, idx, txBuf, sizeof(txBuf));

    if (serialPort->availableForWrite() >= packetLen) {
        serialPort->write(txBuf, packetLen);
    }
    else {
        serialTxOverflows++;
    }
}

//------------------------------------------------------------------------------
void vescSetRpm(int32_t rpm, HardwareSerial *serialPort)
{
    unsigned char cmd[5];
    unsigned char txBuf[sizeof(cmd) + VESC_HEADER_TRAILER_SIZE];
    int32_t idx = 0;
    unsigned int packetLen;

    cmd[idx++] = COMM_SET_RPM;
    buffer_append_int32((uint8_t*)cmd, rpm, &idx);

    packetLen = createPacket(cmd, idx, txBuf, sizeof(txBuf));

    if (serialPort->availableForWrite() >= packetLen) {
        serialPort->write(txBuf, packetLen);
    }
    else {
        serialTxOverflows++;
    }
}

//------------------------------------------------------------------------------
void vescRequestData(uint32_t mask, HardwareSerial *serialPort)
{
    //Serial.println("vescRequestData");
    unsigned char cmd[5];
    unsigned char txBuf[sizeof(cmd) + VESC_HEADER_TRAILER_SIZE];
    int32_t idx = 0;
    unsigned int packetLen;

    cmd[idx++] = COMM_GET_VALUES_SETUP_SELECTIVE;
    buffer_append_uint32((uint8_t*)cmd, mask, &idx);

    packetLen = createPacket(cmd, idx, txBuf, sizeof(txBuf));

    if (serialPort->availableForWrite() >= packetLen) {
        //Serial.println(txBuf[0]);
        serialPort->write(txBuf, packetLen);
    }
    else {
        Serial.println("buffer full");
        serialTxOverflows++;
    }
}

//------------------------------------------------------------------------------
int vescTryDecodePacket(unsigned char dataBuf[], unsigned int dataLen)
{
    static VescHeaderType header; // Stores state between calls in case of incomplete packets

    if (dataBuf == NULL) {
        return 0;
    }

    if (ringbufLen(&RxRingbuf) < 2) {
        return -1; // Need more data to start decoding a packet
    }

    // Try and find start byte
    if (header.startByte == 0) {
        while (ringbufRead(&RxRingbuf, &header.startByte)) {
            if (header.startByte == 2) { // Support for short packages only
                break;
            }
        }
    }

    // Try and find payload length
    if (header.payloadLen == 0) {
        if (!ringbufRead(&RxRingbuf, &header.payloadLen)) {
            return -2;
        }

        // Basic validation checks.
        if (header.payloadLen == 0 || header.payloadLen > dataLen) {
            memset(&header, 0, sizeof(header)); // Setting header to 0 will reset packet state.
            return -3;
        }
    }

    if (ringbufLen(&RxRingbuf) < header.payloadLen + 3) {
        return -4; // A complete packet not received yet
    }

    // Read payload
    for (int i = 0; i < header.payloadLen; i++) {
        ringbufRead(&RxRingbuf, &dataBuf[i]);
    }

    // Read and check CRC
	unsigned char crcBytes[2];
    ringbufRead(&RxRingbuf, &crcBytes[0]);
    ringbufRead(&RxRingbuf, &crcBytes[1]);
    unsigned short crcCalc = crc16(dataBuf, header.payloadLen);
	unsigned short crcRx = 0;
    crcRx  = crcBytes[0] << 8;
    crcRx |= crcBytes[1];

    if (crcRx != crcCalc) {
        memset(&header, 0, sizeof(header));
        return -5;
    }

    // Read and check stop byte
    unsigned char stopByte = 0;
    ringbufRead(&RxRingbuf, &stopByte);
    if (stopByte != 3) {
        memset(&header, 0, sizeof(header));
        return -6;
    }

    // A packet is succesfully received
    unsigned char len = header.payloadLen;
    memset(&header, 0, sizeof(header));
    return len;
}

//------------------------------------------------------------------------------
int vescRxDataAvailable()
{
    return ringbufLen(&RxRingbuf);
}

//------------------------------------------------------------------------------
void vescGetSerialStats(VescSerialStatType *stats)
{
    if (stats == NULL) {
        return;
    }

    stats->rbCapacity = sizeof(RxBuffer);
    stats->rbWatermark = ringbufWatermark(&RxRingbuf);
    stats->rxOverflows = ringbufOverflows(&RxRingbuf);
    stats->txOverflows = serialTxOverflows;
}

//------------------------------------------------------------------------------
//  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
//  routine is run between each time loop() runs, so using delay inside loop can
//  delay response. Multiple bytes of data may be available.
void serialEvent3()
{
  unsigned char data;
    //Serial.println("Data available on Serial3, Reading UART from VESC1");
    while (Serial3.available()) {
        data = Serial3.read();
        //Serial.print("Serial data: ");
        //Serial.println(data);
        ringbufWrite(&RxRingbuf, (unsigned char) data);
    }
}

//------------------------------------------------------------------------------
// Don't care about serial5 at the moment. This is VESC2
void serialEvent5()
{
    while (Serial5.available()) {
        (void)Serial5.read();
    }
}

