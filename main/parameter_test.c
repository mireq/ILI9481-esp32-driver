#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "esp32/rom/lldesc.h"
#include "esp32/rom/uart.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "hal/i2s_ll.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"

const char *TAG = "ili9481";


#define ILI9481_DISPLAY_WIDTH 320
#define ILI9481_DISPLAY_HEIGHT 480

#define CS_ACTIVE    gpio_set_level(driver->pin_cs, 0);
#define CS_IDLE      gpio_set_level(driver->pin_cs, 1);
#define CD_COMMAND   gpio_set_level(driver->pin_dc, 0);
#define CD_DATA      gpio_set_level(driver->pin_dc, 1);
#define RD_ACTIVE    gpio_set_level(driver->pin_rd, 0);
#define RD_IDLE      gpio_set_level(driver->pin_rd, 1);
#define WR_ACTIVE    gpio_set_level(driver->pin_wr, 0);
#define WR_IDLE      gpio_set_level(driver->pin_wr, 1);
#define RST_ACTIVE   gpio_set_level(driver->pin_rst, 0);
#define RST_IDLE     gpio_set_level(driver->pin_rst, 1);

#define ILI9481_NOP 0x00
#define ILI9481_SOFT_RESET 0x01
#define ILI9481_GET_RED_CHANNEL 0x06
#define ILI9481_GET_GREEN_CHANNEL 0x07
#define ILI9481_GET_BLUE_CHANNEL 0x08
#define ILI9481_GET_POWER_MODE 0x0A
#define ILI9481_GET_ADDRESS_MODE 0x0B
#define ILI9481_GET_PIXEL_FORMAT 0x0C
#define ILI9481_GET_DISPLAY_MODE 0x0D
#define ILI9481_GET_SIGNAL_MODE 0x0E
#define ILI9481_GET_DIAGNOSTIC_RESULT 0x0F
#define ILI9481_ENTER_SLEEP_MODE 0x10
#define ILI9481_EXIT_SLEEP_MODE 0x11
#define ILI9481_ENTER_PARTIAL_MODE 0x12
#define ILI9481_ENTER_NORMAL_MODE 0x13
#define ILI9481_EXIT_INVERT_MODE 0x20
#define ILI9481_ENTER_INVERT_MODE 0x21
#define ILI9481_SET_GAMMA_CURVE 0x26
#define ILI9481_SET_DISPLAY_OFF 0x28
#define ILI9481_SET_DISPLAY_ON 0x29
#define ILI9481_SET_COLUMN_ADDRESS 0x2A
#define ILI9481_SET_PAGE_ADDRESS 0x2B
#define ILI9481_WRITE_MEMORY_START 0x2C
#define ILI9481_WRITE_LUT 0x2D
#define ILI9481_READ_MEMORY_START 0x2E
#define ILI9481_SET_PARTIAL_AREA 0x30
#define ILI9481_SET_SCROLL_AREA 0x33
#define ILI9481_SET_TEAR_OFF 0x34
#define ILI9481_SET_TEAR_ON 0x35
#define ILI9481_SET_ADDRESS_MODE 0x36
#define ILI9481_SET_SCROLL_START 0x37
#define ILI9481_EXIT_IDLE_MODE 0x38
#define ILI9481_ENTER_IDLE_MODE 0x39
#define ILI9481_SET_PIXEL_FORMAT 0x3A
#define ILI9481_WRITE_MEMORY_CONTINUE 0x3C
#define ILI9481_READ_MEMORY_CONTINUE 0x3E
#define ILI9481_SET_TEAR_SCANLINE 0x44
#define ILI9481_GET_SCANLINE 0x45
#define ILI9481_READ_DDB_START 0xA1
#define ILI9481_COMMAND_ACCESS_PROTECT 0xB0
#define ILI9481_FRAME_MEMORY_ACCESS_SETTING 0xB3
#define ILI9481_DISPLAY_MODE 0xB4
#define ILI9481_DEVICE_CODE_READ 0xBF
#define ILI9481_PANEL_DRIVING_SETTING 0xC0
#define ILI9481_DISPLAY_TIMING_SETTING_NORMAL 0xC1
#define ILI9481_DISPLAY_TIMING_SETTING_PARTIAL 0xC2
#define ILI9481_DISPLAY_TIMING_SETTING_IDLE 0xC3
#define ILI9481_FRAME_RATE_CONTROL 0xC5
#define ILI9481_INTERFACE_CONTROL 0xC6
#define ILI9481_GAMMA_SETTING 0xC8
#define ILI9481_POWER_SETTING 0xD0
#define ILI9481_VCOM_CONTROL 0xD1
#define ILI9481_POWER_SETTING_NORMAL 0xD2
#define ILI9481_POWER_SETTING_PARTIAL 0xD3
#define ILI9481_POWER_SETTING_IDLE 0xD4
#define ILI9481_NV_MEMORY_WRITE 0xE0
#define ILI9481_NV_MEMORY_CONTROL 0xE1
#define ILI9481_NV_MEMORY_STATUS 0xE2
#define ILI9481_NV_MEMORY_PROTECTION 0xE3

#define ILI9481_CMDLIST_END 0x00


#define ili9481_rgb_to_color(r, g, b) ((r << 16) | (g << 8) | b)


typedef struct ili9481_driver {
	uint8_t pin_rst;
	uint8_t pin_rd;
	uint8_t pin_wr;
	uint8_t pin_cs;
	uint8_t pin_dc;
	uint8_t pin_d0;
	uint8_t pin_d1;
	uint8_t pin_d2;
	uint8_t pin_d3;
	uint8_t pin_d4;
	uint8_t pin_d5;
	uint8_t pin_d6;
	uint8_t pin_d7;
	uint16_t display_width;
	uint16_t display_height;
	uint32_t data_mask;
} ili9481_driver_t;


