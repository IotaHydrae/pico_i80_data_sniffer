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
// #include "hardware/pll.h"
#include "hardware/vreg.h"
#include "pico/stdlib.h"

#include "analyzer.h"

// Set up a PIO state machine to shift in serial data, sampling with an
// external clock, and push the data to the RX FIFO, 8 bits at a time.
//
// Use one of the hard SPI peripherals to drive data into the SM through a
// pair of external jumper wires, then read back and print out the data from
// the SM to confirm everything worked ok.
//
// On your Pico you need to connect jumper wires to these pins:
// - GPIO 2 -> GPIO 5 (clock output to clock input)
// - GPIO 3 -> GPIO 4 (data output to data input)

#define PIO_I80_INPUT_DB_PIN_BASE 0
#define PIO_I80_INPUT_DB_PIN_COUNT 8
#define PIO_I80_INPUT_WR_PIN \
	(PIO_I80_INPUT_DB_PIN_BASE + PIO_I80_INPUT_DB_PIN_COUNT)

#define MIPI_DCS_COLUMN_ADDRESS_VAL(xs, xe) \
	xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF

#define MIPI_DCS_ROW_ADDRESS_VAL(ys, ye) ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF

uint8_t txbuf[] = { 0x2A, MIPI_DCS_COLUMN_ADDRESS_VAL(0, 239),
		    0x2B, MIPI_DCS_ROW_ADDRESS_VAL(0, 239),
		    0x2C, 0x12,
		    0x34, 0x56,
		    0x78, 0x9A };

#define TX_BUF_SIZE (sizeof(txbuf) / sizeof(txbuf[0]))
#define RX_BUF_SIZE (64)

static uint8_t rd_buf[RX_BUF_SIZE] = { 0 };

static uint dma_rx;
static dma_channel_config c;

static inline void pio_clocked_input_rd8(PIO pio, uint sm, void *buf,
					 size_t len)
{
	dma_channel_configure(dma_rx, &c, (uint8_t *)buf, &pio->rxf[sm], len,
			      true);
}

#define CPU_SPEED_MHZ 240
int main()
{
	// #define CPU_SPEED_MHZ (DEFAULT_SYS_CLK_KHZ / 1000)
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

	printf("\n\n\nPico I80 Data Sniffer\n");

	// Load the clocked_input program, and configure a free state machine
	// to run the program.

	PIO pio = pio0;
	uint offset = pio_add_program(pio, &i80_input_program);
	uint sm = pio_claim_unused_sm(pio, true);
	i80_input_program_init(pio, sm, offset, 0, 8);

	dma_rx = dma_claim_unused_channel(true);
	c = dma_channel_get_default_config(dma_rx);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
	channel_config_set_write_increment(&c, true);
	channel_config_set_read_increment(&c, false);
	channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

	hexdump(txbuf, sizeof(txbuf));

	for (int i = 0; i < 5; i++) {
		memset(rd_buf, 0x00, sizeof(rd_buf));
		pio_clocked_input_rd8(pio, sm, rd_buf, sizeof(rd_buf));
		// hexdump(rd_buf, sizeof(rd_buf));
		analyzer_i80_8bit_video_sync(rd_buf, sizeof(rd_buf));
	}

	return 0;

	puts("Done.");
}
