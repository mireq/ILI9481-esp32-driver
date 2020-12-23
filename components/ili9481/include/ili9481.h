// SPDX-License-Identifier: MIT

#pragma once


#include "driver/spi_master.h"
#include "esp_err.h"


#define ILI9481_WRITE_QUEUE_SIZE 2

#define   SET_COL_ADDRESS                 0x2AU
#define   SET_PAGE_ADDRESS                0x2BU
#define   WRITE_MEMORY_START              0x2CU
#define   WRITE_MEMORY_CONTINUE           0x3CU
#define   READ_MEMORY_START               0x2EU
#define   SET_DISPLAY_OFF                 0x28U
#define   SET_DISPLAY_ON                  0x29U
#define   SET_ADDRESS_MODE                0x36U
#define   EXIT_INVERT_MODE                0x20U
#define   ENTER_INVERT_MODE               0x21U
#define   ENTER_NORMAL_MODE               0x13U //0 param
#define   EXIT_SLEEP_MODE                 0x11U
#define   SET_TEAR_OFF                    0x34U //0 param
#define   SET_TEAR_ON                     0x35U //1 param
#define   SET_PIXEL_FORMAT                0x3AU
#define   SET_TEAR_SCANLINE               0x44U //2 param
#define   FRAME_MEMORY_ACCESS_SETTING     0xB3U //4 param
#define   SET_DISPLAY_MODE                0xB4U //1 param
#define   PANEL_DRIVE_SETTING             0xC0U //6 param
#define   TIMING_SETTING_NORMAL           0xC1U //3 param
#define   TIMING_SETTING_PARTIAL          0xC2U //3 param
#define   FRAME_RATE_CONTROL              0xC5U //1 param
#define   INTERFACE_CONTROL               0xC6U //1 param
#define   POWER_SETTING                   0xD0U //3 param
#define   VCOM_CONTROL                    0xD1U //3 param
#define   POWER_SETTING_NORMAL            0xD2U //2 param
#define   POWER_SETTING_PARTIAL           0xD3U //2 param
#define   GAMMA_SETTING                   0xC8U //12 param