typedef struct ili9481_config {
	uint8_t kp0;
	uint8_t kp1;
	uint8_t kp2;
	uint8_t kp3;
	uint8_t kp4;
	uint8_t kp5;
	uint8_t rp0;
	uint8_t rp1;
	uint8_t vrp0;
	uint8_t vrp1;
	uint8_t kn0;
	uint8_t kn1;
	uint8_t kn2;
	uint8_t kn3;
	uint8_t kn4;
	uint8_t kn5;
	uint8_t rn0;
	uint8_t rn1;
	uint8_t vrn0;
	uint8_t vrn1;

	uint8_t vc;
	uint8_t bt;
	uint8_t vrh;
	uint8_t vcm;
	uint8_t vdv;

	uint8_t ap0;
	uint8_t dc00;
	uint8_t dc10;

	uint8_t bc0;
	uint8_t div0;
	uint8_t rtn0;
	uint8_t fp0;
	uint8_t bp0;

	uint8_t fra;
} ili9481_config_t;

typedef struct ili9481_command {
	uint8_t command;
	uint8_t data_size;
	const uint8_t *data;
} ili9481_command_t;


static inline void __attribute__((always_inline)) write_bits(ili9481_driver_t *driver, uint8_t data) {
	/*
	gpio_set_level(driver->pin_wr, 0);
	gpio_set_level(driver->pin_d0, ((data >> 0) & 0x01));
	gpio_set_level(driver->pin_d1, ((data >> 1) & 0x01));
	gpio_set_level(driver->pin_d2, ((data >> 2) & 0x01));
	gpio_set_level(driver->pin_d3, ((data >> 3) & 0x01));
	gpio_set_level(driver->pin_d4, ((data >> 4) & 0x01));
	gpio_set_level(driver->pin_d5, ((data >> 5) & 0x01));
	gpio_set_level(driver->pin_d6, ((data >> 6) & 0x01));
	gpio_set_level(driver->pin_d7, ((data >> 7) & 0x01));
	gpio_set_level(driver->pin_wr, 1);
	*/

	uint32_t out_data = 0;
	out_data |= ((data >> 0) & 0x01) << driver->pin_d0;
	out_data |= ((data >> 1) & 0x01) << driver->pin_d1;
	out_data |= ((data >> 2) & 0x01) << driver->pin_d2;
	out_data |= ((data >> 3) & 0x01) << driver->pin_d3;
	out_data |= ((data >> 4) & 0x01) << driver->pin_d4;
	out_data |= ((data >> 5) & 0x01) << driver->pin_d5;
	out_data |= ((data >> 6) & 0x01) << driver->pin_d6;
	out_data |= ((data >> 7) & 0x01) << driver->pin_d7;

	REG_WRITE(GPIO_OUT_W1TS_REG, out_data);
	out_data = (out_data ^ driver->data_mask);
	REG_WRITE(GPIO_OUT_W1TC_REG, out_data | (1 << driver->pin_wr));
	REG_WRITE(GPIO_OUT_W1TS_REG, (1 << driver->pin_wr));
}


static uint8_t read_bits(ili9481_driver_t *driver) {
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = (
		(1ULL << driver->pin_d0) |
		(1ULL << driver->pin_d1) |
		(1ULL << driver->pin_d2) |
		(1ULL << driver->pin_d3) |
		(1ULL << driver->pin_d4) |
		(1ULL << driver->pin_d5) |
		(1ULL << driver->pin_d6) |
		(1ULL << driver->pin_d7)
	);
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	uint8_t data = 0;

	RD_ACTIVE
	data |= gpio_get_level(driver->pin_d0) << 0;
	data |= gpio_get_level(driver->pin_d1) << 1;
	data |= gpio_get_level(driver->pin_d2) << 2;
	data |= gpio_get_level(driver->pin_d3) << 3;
	data |= gpio_get_level(driver->pin_d4) << 4;
	data |= gpio_get_level(driver->pin_d5) << 5;
	data |= gpio_get_level(driver->pin_d6) << 6;
	data |= gpio_get_level(driver->pin_d7) << 7;
	RD_IDLE

	io_conf.mode = GPIO_MODE_OUTPUT;
	gpio_config(&io_conf);

	return data;
}


static void write_command(ili9481_driver_t *driver, uint8_t command) {
	CD_COMMAND;
	write_bits(driver, command);
	CD_DATA;
}


static inline void __attribute__((always_inline)) write_data_8(ili9481_driver_t *driver, uint8_t data) {
	write_bits(driver, data);
}


static inline void __attribute__((always_inline)) write_data_24(ili9481_driver_t *driver, uint32_t data) {
	write_bits(driver, (data & 0x00FF));
	write_bits(driver, ((data >> 8) & 0x00FF));
	write_bits(driver, ((data >> 16) & 0x00FF));
}


static uint8_t read_data_8(ili9481_driver_t *driver) {
	CD_DATA;
	uint8_t data = read_bits(driver);
	return data;
}

static void execute_commands(ili9481_driver_t *driver, ili9481_command_t *sequence) {
	while (sequence->command != ILI9481_CMDLIST_END) {
		write_command(driver, sequence->command);
		for (uint8_t i = 0; i < sequence->data_size; ++i) {
			write_data_8(driver, sequence->data[i]);
		}
		sequence++;
	}
}


