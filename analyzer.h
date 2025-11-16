#ifndef __ANALYZER_H
#define __ANALYZER_H

#include <stdint.h>

void hexdump(const void *data, uint32_t size);
void analyzer_i80_8bit_video_sync(const uint8_t *src, uint32_t len);

#endif