// System Function Command Table 1
//#define ILI9481_CMD_NOP               0x00 // No operation
//#define ILI9481_CMD_SWRESET           0x01 // Software reset
//#define ILI9481_CMD_RDDID             0x04 // Read display ID
//#define ILI9481_CMD_RDDST             0x09 // Read display status
//#define ILI9481_CMD_RDDPM             0x0a // Read display power
//#define ILI9481_CMD_RDDMADCTL         0x0b // Read display
//#define ILI9481_CMD_RDDCOLMOD         0x0c // Read display pixel
//#define ILI9481_CMD_RDDIM             0x0d // Read display image
//#define ILI9481_CMD_RDDSM             0x0e // Read display signal
//#define ILI9481_CMD_RDDSDR            0x0f // Read display self-diagnostic result
//#define ILI9481_CMD_SLPIN             0x10 // Sleep in
//#define ILI9481_CMD_SLPOUT            0x11 // Sleep out
//#define ILI9481_CMD_PTLON             0x12 // Partial mode on
//#define ILI9481_CMD_NORON             0x13 // Partial off (Normal)
//#define ILI9481_CMD_INVOFF            0x20 // Display inversion off
//#define ILI9481_CMD_INVON             0x21 // Display inversion on
//#define ILI9481_CMD_GAMSET            0x26 // Gamma set
//#define ILI9481_CMD_DISPOFF           0x28 // Display off
//#define ILI9481_CMD_DISPON            0x29 // Display on
//#define ILI9481_CMD_CASET             0x2a // Column address set
//#define ILI9481_CMD_RASET             0x2b // Row address set
//#define ILI9481_CMD_RAMWR             0x2c // Memory write
//#define ILI9481_CMD_RAMRD             0x2e // Memory read
//#define ILI9481_CMD_PTLAR             0x30 // Partial start/end address set
//#define ILI9481_CMD_VSCRDEF           0x33 // Vertical scrolling definition
//#define ILI9481_CMD_TEOFF             0x34 // Tearing line effect off
//#define ILI9481_CMD_TEON              0x35 // Tearing line effect on
//#define ILI9481_CMD_MADCTL            0x36 // Memory data access control
//#define ILI9481_CMD_VSCRSADD          0x37 // Vertical address scrolling
//#define ILI9481_CMD_IDMOFF            0x38 // Idle mode off
//#define ILI9481_CMD_IDMON             0x39 // Idle mode on
//#define ILI9481_CMD_COLMOD            0x3a // Interface pixel format
//#define ILI9481_CMD_RAMWRC            0x3c // Memory write continue
//#define ILI9481_CMD_RAMRDC            0x3e // Memory read continue
//#define ILI9481_CMD_TESCAN            0x44 // Set tear scanline
//#define ILI9481_CMD_RDTESCAN          0x45 // Get scanline
//#define ILI9481_CMD_WRDISBV           0x51 // Write display brightness
//#define ILI9481_CMD_RDDISBV           0x52 // Read display brightness value
//#define ILI9481_CMD_WRCTRLD           0x53 // Write CTRL display
//#define ILI9481_CMD_RDCTRLD           0x54 // Read CTRL value display
//#define ILI9481_CMD_WRCACE            0x55 // Write content adaptive brightness control and Color enhancemnet
//#define ILI9481_CMD_RDCABC            0x56 // Read content adaptive brightness control
//#define ILI9481_CMD_WRCABCMB          0x5e // Write CABC minimum brightness
//#define ILI9481_CMD_RDCABCMB          0x5f // Read CABC minimum brightness
//#define ILI9481_CMD_RDABCSDR          0x68 // Read Automatic Brightness Control Self-Diagnostic Result
//#define ILI9481_CMD_RDID1             0xda // Read ID1
//#define ILI9481_CMD_RDID2             0xdb // Read ID2
//#define ILI9481_CMD_RDID3             0xdc // Read ID3
//
//// System Function Command Table 2
//#define ILI9481_CMD_RAMCTRL           0xb0 // RAM Control
//#define ILI9481_CMD_RGBCTRL           0xb1 // RGB Control
//#define ILI9481_CMD_PORCTRL           0xb2 // Porch control
//#define ILI9481_CMD_FRCTRL1           0xb3 // Frame Rate Control 1
//#define ILI9481_CMD_GCTRL             0xb7 // Gate control
//#define ILI9481_CMD_DGMEN             0xba // Digital Gamma Enable
//#define ILI9481_CMD_VCOMS             0xbb // VCOM Setting
//#define ILI9481_CMD_LCMCTRL           0xc0 // LCM Control
//#define ILI9481_CMD_IDSET             0xc1 // ID Setting
//#define ILI9481_CMD_VDVVRHEN          0xc2 // VDV and VRH Command enable
//#define ILI9481_CMD_VRHSET            0xc3 // VRH Set
//#define ILI9481_CMD_VDVSET            0xc4 // VDV Set
//#define ILI9481_CMD_VCMOFSET          0xc5 // VCOM Offset Set
//#define ILI9481_CMD_FRCTR2            0xc6 // FR Control 2
//#define ILI9481_CMD_CABCCTRL          0xc7 // CABC Control
//#define ILI9481_CMD_REGSEL1           0xc8 // Register value selection 1
//#define ILI9481_CMD_REGSEL2           0xca // Register value selection 2
//#define ILI9481_CMD_PWMFRSEL          0xcc // PWM Frequency Selection
//#define ILI9481_CMD_PWCTRL1           0xd0 // Power Control 1
//#define ILI9481_CMD_VAPVANEN          0xd2 // Enable VAP/VAN signal output
//#define ILI9481_CMD_CMD2EN            0xdf // Command 2 Enable
//#define ILI9481_CMD_PVGAMCTRL         0xe0 // Positive Voltage Gamma Control
//#define ILI9481_CMD_NVGAMCTRL         0xe1 // Negative voltage Gamma Control
//#define ILI9481_CMD_DGMLUTR           0xe2 // Digital Gamma Look-up Table for Red
//#define ILI9481_CMD_DGMLUTB           0xe3 // Digital Gamma Look-up Table for Blue
//#define ILI9481_CMD_GATECTRL          0xe4 // Gate control
//#define ILI9481_CMD_PWCTRL2           0xe8 // Power Control 2
//#define ILI9481_CMD_EQCTRL            0xe9 // Equalize Time Control
//#define ILI9481_CMD_PROMCTRL          0xec // Program Control
//#define ILI9481_CMD_PROMEN            0xfa // Program Mode Enable
//#define ILI9481_CMD_NVMSET            0xfc // NVM Setting
//#define ILI9481_CMD_PROMACT           0xfe // Program Action
//
//#define ILI9481_CMDLIST_END           0xff // End command (used for command list)

struct ili9481_driver;