static void set_addr_window(ili9481_driver_t *driver, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	write_command(driver, ILI9481_SET_COLUMN_ADDRESS);
	write_data_8(driver, x0 >> 8);
	write_data_8(driver, x0 & 0xFF);
	write_data_8(driver, x1 >> 8);
	write_data_8(driver, x1 & 0xFF);

	write_command(driver, ILI9481_SET_PAGE_ADDRESS);
	write_data_8(driver, y0 >> 8);
	write_data_8(driver, y0);
	write_data_8(driver, y1 >> 8);
	write_data_8(driver, y1);

	write_command(driver, ILI9481_WRITE_MEMORY_START);
}


esp_err_t ili9481_init(ili9481_driver_t *driver) {
	driver->data_mask = 0x00000000;
	driver->data_mask |= (1 << driver->pin_d0);
	driver->data_mask |= (1 << driver->pin_d1);
	driver->data_mask |= (1 << driver->pin_d2);
	driver->data_mask |= (1 << driver->pin_d3);
	driver->data_mask |= (1 << driver->pin_d4);
	driver->data_mask |= (1 << driver->pin_d5);
	driver->data_mask |= (1 << driver->pin_d6);
	driver->data_mask |= (1 << driver->pin_d7);

	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = (
		(1ULL << driver->pin_rst) |
		(1ULL << driver->pin_rd) |
		(1ULL << driver->pin_wr) |
		(1ULL << driver->pin_cs) |
		(1ULL << driver->pin_dc) |
		(1ULL << driver->pin_d0) |
		(1ULL << driver->pin_d1) |
		(1ULL << driver->pin_d2) |
		(1ULL << driver->pin_d3) |
		(1ULL << driver->pin_d4) |
		(1ULL << driver->pin_d5) |
		(1ULL << driver->pin_d6) |
		(1ULL << driver->pin_d7)
	);
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	ESP_LOGI(TAG, "GPIO initialized");

	RD_IDLE
	WR_IDLE
	CD_DATA
	CS_ACTIVE
	RST_ACTIVE
	vTaskDelay(100 / portTICK_PERIOD_MS);
	RST_IDLE
	vTaskDelay(100 / portTICK_PERIOD_MS);
	write_command(driver, ILI9481_EXIT_SLEEP_MODE);
	vTaskDelay(120 / portTICK_PERIOD_MS);

	ESP_LOGI(TAG, "Display initialized");

	return ESP_OK;
}


static void write_init_message(ili9481_driver_t *driver) {
	printf("ILI9881 initialized, device code is:");

	uint8_t data;
	write_command(driver, 0xBF);
	data = read_data_8(driver);
	data = read_data_8(driver);
	printf(" %02x", data);
	data = read_data_8(driver);
	printf(" %02x", data);
	data = read_data_8(driver);
	printf(" %02x", data);
	data = read_data_8(driver);
	printf(" %02x", data);
	data = read_data_8(driver);
	printf(" %02x\n", data);
}


static void configure_display(ili9481_driver_t *driver, ili9481_config_t *config) {
	//write_command(driver, ILI9481_SET_DISPLAY_OFF);

	write_command(driver, ILI9481_POWER_SETTING);
	write_data_8(driver, config->vc);
	write_data_8(driver, 0x40U | config->bt); // PON=1
	write_data_8(driver, 0x10U | config->vrh); // VCIRE=1

	write_command(driver, ILI9481_VCOM_CONTROL);
	write_data_8(driver, 0x00U); // settings from VCOM_CONTROL
	write_data_8(driver, config->vcm);
	write_data_8(driver, config->vdv);

	write_command(driver, ILI9481_POWER_SETTING_NORMAL);
	write_data_8(driver, config->ap0);
	write_data_8(driver, config->dc10 | (config->dc00 << 4));

	write_command(driver, ILI9481_DISPLAY_TIMING_SETTING_NORMAL);
	write_data_8(driver, config->div0 | (config->bc0 << 4));
	write_data_8(driver, config->rtn0);
	write_data_8(driver, config->bp0 | (config->fp0 << 4));

	write_command(driver, ILI9481_PANEL_DRIVING_SETTING);
	write_data_8(driver, 0x00U); // REV=0, SM=0, GS=0
	write_data_8(driver, 0x3BU); // NL=480 (lines to drive)
	write_data_8(driver, 0x00U); // SCN (scanning start position)
	write_data_8(driver, 0x02U); // NDL (non-display area o/p level), PTS[3]

	write_data_8(driver, 0x11); // PTG=1 (interval scan), ISC[3]=0001 (3 frames)

	write_command(driver, ILI9481_FRAME_RATE_CONTROL);
	write_data_8(driver, config->fra);

	write_command(driver, ILI9481_GAMMA_SETTING); //gamma setting
	write_data_8(driver, config->kp0 | (config->kp1 << 4));
	write_data_8(driver, config->kp2 | (config->kp3 << 4));
	write_data_8(driver, config->kp4 | (config->kp5 << 4));
	write_data_8(driver, config->rp0 | (config->rp1 << 4));
	write_data_8(driver, config->vrp0);
	write_data_8(driver, config->vrp1);
	write_data_8(driver, config->kn0 | (config->kn1 << 4));
	write_data_8(driver, config->kn2 | (config->kn3 << 4));
	write_data_8(driver, config->kn4 | (config->kn5 << 4));
	write_data_8(driver, config->rn0 | (config->rn1 << 4));
	write_data_8(driver, config->vrn0);
	write_data_8(driver, config->vrn1);

	write_command(driver, ILI9481_SET_PIXEL_FORMAT);
	write_data_8(driver, 0x66); //18-bit per pixel

	write_command(driver, ILI9481_SET_ADDRESS_MODE);
	write_data_8(driver, 0x40); // not mirror

	write_command(driver, ILI9481_SET_DISPLAY_ON);
	//write_command(driver, ILI9481_ENTER_INVERT_MODE);

	set_addr_window(driver, 0, 0,  driver->display_width - 1, driver->display_height - 1);
}


