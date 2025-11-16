#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "mipi_display.h"
#include "analyzer.h"

static uint32_t frame_count = 0;
uint8_t pixels[1024];

void analyzer_i80_8bit_video_sync(const uint8_t *src, uint32_t len)
{
	uint16_t xs, ys, xe, ye;
	uint32_t clip_size;
	uint8_t grade = 0;

	for (int i = 0; i < len; i++) {
		if (src[i] == MIPI_DCS_SET_COLUMN_ADDRESS) {
			xs = (src[i + 1] << 8) | src[i + 2];
			xe = (src[i + 3] << 8) | src[i + 4];
			// printf("Column Address: %d-%d\n", xs, xe);
			grade++;
		}

		if (src[i] == MIPI_DCS_SET_PAGE_ADDRESS) {
			ys = (src[i + 1] << 8) | src[i + 2];
			ye = (src[i + 3] << 8) | src[i + 4];
			// printf("Page Address: %d-%d\n", ys, ye);
			grade++;
		}

		if (src[i] == MIPI_DCS_WRITE_MEMORY_START) {
			// printf("Read Memory Start\n");
			clip_size = (xe - xs + 1) * (ye - ys + 1);
			grade++;
		}

		if (grade == 3) {
			// pixels = malloc(clip_size);
			if ((src + clip_size) == NULL)
				return;
			//.
			// memcpy(pixels, src + 1, clip_size);
			printf("[%04d] video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n",
			       frame_count++, xs, ys, xe, ye, clip_size);
			hexdump(src, 5);
			// free(pixels);
			grade = 0;
		}
	}
}

void hexdump(const void *data, uint32_t size)
{
	const uint8_t *data_ptr = (const uint8_t *)data;
	uint32_t i, b;

	for (i = 0; i < size; i++) {
		if (i % 16 == 0) {
			printf("%08x  ", (uint32_t)data_ptr + i);
		}
		if (i % 8 == 0) {
			printf(" ");
		}
		printf("%02x ", data_ptr[i]);
		if (i % 16 == 15) {
			printf(" |");
			for (b = 0; b < 16; b++) {
				if (isprint(data_ptr[i + b - 15])) {
					printf("%c", data_ptr[i + b - 15]);
				} else {
					printf(".");
				}
			}
			printf("|\n");
		}
	}
	printf("%08x\n", 16 + size - (size % 16));
}