typedef struct ili9481_transaction_data {
	struct ili9481_driver *driver;
	bool data;
} ili9481_transaction_data_t;

typedef uint16_t ili9481_color_t;

typedef struct ili9481_driver {
	int pin_rst;
	int pin_rd;
	int pin_wr;
	int pin_cs;
	int pin_dc;
	int pin_d0;
	int pin_d1;
	int pin_d2;
	int pin_d3;
	int pin_d4;
	int pin_d5;
	int pin_d6;
	int pin_d7;
	int dma_chan;
	uint8_t queue_fill;
	uint16_t display_width;
	uint16_t display_height;
	//spi_device_handle_t spi;
	size_t buffer_size;
	ili9481_transaction_data_t data;
	ili9481_transaction_data_t command;
	ili9481_color_t *buffer;
	ili9481_color_t *buffer_a;
	ili9481_color_t *buffer_b;
	ili9481_color_t *current_buffer;
	//spi_transaction_t trans_a;
	//spi_transaction_t trans_b;
} ili9481_driver_t;

typedef struct ili9481_command {
	uint8_t command;
	uint8_t wait_ms;
	uint8_t data_size;
	const uint8_t *data;
} ili9481_command_t;

esp_err_t ili9481_init(ili9481_driver_t *driver);
void ili9481_reset(ili9481_driver_t *driver);
void ili9481_lcd_init(ili9481_driver_t *driver);
void ili9481_start_command(ili9481_driver_t *driver);
void ili9481_start_data(ili9481_driver_t *driver);
void ili9481_run_command(ili9481_driver_t *driver, const ili9481_command_t *command);
void ili9481_run_commands(ili9481_driver_t *driver, const ili9481_command_t *sequence);
void ili9481_clear(ili9481_driver_t *driver, ili9481_color_t color);
void ili9481_fill_area(ili9481_driver_t *driver, ili9481_color_t color, uint16_t start_x, uint16_t start_y, uint16_t width, uint16_t height);
void ili9481_fill_area(ili9481_driver_t *driver, ili9481_color_t color, uint16_t start_x, uint16_t start_y, uint16_t width, uint16_t height);
void ili9481_set_window(ili9481_driver_t *driver, uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);
void ili9481_write_pixels(ili9481_driver_t *driver, ili9481_color_t *pixels, size_t length);
void ili9481_wait_until_queue_empty(ili9481_driver_t *driver);
void ili9481_swap_buffers(ili9481_driver_t *driver);
/*
inline ili9481_color_t ili9481_rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
	return (((uint16_t)r >> 3) << 11) | (((uint16_t)g >> 2) << 5) | ((uint16_t)b >> 3);
}
*/
extern uint8_t ili9481_dither_table[];
void ili9481_randomize_dither_table();
#define ili9481_rgb_to_color(r, g, b) ((((ili9481_color_t)(b) >> 3) << 11) | (((ili9481_color_t)(g) >> 2) << 5) | ((ili9481_color_t)(r) >> 3))
inline ili9481_color_t __attribute__((always_inline)) ili9481_rgb_to_color_dither(uint8_t r, uint8_t g, uint8_t b, uint16_t x, uint16_t y) {
	const uint8_t pos = ((y << 8) + (y << 3) + x) & 0xff;
	uint8_t rand_b = ili9481_dither_table[pos];
	const uint8_t rand_r = rand_b & 0x07;
	rand_b >>= 3;
	const uint8_t rand_g = rand_b & 0x03;
	rand_b >>= 2;

	if (r < 249) {
		r = r + rand_r;
	}
	if (g < 253) {
		g = g + rand_g;
	}
	if (b < 249) {
		b = b + rand_b;
	}
	return ili9481_rgb_to_color(r, g, b);
}

inline void __attribute__((always_inline)) ili9481_color_to_rgb(ili9481_color_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
	*b = (color << 3);
	color >>= 5;
	color <<= 2;
	*g = color;
	color >>= 8;
	*r = color << 3;
}

//void ili9481_color_to_rgb(ili9481_color_t color, uint8_t *r, uint8_t *g, uint8_t *b);
//ili9481_color_t ili9481_rgb_to_color_dither(uint8_t r, uint8_t g, uint8_t b, uint16_t x, uint16_t y);
void ili9481_draw_gray2_bitmap(uint8_t *src_buf, ili9481_color_t *target_buf, uint8_t r, uint8_t g, uint8_t b, int x, int y, int src_w, int src_h, int target_w, int target_h);