static uint8_t htoi(int c) {
	switch (c) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a': return 10;
		case 'b': return 11;
		case 'c': return 12;
		case 'd': return 13;
		case 'e': return 14;
		case 'f': return 15;
		default: return 0xff;
	}
}


static void transfer_image(ili9481_driver_t *driver) {
	size_t pixels_count = driver->display_height * driver->display_width;
	size_t current_pixel = 0;
	STATUS s;
	uint8_t color[3] = {0, 0, 0};
	int component = 0;
	int top = 0;
	uint8_t c;
	while (1) {
		s = uart_rx_one_char(&c);
		if (s != OK) {
			continue;
		}
		if (c == 0xff) {
			c = 0x00;
		}
		if (top) {
			color[component] |= htoi(c);
		}
		else {
			color[component] = (htoi(c) << 4);
		}
		if (component == 2 && top == 1) {
			write_data_24(driver, ili9481_rgb_to_color(color[0], color[1], color[2]));
			color[0] = 0;
			color[1] = 0;
			color[2] = 0;
			current_pixel++;
			if (current_pixel >= pixels_count) {
				return;
			}
		}
		if (top == 1) {
			component = (component + 1) % 3;
			top = 0;
		}
		else {
			top = 1;
		}
	}
}


#define NUM_PATTERNS 5


static void draw_vertical_gradient(ili9481_driver_t *driver, int x, int y) {
	int col = y * 255 / driver->display_height;
	write_data_24(driver, ili9481_rgb_to_color(col, col, col));
}

static void draw_horizontal_gradient(ili9481_driver_t *driver, int x, int y) {
	int col = x * 255 / driver->display_width;
	write_data_24(driver, ili9481_rgb_to_color(col, col, col));
}

static void draw_combined_gradient(ili9481_driver_t *driver, int x, int y) {
	int col;
	if (x < 160) {
		col = (x * 255 / (driver->display_width / 2)) & 0xff;
	}
	else {
		col = y * 255 / driver->display_height;
	}
	write_data_24(driver, ili9481_rgb_to_color(col, col, col));
}

static void draw_combined_gradient2(ili9481_driver_t *driver, int x, int y) {
	int col;
	if (x < 160) {
		col = (x * 255 / (driver->display_width / 2)) & 0xff;
	}
	else {
		if (y % 8 < 4) {
			col = 0;
		}
		else {
			col = y * 255 / driver->display_height;
		}
	}
	write_data_24(driver, ili9481_rgb_to_color(col, col, col));
}


static void draw_rainbow(ili9481_driver_t *driver, int x, int y) {
	int r = ((320 - abs(y - 0)) * 480) / driver->display_height;
	int g = ((320 - abs(y - 240)) * 480) / driver->display_height;
	int b = ((320 - abs(y - 480)) * 480) / driver->display_height;
	if (r > 255) { r = 255; }
	if (r < 0) { r = 0; }
	if (g > 255) { g = 255; }
	if (g < 0) { g = 0; }
	if (b > 255) { b = 255; }
	if (b < 0) { b = 0; }
	write_data_24(driver, ili9481_rgb_to_color(r, g, b));
}

static void draw_pattern(ili9481_driver_t *driver, int pattern) {
	for(int y=0; y < driver->display_height; y++) {
		for(int x=0; x < driver->display_width; x++) {
			switch (pattern) {
				case 0:
					draw_vertical_gradient(driver, x, y);
					break;
				case 1:
					draw_horizontal_gradient(driver, x, y);
					break;
				case 2:
					draw_combined_gradient(driver, x, y);
					break;
				case 3:
					draw_combined_gradient2(driver, x, y);
					break;
				case 4:
					draw_rainbow(driver, x, y);
					break;
			}
		}
	}
}


