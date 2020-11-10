#include "ringbuf.h"

#include <stddef.h>
#include <string.h>
#include <limits.h>

//------------------------------------------------------------------------------
static bool isPowerOfTwo(unsigned int x)
{
    return (x & (x - 1)) == 0;
}

//------------------------------------------------------------------------------
void ringbufInit(RingbufType *ringbuf, unsigned char *buf, unsigned int size)
{
    memset(ringbuf, 0, sizeof(ringbuf));

    if (ringbuf == NULL || buf == NULL || size > UCHAR_MAX  || !isPowerOfTwo(size)) {
        return;
    }

    ringbuf->buf = buf;
    ringbuf->capacity = size;
}

//------------------------------------------------------------------------------
unsigned char ringbufLen(RingbufType *ringbuf)
{
    if (ringbuf == NULL) {
        return 0;
    }

    return ringbuf->len;
}

//------------------------------------------------------------------------------
unsigned char ringbufWatermark(RingbufType *ringbuf)
{
    if (ringbuf == NULL) {
    return 0;
    }

    return ringbuf->watermark;
}

//------------------------------------------------------------------------------
unsigned int ringbufOverflows(RingbufType *ringbuf)
{
    if (ringbuf == NULL) {
        return 0;
    }

    return ringbuf->overflows;
}

//------------------------------------------------------------------------------
bool ringbufWrite(RingbufType *ringbuf, unsigned char data)
{
    if (ringbuf == NULL || ringbuf->buf == NULL) {
       return false;
    }

    if (ringbuf->len >= ringbuf->capacity) {
        ringbuf->overflows++;
        return false;
    }

    ringbuf->buf[ringbuf->headIdx] = data;
    ringbuf->headIdx = (ringbuf->headIdx + 1) % ringbuf->capacity;
    ringbuf->len++;

    if (ringbuf->len > ringbuf->watermark) {
        ringbuf->watermark = ringbuf->len;
    }

    return true;
}

//------------------------------------------------------------------------------
bool ringbufRead(RingbufType *ringbuf, unsigned char *data)
{
    if (ringbuf == NULL || ringbuf->buf == NULL || data == NULL) {
       return false;
    }

    if (ringbuf->len == 0) {
        *data = 0;
        return false;
    }

    *data = ringbuf->buf[ringbuf->tailIdx];
    ringbuf->tailIdx = (ringbuf->tailIdx + 1) % ringbuf->capacity;
    ringbuf->len--;

    return true;
}
