#ifndef RINGBUF_H_
#define RINGBUF_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *buf;
    unsigned char capacity;  // Must be a power of 2
    unsigned char len;
    unsigned char headIdx;
    unsigned char tailIdx;
    unsigned char watermark;
    unsigned int overflows;
} RingbufType;

void ringbufInit(RingbufType *ringbuf, unsigned char *buf, unsigned int size);
unsigned char ringbufLen(RingbufType *ringbuf);
unsigned char ringbufWatermark(RingbufType *ringbuf);
unsigned int ringbufOverflows(RingbufType *ringbuf);
bool ringbufWrite(RingbufType *ringbuf, unsigned char data);
bool ringbufRead(RingbufType *ringbuf, unsigned char *data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* RINGBUF_H_ */