static void main_loop(ili9481_driver_t *driver, ili9481_config_t *config) {
	configure_display(driver, config);

	int pattern = 0;
	draw_pattern(driver, pattern);
	STATUS s;
	while (1) {
		uint8_t c;
		s = uart_rx_one_char(&c);
		if (s == OK) {
			int configure = 1;
			switch (c) {
				case 'q':
					config->kp0 = (config->kp0 + 1) % 8;
					break;
				case 'w':
					config->kp1 = (config->kp1 + 1) % 8;
					break;
				case 'e':
					config->kp2 = (config->kp2 + 1) % 8;
					break;
				case 'r':
					config->kp3 = (config->kp3 + 1) % 8;
					break;
				case 't':
					config->kp4 = (config->kp4 + 1) % 8;
					break;
				case 'y':
					config->kp5 = (config->kp5 + 1) % 8;
					break;
				case 'u':
					config->rp0 = (config->rp0 + 1) % 8;
					break;
				case 'i':
					config->rp1 = (config->rp1 + 1) % 8;
					break;
				case 'a':
					config->kn0 = (config->kn0 + 1) % 8;
					break;
				case 's':
					config->kn1 = (config->kn1 + 1) % 8;
					break;
				case 'd':
					config->kn2 = (config->kn2 + 1) % 8;
					break;
				case 'f':
					config->kn3 = (config->kn3 + 1) % 8;
					break;
				case 'g':
					config->kn4 = (config->kn4 + 1) % 8;
					break;
				case 'h':
					config->kn5 = (config->kn5 + 1) % 8;
					break;
				case 'z':
					config->vrp0 = (config->vrp0 + 1) % 16;
					break;
				case 'x':
					config->vrp1 = (config->vrp1 + 1) % 32;
					break;
				case 'c':
					config->vrn0 = (config->vrn0 + 1) % 16;
					break;
				case 'v':
					config->vrn1 = (config->vrn1 + 1) % 32;
					break;
				case 'o':
					config->vcm = (config->vcm + 1) % 64;
					break;
				case 'p':
					config->vdv = (config->vdv + 1) % 32;
					break;
				case 'b':
					config->vc = (config->vc + 1) % 8;
					break;
				case 'n':
					config->bt = (config->bt + 1) % 8;
					break;
				case 'm':
					config->vrh = (config->vrh + 1) % 16;
					break;
				case 'Q':
					config->ap0 = (config->ap0 + 1) % 8;
					break;
				case 'W':
					config->dc10 = (config->dc10 + 1) % 8;
					break;
				case 'E':
					config->dc00 = (config->dc00 + 1) % 8;
					break;
				case 'P':
					config->fra = (config->fra + 1) % 8;
					break;
				case 'A':
					config->bc0 = (config->bc0 + 1) % 2;
					break;
				case 'S':
					config->div0 = (config->div0 + 1) % 4;
					break;
				case 'D':
					config->rtn0 = (config->rtn0 + 1) % 32;
					break;
				case 'F':
					config->fp0 = (config->fp0 + 1) % 16;
					break;
				case 'G':
					config->bp0 = (config->bp0 + 1) % 16;
					break;
				case 'U':
					transfer_image(driver);
					configure = 0;
					break;
				case 'X':
					pattern = (pattern + 1) % NUM_PATTERNS;
					configure = 0;
					draw_pattern(driver, pattern);
					break;
				default:
					printf("%d\n", c);
					break;
			}
			if (configure) {
				printf("kp0=%x, kp1=%x, kp2=%x, kp3=%x, kp4=%x, kp5=%x, rp0=%x, rp1=%x, vrp0=%x, vrp1=%x\nkn0=%x, kn1=%x, kn2=%x, kn3=%x, kn4=%x, kn5=%x, rn0=%x, rn1=%x, vrn0=%x, vrn1=%x\nvc=%x, bt=%x, vrh=%x, vcm=%x, vdv=%x, ap0=%x, dc00=%x, dc10=%x\nfra=%x, bc0=%x, div0=%x, rtn0=%x, fp0=%x, bp0=%x\n\n", config->kp0, config->kp1, config->kp2, config->kp3, config->kp4, config->kp5, config->rp0, config->rp1, config->vrp0, config->vrp1, config->kn0, config->kn1, config->kn2, config->kn3, config->kn4, config->kn5, config->rn0, config->rn1, config->vrn0, config->vrn1, config->vc, config->bt, config->vrh, config->vcm, config->vdv, config->ap0, config->dc00, config->dc10, config->fra, config->bc0, config->div0, config->rtn0, config->fp0, config->bp0);
				configure_display(driver, config);
			}
		}
	}
}


static void i2s_reset_fifo(i2s_dev_t *dev) {
	dev->conf.tx_reset = 1;
	dev->conf.tx_fifo_reset = 1;
	dev->conf.rx_fifo_reset = 1;
	dev->conf.tx_reset = 0;
	dev->conf.tx_fifo_reset = 0;
	dev->conf.rx_fifo_reset = 0;
}


static void i2s_configure_dma(i2s_dev_t *dev) {
	dev->lc_conf.in_rst = 1;
	dev->lc_conf.out_rst = 1;
	dev->lc_conf.ahbm_rst = 1;
	dev->lc_conf.ahbm_fifo_rst = 1;
	dev->lc_conf.in_rst = 0;
	dev->lc_conf.out_rst = 0;
	dev->lc_conf.ahbm_rst = 0;
	dev->lc_conf.ahbm_fifo_rst = 0;

	dev->lc_conf.check_owner = 1;
	dev->lc_conf.out_data_burst_en = 1;
	dev->lc_conf.outdscr_burst_en = 1;
}


static void i2s_set_lcd_mode(i2s_dev_t *dev) {
	dev->conf2.val = 0;
	dev->conf2.lcd_en = 1;
	dev->conf2.lcd_tx_wrx2_en = 1;
	dev->conf2.lcd_tx_sdx2_en = 0;
}


static void i2s_set_speed(i2s_dev_t *dev) {
	dev->sample_rate_conf.val = 0;
	dev->sample_rate_conf.rx_bits_mod = 4;
	dev->sample_rate_conf.tx_bits_mod = 4;
	dev->sample_rate_conf.rx_bck_div_num = 2;
	dev->sample_rate_conf.tx_bck_div_num = 2;

	dev->clkm_conf.val = 0;
	dev->clkm_conf.clka_en = 0;
	dev->clkm_conf.clkm_div_a = 63;
	dev->clkm_conf.clkm_div_b = 0;
	dev->clkm_conf.clkm_div_num = 8;
	dev->clkm_conf.clk_en = 1;

	dev->timing.val = 0;
}


static void i2s_configure_tx(i2s_dev_t *dev) {
	dev->fifo_conf.val = 0;
	dev->fifo_conf.rx_fifo_mod_force_en = 1;
	dev->fifo_conf.tx_fifo_mod_force_en = 1;
	dev->fifo_conf.rx_fifo_mod = 1;
	dev->fifo_conf.tx_fifo_mod = 1;
	dev->fifo_conf.dscr_en = 1;

	dev->conf1.val = 0;
	dev->conf1.tx_stop_en = 1;
	dev->conf1.tx_pcm_bypass = 1;

	dev->conf_chan.val = 0;
	dev->conf_chan.tx_chan_mod = 2;
	dev->conf_chan.rx_chan_mod = 2;
}


struct i2s_transaction_t;
typedef void(*transaction_cb_t)(struct i2s_transaction_t *trans);

