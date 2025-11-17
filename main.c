/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i80_input.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/vreg.h"
#include "pico/stdlib.h"

#include "mipi_display.h"
#include "analyzer.h"

/*
 * GPIO0 ~ GPIO7 connect to DB0 ~ DB7 pins
 * GPIO8 connect to WR pin
 */
#define PIO_I80_INPUT_DB_PIN_BASE 0
#define PIO_I80_INPUT_DB_PIN_COUNT 8
#define PIO_I80_INPUT_WR_PIN \
	(PIO_I80_INPUT_DB_PIN_BASE + PIO_I80_INPUT_DB_PIN_COUNT)

#define MIPI_DCS_COLUMN_ADDRESS_VAL(xs, xe) \
	xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF
#define MIPI_DCS_PAGE_ADDRESS_VAL(ys, ye) ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF

uint8_t txbuf[] = { MIPI_DCS_SET_COLUMN_ADDRESS,
		    MIPI_DCS_COLUMN_ADDRESS_VAL(0, 239),
		    MIPI_DCS_SET_PAGE_ADDRESS,
		    MIPI_DCS_PAGE_ADDRESS_VAL(0, 239),
		    MIPI_DCS_WRITE_MEMORY_START,
		    0x12,
		    0x34,
		    0x56,
		    0x78,
		    0x9A };

#define TX_BUF_SIZE (sizeof(txbuf) / sizeof(txbuf[0]))
#define RX_BUF_SIZE (256)

static uint8_t rd_buf[RX_BUF_SIZE] = { 0 };

static uint dma_rx;
static dma_channel_config c;

static inline void pio_i80_input_rd8(PIO pio, uint sm, void *buf, size_t len)
{
	dma_channel_configure(dma_rx, &c, (uint8_t *)buf, &pio->rxf[sm], len,
			      true);
	dma_channel_wait_for_finish_blocking(dma_rx);
}

#define CPU_SPEED_MHZ 240
int main()
{
	if (CPU_SPEED_MHZ > 266 && CPU_SPEED_MHZ <= 360)
		vreg_set_voltage(VREG_VOLTAGE_1_20);
	else if (CPU_SPEED_MHZ > 360 && CPU_SPEED_MHZ <= 396)
		vreg_set_voltage(VREG_VOLTAGE_1_25);
	else if (CPU_SPEED_MHZ > 396)
		vreg_set_voltage(VREG_VOLTAGE_MAX);
	else
		vreg_set_voltage(VREG_VOLTAGE_DEFAULT);

	set_sys_clock_khz(CPU_SPEED_MHZ * 1000, true);
	clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
			CPU_SPEED_MHZ * MHZ, CPU_SPEED_MHZ * MHZ);
	stdio_uart_init_full(uart0, 115200, 16, 17);

	printf("\n\n\nPico I80 Data Sniffer Demo\n");

	PIO pio = pio0;
	uint offset = pio_add_program(pio, &i80_input_program);
	uint sm = pio_claim_unused_sm(pio, true);
	i80_input_program_init(pio, sm, offset, PIO_I80_INPUT_DB_PIN_BASE,
			       PIO_I80_INPUT_DB_PIN_COUNT);

	dma_rx = dma_claim_unused_channel(true);
	c = dma_channel_get_default_config(dma_rx);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
	channel_config_set_write_increment(&c, true);
	channel_config_set_read_increment(&c, false);
	channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

	printf("\nPreset Tx Data:\n");
	hexdump(txbuf, sizeof(txbuf));

	for (int i = 0; i < 5; i++) {
		printf("try %d\n", i);
		memset(rd_buf, 0x00, sizeof(rd_buf));
		pio_i80_input_rd8(pio, sm, rd_buf, sizeof(rd_buf));
		// hexdump(rd_buf, 16);
		analyzer_i80_8bit_video_sync(rd_buf, sizeof(rd_buf));
	}

	puts("Done.");

	return 0;
}
