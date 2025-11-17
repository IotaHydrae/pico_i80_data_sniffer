#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <machine/endian.h>

#include "mipi_display.h"
#include "analyzer.h"

// #if _BYTE_ORDER == _LITTLE_ENDIAN
// #define htobe16(x) __bswap16(x)
// #endif

static uint32_t frame_count = 0;
const uint8_t *pixels;

struct mipi_dcs_column_addr {
	uint16_t xs;
	uint16_t xe;
};

struct mipi_dcs_row_addr {
	uint16_t ys;
	uint16_t ye;
};

void analyzer_i80_8bit_video_sync(const uint8_t *src, uint32_t len)
{
	// struct mipi_dcs_column_addr *col;
	// struct mipi_dcs_row_addr *row;
	uint16_t xs, xe;
	uint16_t ys, ye;
	uint32_t clip_size;
	uint8_t grade = 0;

	for (int i = 0; i < len; i++) {
		if (src[i] == MIPI_DCS_SET_COLUMN_ADDRESS) {
			xs = (src[i + 1] << 8) | (src[i + 2] & 0xFF);
			xe = (src[i + 3] << 8) | (src[i + 4] & 0xFF);

			// col = (struct mipi_dcs_column_addr *)&src[i + 1];
			// xs = htobe16(col->xs);
			// xe = htobe16(col->xe);
			// printf("Column Address: %d-%d\n", xs, xe);

			if (xs == xe)
				continue;

			grade++;
		}

		if (src[i] == MIPI_DCS_SET_PAGE_ADDRESS) {
			ys = (src[i + 1] << 8) | (src[i + 2] & 0xFF);
			ye = (src[i + 3] << 8) | (src[i + 4] & 0xFF);

			// row = (struct mipi_dcs_row_addr *)&src[i + 1];
			// ys = htobe16(row->ys);
			// ye = htobe16(row->ye);
			// printf("Page Address: %d-%d\n", ys, ye);

			if (ys == ye)
				continue;

			grade++;
		}

		if (src[i] == MIPI_DCS_WRITE_MEMORY_START) {
			// printf("Read Memory Start, %02x\n", src[i]);
			clip_size = (xe - xs + 1) * (ye - ys + 1);

			grade++;
			if (grade == 3)
				pixels = &src[i] + 1;
			else
				grade = 0;
		}

		if (grade == 3) {
			grade = 0;
			// if ((&src[i] + clip_size) == NULL)
			// 	return;
			// if (clip_size > len)
			// 	return;

			if (clip_size != ((xe - xs + 1) * (ye - ys + 1)))
				continue;

			if (clip_size < 0)
				continue;

			printf("[%04d] video sync: xs=%d, ys=%d, xe=%d, ye=%d, clip_size=%d\n",
			       frame_count++, xs, ys, xe, ye, clip_size);
			hexdump(pixels, 5);
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