typedef struct {
	int queue_size;
	transaction_cb_t pre_cb;
} i2s_driver_config_t;

typedef struct {
	QueueHandle_t tx_queue;
	SemaphoreHandle_t tx_semaphore;
	transaction_cb_t pre_cb;
	intr_handle_t intr_handle;
	lldesc_t *dma;
	i2s_dev_t *hw;
} i2s_driver_t;

typedef struct {
	void *data;
	size_t length;
	void *user_data;
} i2s_transaction_t;

static i2s_driver_t i2s_drivers[2] = {
	{
		.tx_queue = 0,
		.tx_semaphore = 0,
		.pre_cb = NULL,
		.intr_handle = NULL,
		.dma = NULL,
		.hw = &I2S0
	},
	{
		.tx_queue = 0,
		.tx_semaphore = 0,
		.pre_cb = NULL,
		.intr_handle = NULL,
		.dma = NULL,
		.hw = &I2S1
	}
};


uint8_t *buf = NULL;

static void IRAM_ATTR i2s_isr(void *const params) {
	const i2s_driver_t *drv = (i2s_driver_t *)(params);
	i2s_dev_t *dev = drv->hw;
	int done = 0;
	//ESP_EARLY_LOGE(TAG, "interrupt status: 0x%08x", dev->int_st.val);

    if (dev->int_st.out_eof || dev->int_st.out_total_eof)
    {
    	 		while (!dev->state.tx_idle);

        dev->conf.tx_start = 0;
        dev->conf.tx_reset = 1;
        dev->conf.tx_reset = 0;
        done = 1;

    }

	dev->int_clr.val = dev->int_st.val;

	if (done) {
		dev->out_link.start = 1;
		dev->conf.tx_start = 1;
	}

		/*
	if (dev->int_st.out_total_eof) {
		ESP_EARLY_LOGE(TAG, "interrupt status: 0x%08x", dev->int_st.val);
		dev->int_clr.val = dev->int_st.val;
		while (!dev->state.tx_idle);

		dma_descriptor->size = 320*3;
		dma_descriptor->length = 320*3;
		dma_descriptor->buf = buf;
		dma_descriptor->owner = 1;
		dma_descriptor->sosf = 0;
		dma_descriptor->eof = 1;
		dma_descriptor->empty = 0;
		dma_descriptor->qe.stqe_next = NULL;

		dev->lc_conf.in_rst = 1;
		dev->lc_conf.out_rst = 1;
		dev->lc_conf.ahbm_rst = 1;
		dev->lc_conf.ahbm_fifo_rst = 1;
		dev->lc_conf.in_rst = 0;
		dev->lc_conf.out_rst = 0;
		dev->lc_conf.ahbm_rst = 0;
		dev->lc_conf.ahbm_fifo_rst = 0;

		dev->conf.tx_reset = 1;
		dev->conf.tx_reset = 0;

		dev->lc_conf.check_owner = 1;
		dev->lc_conf.out_data_burst_en = 1;
		dev->lc_conf.outdscr_burst_en = 1;

		dev->out_link.addr = (uint32_t)dma_descriptor;

		dev->fifo_conf.dscr_en = 1;
		dev->out_link.start = 1;
		dev->conf.tx_start = 1;
	}
	uint32_t status = dev->int_st.val;
	if (status == 0) {
		return;
	}


	dev->int_clr.val = dev->int_st.val;

	if (status == BIT(16)) {
		I2S1.out_link.start = 1;
		I2S1.conf.tx_start = 1;
	}
	else {
		ESP_EARLY_LOGE(TAG, "interrupt status: 0x%08x", status);
	}
	I2S1.int_clr.val = I2S1.int_st.val;
	esp_intr_disable(i2s_drivers[1].intr_handle);

	I2S1.conf.tx_start = 0;


	esp_intr_enable(i2s_drivers[1].intr_handle);

	dma_descriptor->size = 320*3;
	dma_descriptor->length = 320*3;
	dma_descriptor->buf = buf;
	dma_descriptor->owner = 1;
	dma_descriptor->sosf = 0;
	dma_descriptor->eof = 1;
	dma_descriptor->qe.stqe_next = NULL;
	I2S1.out_link.addr = (uint32_t)dma_descriptor;

	*/
	//I2S1.out_link.start = 1;
	//I2S1.conf.tx_start = 1;
}

static esp_err_t i2s_init(i2s_dev_t *dev, i2s_driver_config_t *config) {
	int dev_num = (dev == &I2S0) ? 0 : 1;
	i2s_driver_t *drv = &(i2s_drivers[dev_num]);
	drv->pre_cb = config->pre_cb;

	drv->tx_queue = NULL;
	drv->tx_semaphore = NULL;
	drv->dma = NULL;

	drv->tx_queue = xQueueCreate(config->queue_size, sizeof(i2s_transaction_t));
	if (drv->tx_queue == NULL) {
		ESP_LOGE(TAG, "I2S queue not allocated");
		goto cleanup;
	}
	drv->tx_semaphore = xSemaphoreCreateBinary();
	if (drv->tx_semaphore == NULL) {
		ESP_LOGE(TAG, "I2S semaphore not allocated");
		goto cleanup;
	}

	drv->dma = (lldesc_t *)heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA);
	if (drv->dma == NULL) {
		ESP_LOGE(TAG, "I2S dma not allocated");
		goto cleanup;
	}

	return ESP_OK;

cleanup:
	if (drv->dma != NULL) {
		heap_caps_free(drv->dma);
		drv->dma = NULL;
	}
	if (drv->tx_semaphore != NULL) {
		vSemaphoreDelete(drv->tx_semaphore);
		drv->tx_semaphore = NULL;
	}
	if (drv->tx_queue != NULL) {
		vSemaphoreDelete(drv->tx_queue);
		drv->tx_queue = NULL;
	}
	return ESP_FAIL;
}


static esp_err_t i2s_intr_init(i2s_dev_t *dev) {
	int dev_num = (dev == &I2S0) ? 0 : 1;
	i2s_driver_t *drv = &(i2s_drivers[dev_num]);
	const esp_err_t ret = esp_intr_alloc(
		dev_num == 0 ? ETS_I2S0_INTR_SOURCE : ETS_I2S1_INTR_SOURCE,
		0,
		i2s_isr,
		drv,
		&drv->intr_handle
	);
	if (ret == ESP_OK) {
		//esp_intr_set_in_iram(drv->intr_handle, true);
		esp_intr_enable(drv->intr_handle);
	}
	else {
		ESP_LOGE(TAG, "Interrupt handler not allocated");
		return ESP_FAIL;
	}

	dev->int_ena.out_total_eof = 1;
	//dev->int_ena.out_dscr_err = 1;
	//dev->int_ena.out_eof = 1;
	//dev->int_ena.out_done = 1;

	//dev->int_ena.in_err_eof = 1;
	//dev->int_ena.out_eof = 1;
	//dev->int_ena.out_dscr_err = 1;
	//dev->int_ena.out_eof = 1;
	return ESP_OK;
}

static void i2s_trans_dma_start(i2s_driver_t *drv, i2s_transaction_t *transaction) {
	i2s_dev_t *dev = drv->hw;


	/*
	dev->conf.tx_reset = 1;
	dev->conf.tx_fifo_reset = 1;
	dev->conf.rx_fifo_reset = 1;
	dev->conf.tx_reset = 0;
	dev->conf.tx_fifo_reset = 0;
	dev->conf.rx_fifo_reset = 0;
	*/

	//i2s_configure_dma(dev);
	//i2s_set_lcd_mode(dev);
	//i2s_set_speed(dev);
	//i2s_configure_tx(dev);

	drv->dma->size = 320*3;
	drv->dma->length = 320*3;
	drv->dma->buf = buf;
	drv->dma->owner = 1;
	drv->dma->sosf = 0;
	drv->dma->eof = 1;
	drv->dma->empty = 0;
	drv->dma->qe.stqe_next = NULL;

	//dev->out_link.addr = 0;
	dev->out_link.addr = ((uint32_t)drv->dma) & 0xfffff;
	dev->out_link.start = 1;
	dev->conf.tx_start = 1;
}

static void i2s_trans_enqueue(i2s_dev_t *dev, i2s_transaction_t *transaction) {
	int dev_num = (dev == &I2S0) ? 0 : 1;
	i2s_driver_t *drv = &(i2s_drivers[dev_num]);
	i2s_trans_dma_start(drv, transaction);
	//xQueueSend(drv->tx_queue, transaction, portMAX_DELAY);
}

int line = 0;

	/*
	line++;
	if (line < 10) {
		i2s_dev_t *dev = &I2S1;
		dev->out_link.start = 1;
		dev->conf.tx_start = 1;
	}
	*/

static void draw_dma_pattern(ili9481_driver_t *driver) {
	i2s_driver_config_t config = {
		.queue_size = 2,
		.pre_cb = NULL
	};
	i2s_init(&I2S1, &config);

	const int sig_data_base = I2S1O_DATA_OUT0_IDX;
	const int sig_wr = I2S1O_WS_OUT_IDX;
	i2s_dev_t *dev = &I2S1;

	gpio_matrix_out(driver->pin_d0, sig_data_base+0, false, false);
	gpio_matrix_out(driver->pin_d1, sig_data_base+1, false, false);
	gpio_matrix_out(driver->pin_d2, sig_data_base+2, false, false);
	gpio_matrix_out(driver->pin_d3, sig_data_base+3, false, false);
	gpio_matrix_out(driver->pin_d4, sig_data_base+4, false, false);
	gpio_matrix_out(driver->pin_d5, sig_data_base+5, false, false);
	gpio_matrix_out(driver->pin_d6, sig_data_base+6, false, false);
	gpio_matrix_out(driver->pin_d7, sig_data_base+7, false, false);

	periph_module_enable(PERIPH_I2S1_MODULE);

	i2s_reset_fifo(dev);
	i2s_set_lcd_mode(dev);
	i2s_set_speed(dev);
	i2s_configure_tx(dev);

	buf = (uint8_t *)heap_caps_malloc(320*3, MALLOC_CAP_DMA);
	for (size_t i = 0; i < 320; ++i) {
		buf[i*3] = (i < 107) ? (i * 256 / 107) : 0;
		buf[i*3+1] = (i < 107) ? 0 : (i < 214 ? (((i-107) * 256 / 107)) : 0);
		buf[i*3+2] = (i < 214) ? 0 : ((i-106) * 256 / 106);
	}
	/*
	int pos = 0;
	buf[pos+0] = 255;
	buf[pos+1] = 255;
	buf[pos+2] = 255;
	pos = 107*3;
	buf[pos+0] = 255;
	buf[pos+1] = 255;
	buf[pos+2] = 255;
	pos = 214*3;
	buf[pos+0] = 255;
	buf[pos+1] = 255;
	buf[pos+2] = 255;
	pos = 319*3;
	buf[pos+0] = 255;
	buf[pos+1] = 255;
	buf[pos+2] = 255;
	*/

	uint32_t *rotate_buf = (uint32_t *)buf;
	for (size_t i = 0; i < 320*3/4; ++i) {
		rotate_buf[i] = (rotate_buf[i] >> 16) | (rotate_buf[i] << 16);
	}

	/*
	dma_descriptor = (lldesc_t *)heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA);
	dma_descriptor->size = 320*3;
	dma_descriptor->length = 320*3;
	dma_descriptor->buf = buf;
	dma_descriptor->owner = 1;
	dma_descriptor->sosf = 0;
	dma_descriptor->eof = 1;
	dma_descriptor->empty = 0;
	dma_descriptor->qe.stqe_next = NULL;
	*/

	i2s_intr_init(&I2S1);
	i2s_configure_dma(dev);

	//dev->out_link.addr = (uint32_t)dma_descriptor;

	gpio_matrix_out(driver->pin_wr, sig_wr, true, false);

	i2s_transaction_t trans = {
		.data = buf,
		.length = 320*3,
		.user_data = NULL
	};
	i2s_trans_enqueue(&I2S1, &trans);

	/*
	while (1) {
		vTaskDelay(1);
		i2s_trans_enqueue(&I2S1, &trans);
		//printf("ook\n");
	}
	*/

	/*
	while (1) {
		printf("queue\n");
	}
	*/

	//esp_intr_enable(i2s_drivers[1].intr_handle);

	/*
	dev->out_link.start = 1;
	dev->conf.tx_start = 1;

	dma_descriptor->eof = 0;
	dma_descriptor->qe.stqe_next = NULL;
	*/
	/*
	while(1) {
	}
	*/
	//vTaskDelay(10);
	while (1) {
		vTaskDelay(50);
		for (size_t i = 0; i < 320*3/4; ++i) {
			rotate_buf[i] = (rotate_buf[i] >> 16) | (rotate_buf[i] << 16);
		}
	}

	printf("done\n");
	vTaskDelay(portMAX_DELAY);
}


static void parameter_test(ili9481_driver_t *driver, ili9481_command_t *commands) {
	execute_commands(driver, commands);
	set_addr_window(driver, 0, 0,  driver->display_width - 1, driver->display_height - 1);

	draw_dma_pattern(driver);
	//draw_pattern(driver, 0);
}


void app_main(void) {
	ili9481_driver_t display = {
		.pin_rst = GPIO_NUM_23,
		.pin_rd = GPIO_NUM_32,
		.pin_wr = GPIO_NUM_5,
		.pin_cs = GPIO_NUM_18,
		.pin_dc = GPIO_NUM_19,
		.pin_d0 = GPIO_NUM_21,
		.pin_d1 = GPIO_NUM_25,
		.pin_d2 = GPIO_NUM_26,
		.pin_d3 = GPIO_NUM_27,
		.pin_d4 = GPIO_NUM_14,
		.pin_d5 = GPIO_NUM_12,
		.pin_d6 = GPIO_NUM_13,
		.pin_d7 = GPIO_NUM_15,
		.display_width = ILI9481_DISPLAY_WIDTH,
		.display_height = ILI9481_DISPLAY_HEIGHT,
	};
	/*
	ili9481_config_t config = {
		.kp0 = 0x0,
		.kp1 = 0x0,
		.kp2 = 0x6,
		.kp3 = 0x2,
		.kp4 = 0x1,
		.kp5 = 0x2,
		.rp0 = 0x0,
		.rp1 = 0x0,
		.vrp0 = 0x00,
		.vrp1 = 0x01f,
		.kn0 = 0x5,
		.kn1 = 0x6,
		.kn2 = 0x2,
		.kn3 = 0x3,
		.kn4 = 0x7,
		.kn5 = 0x7,
		.rn0 = 0x0,
		.rn1 = 0x0,
		.vrn0 = 0x0f,
		.vrn1 = 0x00,

		.vc = 0x7,
		.bt = 0x2,
		.vrh = 0x7,
		.vcm = 0x07,
		.vdv = 0x10,

		.ap0 = 0x1,
		.dc00 = 0x2,
		.dc10 = 0x0,

		.bc0 = 0x1,
		.div0 = 0x0,
		.rtn0 = 0x10,
		.fp0 = 0x8,
		.bp0 = 0x8,

		.fra = 0x3
	};
	*/

	ESP_ERROR_CHECK(ili9481_init(&display));
	write_init_message(&display);

	ili9481_command_t init_sequence[] = {
		{ILI9481_COMMAND_ACCESS_PROTECT, 1, (const uint8_t *)"\x00"},
		{ILI9481_POWER_SETTING, 3, (const uint8_t *)"\x07\x41\x1d"},
		{ILI9481_VCOM_CONTROL, 3, (const uint8_t *)"\x00\x2b\x1f"},
		{ILI9481_POWER_SETTING_NORMAL, 2, (const uint8_t *)"\x01\x11"},
		{ILI9481_DISPLAY_TIMING_SETTING_NORMAL, 3, (const uint8_t *)"\x10\x10\x88"},
		{ILI9481_PANEL_DRIVING_SETTING, 5, (const uint8_t *)"\x00\x3b\x00\x02\x11"},
		{ILI9481_FRAME_RATE_CONTROL, 1, (const uint8_t *)"\x00"},
		{ILI9481_GAMMA_SETTING, 12, (const uint8_t *)"\x00\x14\x33\x10\x00\x16\x44\x36\x77\x00\x0f\x00"},
		{ILI9481_SET_PIXEL_FORMAT, 1, (const uint8_t *)"\x66"},
		{ILI9481_SET_ADDRESS_MODE, 1, (const uint8_t *)"\x40"},
		{ILI9481_SET_DISPLAY_ON, 0, NULL},
		{ILI9481_CMDLIST_END, 0, NULL},
	};

	parameter_test(&display, init_sequence);

	//main_loop(&display, &config);

	vTaskDelay(portMAX_DELAY);
	vTaskDelete(NULL);
}
