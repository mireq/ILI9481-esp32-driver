#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "ili9481.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp32/rom/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"


const char *TAG = "ili9481";

#define CS_ACTIVE    gpio_set_level(driver->pin_cs, 0);
#define CS_IDLE      gpio_set_level(driver->pin_cs, 1);
#define CD_COMMAND   gpio_set_level(driver->pin_dc, 0);
#define CD_DATA      gpio_set_level(driver->pin_dc, 1);


static void write_bits(ili9481_driver_t *driver, uint8_t data) {
	gpio_set_level(driver->pin_d0, ((data >> 0) & 0x01));
	gpio_set_level(driver->pin_d1, ((data >> 1) & 0x01));
	gpio_set_level(driver->pin_d2, ((data >> 2) & 0x01));
	gpio_set_level(driver->pin_d3, ((data >> 3) & 0x01));
	gpio_set_level(driver->pin_d4, ((data >> 4) & 0x01));
	gpio_set_level(driver->pin_d5, ((data >> 5) & 0x01));
	gpio_set_level(driver->pin_d6, ((data >> 6) & 0x01));
	gpio_set_level(driver->pin_d7, ((data >> 7) & 0x01));
	gpio_set_level(driver->pin_wr, 0);
	gpio_set_level(driver->pin_wr, 1);
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

	gpio_set_level(driver->pin_rd, 0);
	data |= gpio_get_level(driver->pin_d0) << 0;
	data |= gpio_get_level(driver->pin_d1) << 1;
	data |= gpio_get_level(driver->pin_d2) << 2;
	data |= gpio_get_level(driver->pin_d3) << 3;
	data |= gpio_get_level(driver->pin_d4) << 4;
	data |= gpio_get_level(driver->pin_d5) << 5;
	data |= gpio_get_level(driver->pin_d6) << 6;
	data |= gpio_get_level(driver->pin_d7) << 7;
	gpio_set_level(driver->pin_rd, 1);

	io_conf.mode = GPIO_MODE_OUTPUT;
	gpio_config(&io_conf);

	return data;
}


static void write_command(ili9481_driver_t *driver, uint8_t command) {
	CD_COMMAND;
	CS_ACTIVE;
	write_bits(driver, command);
	CS_IDLE;
}


static void write_data_8(ili9481_driver_t *driver, uint8_t data) {
	CD_DATA;
	CS_ACTIVE;
	write_bits(driver, data);
	CS_IDLE;
}


static uint8_t read_data_8(ili9481_driver_t *driver) {
	CD_DATA;
	CS_ACTIVE;
	uint8_t data = read_bits(driver);
	CS_IDLE;
	return data;
}


/*
static void write_data_16(ili9481_driver_t *driver, uint16_t data) {
	CD_DATA;
	CS_ACTIVE;
	write_bits(driver, ((data >> 8) & 0x00FF));
	write_bits(driver, (data & 0x00FF));
	CS_IDLE;
}
*/

static void write_data_24(ili9481_driver_t *driver, uint32_t data) {
	CD_DATA;
	CS_ACTIVE;
	write_bits(driver, ((data >> 16) & 0x00FF));
	write_bits(driver, ((data >> 8) & 0x00FF));
	write_bits(driver, (data & 0x00FF));
	CS_IDLE;
}

static void set_addr_window(ili9481_driver_t *driver, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
write_command(driver, SET_COL_ADDRESS); // Column addr set
	write_data_8(driver, x0 >> 8);
	write_data_8(driver, x0 & 0xFF);     // XSTART
	write_data_8(driver, x1 >> 8);
	write_data_8(driver, x1 & 0xFF);     // XEND

	write_command(driver, SET_PAGE_ADDRESS); // Row addr set
	write_data_8(driver, y0 >> 8);
	write_data_8(driver, y0);     // YSTART
	write_data_8(driver, y1 >> 8);
	write_data_8(driver, y1);     // YEND

	write_command(driver, WRITE_MEMORY_START); // write to RAM
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

#undef ili9481_rgb_to_color
#define ili9481_rgb_to_color(r, g, b) ((r << 16) | (g << 8) | b)

esp_err_t ili9481_init(ili9481_driver_t *driver) {
	/*
	driver->buffer = (ili9481_color_t *)heap_caps_malloc(driver->buffer_size * 2 * sizeof(ili9481_color_t), MALLOC_CAP_DMA);
	if (driver->buffer == NULL) {
		ESP_LOGE(TAG, "buffer not allocated");
		return ESP_FAIL;
	}
	driver->buffer_a = driver->buffer;
	driver->buffer_b = driver->buffer + driver->buffer_size;
	driver->current_buffer = driver->buffer_a;
	driver->queue_fill = 0;
	*/

	driver->data.driver = driver;
	driver->data.data = true;
	driver->command.driver = driver;
	driver->command.data = false;

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

	gpio_set_level(driver->pin_rd, 1);
	gpio_set_level(driver->pin_dc, 1);
	gpio_set_level(driver->pin_cs, 1);
	gpio_set_level(driver->pin_wr, 1);
	gpio_set_level(driver->pin_rst, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(driver->pin_rst, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(driver->pin_rst, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(driver->pin_cs, 0);

	ESP_LOGW(TAG, "initialized");

	write_command(driver, EXIT_SLEEP_MODE);
	vTaskDelay(80 / portTICK_PERIOD_MS);


	uint8_t data;
	write_command(driver, 0xBF);
	data = read_data_8(driver);
	data = read_data_8(driver);
	printf("%x\n", data);
	data = read_data_8(driver);
	printf("%x\n", data);
	data = read_data_8(driver);
	printf("%x\n", data);
	data = read_data_8(driver);
	printf("%x\n", data);
	data = read_data_8(driver);
	printf("%x\n", data);

  write_command(driver, POWER_SETTING); //power setting
    write_data_8(driver, 0x07U);
    write_data_8(driver, 0x42U);
    write_data_8(driver, 0x17U);

  write_command(driver, VCOM_CONTROL); //VCOM control
    write_data_8(driver, 0x00U);
    write_data_8(driver, 0x07U);
    write_data_8(driver, 0x10U);

  write_command(driver, POWER_SETTING_NORMAL); //power setting for normal mode
    write_data_8(driver, 0x01U);
    write_data_8(driver, 0x02U); //Fosc setting

  write_command(driver, TIMING_SETTING_NORMAL); //power setting for normal mode
    write_data_8(driver, 0x10);
    write_data_8(driver, 0x10);
    write_data_8(driver, 0x88);

  write_command(driver, PANEL_DRIVE_SETTING); //panel driving setting - 5 parameters
    write_data_8(driver, 0x10U); //REV=1, SM=0, GS=0 - grayscale inversion enabled : will invert colors
    //write_data_8(driver, 0x00); //REV=0, SM=0, GS=0 - no color inversion

    write_data_8(driver, 0x3BU); //NL[5] - max lines
    write_data_8(driver, 0x00U); //SCN - scanning start position
    write_data_8(driver, 0x02U); //NDL (non-display area o/p level), PTS[3]

    write_data_8(driver, 0x11); //PTG=1 (interval scan), ISC[3]=0001 (3 frames)
    // write_data_8(driver, 0x1F); //PTG=1 (interval scan), ISC[3]=1111 (31 frames)
    // write_data_8(driver, 0x01); //PTG=0 (normal scan), ISC[3]=0002 (3 frames)

  write_command(driver, FRAME_RATE_CONTROL); //frame rate and inversion control - 1 parameter
    write_data_8(driver, 0x03); //72FPS (default) - this had backlight flickering
    //write_data_8(driver, 0x00U); //125 FPS (max) - this has no flickering

  write_command(driver, GAMMA_SETTING); //gamma setting
  /*
    write_data_8(driver, 0x00U);
    write_data_8(driver, 0x44U);
    write_data_8(driver, 0x06U);
    write_data_8(driver, 0x44U);
    write_data_8(driver, 0x0AU);
    write_data_8(driver, 0x08U);
    write_data_8(driver, 0x17U);
    write_data_8(driver, 0x33U);
    write_data_8(driver, 0x77U);
    write_data_8(driver, 0x44U);
    write_data_8(driver, 0x08U);
    write_data_8(driver, 0x0CU);
    */

    write_data_8(driver, 0x00U);
    write_data_8(driver, 0x26U);
    write_data_8(driver, 0x21U);
    write_data_8(driver, 0x00U);
    write_data_8(driver, 0x00U);
    write_data_8(driver, 0x1fU);
    write_data_8(driver, 0x65U);
    write_data_8(driver, 0x23U);
    write_data_8(driver, 0x77U);
    write_data_8(driver, 0x00U);
    write_data_8(driver, 0x0fU);
    write_data_8(driver, 0x00U);

  write_command(driver, SET_PIXEL_FORMAT); //set pixel format - 1 parameter
    //write_data_8(driver, 0x55U); //16-bit per pixel
    write_data_8(driver, 0x66); //18-bit per pixel

	 /*
  write_command(driver, 0xe4U); //set pixel format - 1 parameter
    write_data_8(driver, 0xa0U); //16-bit per pixel
  write_command(driver, 0xf0U); //set pixel format - 1 parameter
    write_data_8(driver, 0x01U); //16-bit per pixel
  write_command(driver, 0xf3U); //set pixel format - 1 parameter
    write_data_8(driver, 0x02U); //16-bit per pixel
    write_data_8(driver, 0x1aU); //16-bit per pixel
    */

  write_command(driver, SET_DISPLAY_ON); //set display on
  write_command(driver, ENTER_INVERT_MODE); //set display on

  set_addr_window(driver, 0, 0,  driver->display_width - 1, driver->display_height - 1);

  /*
  for(int i=0; i<driver->display_height; i++) {
    for(int j=0; j<driver->display_width; j++) {
      write_data_16(driver, ili9481_rgb_to_color(255, 255, 255));
    }
  }
  for(int i=0; i<driver->display_height; i++) {
    for(int j=0; j<driver->display_width; j++) {
      write_data_16(driver, ili9481_rgb_to_color(0, 0, 0));
    }
  }
  for(int i=0; i<driver->display_height; i++) {
    for(int j=0; j<driver->display_width; j++) {
    	 int col = j%2==0?255:0;
      write_data_16(driver, ili9481_rgb_to_color(col * j / driver->display_width, 0, 0));
    }
  }
  for(int i=0; i<driver->display_height; i++) {
    for(int j=0; j<driver->display_width; j++) {
    	 int col = i%2==0?255:0;
      write_data_16(driver, ili9481_rgb_to_color(0, col * i / driver->display_height, 0));
    }
  }
  for(int i=0; i<driver->display_height; i++) {
    for(int j=0; j<driver->display_width; j++) {
      write_data_16(driver, ili9481_rgb_to_color(i & 0xff, (i+j) & 0xff, j & 0xff));
    }
  }
  */
  /*
  for(int i=0; i<driver->display_height; i++) {
    for(int j=0; j<driver->display_width; j++) {
    	 int col = i%2==0?255:0;
    	 col = 255;
    	 col = col * j / driver->display_width;
      //write_data_24(driver, (col << 16) | (col << 8) | col);
      write_data_24(driver, ili9481_rgb_to_color(col, col, col));
    }
  }
  */
	for(int i=0; i<driver->display_height; i++) {
		for(int j=0; j<driver->display_width; j++) {
			int col = 255;
			if ((j < 160 && i < 240) || (j >= 160 && i >= 240)) {
				col = col * i / (driver->display_height / 2);
			}
			else {
				col = col * j / (driver->display_width / 2);
			}
			col = col % 256;
			write_data_24(driver, ili9481_rgb_to_color(col, col, col));
		}
	}

	//write_command(driver, READ_MEMORY_START); // write to RAM
	//data = read_data_8(driver);
	//for (size_t i = 0; i < 100; ++i) {
	//	data = read_data_8(driver);
	//	printf("%x\n", data);
	//}
  //return ESP_OK;
  //
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
			continue;
		}
		if (top) {
			color[component] |= htoi(c);
		}
		else {
			color[component] = (htoi(c) << 4);
		}
		if (component == 2 && top == 1) {
			write_data_24(driver, ili9481_rgb_to_color(color[2], color[1], color[0]));
			color[0] = 0;
			color[1] = 0;
			color[2] = 0;
		}
		if (top == 1) {
			component = (component + 1) % 3;
			top = 0;
		}
		else {
			top = 1;
		}

		/*
		for (size_t component = 0; component < 3; ++component) {
			uint8_t c;
			s = uart_rx_one_char(&c);
			if (s == OK) {
				color[component] = htoi(c);
			}
			STATUS s = uart_rx_one_char(&c);
			if (s == OK) {
				color[component] |= (htoi(c) << 4);
			}
		}
		*/
	}


	int kp0 = 0x0;
	int kp1 = 0x0;
	int kp2 = 0x6;
	int kp3 = 0x2;
	int kp4 = 0x1;
	int kp5 = 0x2;
	int rp0 = 0x0;
	int rp1 = 0x0;
	int vrp0 = 0x00;
	int vrp1 = 0x01f;
	int kn0 = 0x5;
	int kn1 = 0x6;
	int kn2 = 0x2;
	int kn3 = 0x3;
	int kn4 = 0x7;
	int kn5 = 0x7;
	int rn0 = 0x0;
	int rn1 = 0x0;
	int vrn0 = 0x0f;
	int vrn1 = 0x00;

	int vc = 0x7;
	int bt = 0x2;
	int vrh = 0xd;
	int vcm = 0x2b;
	int vdv = 0x1f;

	while (1) {
		uint8_t c;
		s = uart_rx_one_char(&c);
		if (s == OK) {
			/*
			switch (c) {
case: 113:
			}
			if (c == 113) {
				kp0 = (kp0 + 1) % 8;
			}
			else if (c == 113) {
				kp0 = (kp0 + 1) % 8;
			}
			else {
				printf("%d\n", c);
			}
			*/
			switch (c) {
				case 'q':
					kp0 = (kp0 + 1) % 8;
					break;
				case 119:
					kp1 = (kp1 + 1) % 8;
					break;
				case 101:
					kp2 = (kp2 + 1) % 8;
					break;
				case 114:
					kp3 = (kp3 + 1) % 8;
					break;
				case 116:
					kp4 = (kp4 + 1) % 8;
					break;
				case 121:
					kp5 = (kp5 + 1) % 8;
					break;
				case 117:
					rp0 = (rp0 + 1) % 8;
					break;
				case 105:
					rp1 = (rp1 + 1) % 8;
					break;
				case 97:
					kn0 = (kn0 + 1) % 8;
					break;
				case 115:
					kn1 = (kn1 + 1) % 8;
					break;
				case 100:
					kn2 = (kn2 + 1) % 8;
					break;
				case 102:
					kn3 = (kn3 + 1) % 8;
					break;
				case 103:
					kn4 = (kn4 + 1) % 8;
					break;
				case 104:
					kn5 = (kn5 + 1) % 8;
					break;
				case 122:
					vrp0 = (vrp0 + 1) % 16;
					break;
				case 120:
					vrp1 = (vrp1 + 1) % 32;
					break;
				case 99:
					vrn0 = (vrn0 + 1) % 16;
					break;
				case 118:
					vrn1 = (vrn1 + 1) % 32;
					break;
				case 111:
					vcm = (vcm + 1) % 64;
					break;
				case 112:
					vdv = (vdv + 1) % 32;
					break;
				case 98:
					vc = (vc + 1) % 8;
					break;
				case 110:
					bt = (bt + 1) % 8;
					break;
				case 109:
					vrh= (vrh + 1) % 16;
					break;
				default:
					printf("%d\n", c);
			}
			printf("kp0=%x, kp1=%x, kp2=%x, kp3=%x, kp4=%x, kp5=%x, rp0=%x, rp1=%x, vrp0=%x, vrp1=%x\nkn0=%x, kn1=%x, kn2=%x, kn3=%x, kn4=%x, kn5=%x, rn0=%x, rn1=%x, vrn0=%x, vrn1=%x\nvc=%x, bt=%x, vrh=%x, vcm=%x, vdv=%x\n\n", kp0, kp1, kp2, kp3, kp4, kp5, rp0, rp1, vrp0, vrp1, kn0, kn1, kn2, kn3, kn4, kn5, rn0, rn1, vrn0, vrn1, vc, bt, vrh, vcm, vdv);

			write_command(driver, POWER_SETTING); //power setting
			write_data_8(driver, vc);
			write_data_8(driver, 0x40U | bt);
			write_data_8(driver, 0x10U | vrh);

			write_command(driver, VCOM_CONTROL); //VCOM control
			write_data_8(driver, 0x00U);
			write_data_8(driver, vcm);
			write_data_8(driver, vdv);

			write_command(driver, GAMMA_SETTING);
			write_data_8(driver, kp0 | (kp1 << 4));
			write_data_8(driver, kp2 | (kp3 << 4));
			write_data_8(driver, kp4 | (kp5 << 4));
			write_data_8(driver, rp0 | (rp1 << 4));
			write_data_8(driver, vrp0);
			write_data_8(driver, vrp1);
			write_data_8(driver, kn0 | (kn1 << 4));
			write_data_8(driver, kn2 | (kn3 << 4));
			write_data_8(driver, kn4 | (kn5 << 4));
			write_data_8(driver, rn0 | (rn1 << 4));
			write_data_8(driver, vrn0);
			write_data_8(driver, vrn1);
		}
		/*
		for (int a = 0x38; a < 0x40; ++a) {
			write_command(driver, VCOM_CONTROL); //VCOM control
			write_data_8(driver, 0x00U);
			write_data_8(driver, a);
			write_data_8(driver, 0x13U);
			vTaskDelay(10);
			ESP_LOGW(TAG, "%02x, ", a);
		}
		*/
	}

    vTaskDelay(1000);

	return ESP_OK;
}


/*
static void ili9481_pre_cb(spi_transaction_t *transaction) {
	const ili9481_transaction_data_t *data = (ili9481_transaction_data_t *)transaction->user;
	gpio_set_level(data->driver->pin_dc, data->data);
}


esp_err_t ili9481_init(ili9481_driver_t *driver) {
	driver->buffer = (ili9481_color_t *)heap_caps_malloc(driver->buffer_size * 2 * sizeof(ili9481_color_t), MALLOC_CAP_DMA);
	if (driver->buffer == NULL) {
		ESP_LOGE(TAG, "buffer not allocated");
		return ESP_FAIL;
	}
	driver->buffer_a = driver->buffer;
	driver->buffer_b = driver->buffer + driver->buffer_size;
	driver->current_buffer = driver->buffer_a;
	driver->queue_fill = 0;

	driver->data.driver = driver;
	driver->data.data = true;
	driver->command.driver = driver;
	driver->command.data = false;

	gpio_pad_select_gpio(driver->pin_rst);
	gpio_pad_select_gpio(driver->pin_dc);
	gpio_set_direction(driver->pin_rst, GPIO_MODE_OUTPUT);
	gpio_set_direction(driver->pin_dc, GPIO_MODE_OUTPUT);

	spi_bus_config_t buscfg = {
		.mosi_io_num=driver->pin_mosi,
		.miso_io_num=-1,
		.sclk_io_num=driver->pin_sclk,
		.quadwp_io_num=-1,
		.quadhd_io_num=-1,
		.max_transfer_sz=driver->buffer_size * 2 * sizeof(ili9481_color_t), // 2 buffers with 2 bytes for pixel
		.flags=SPICOMMON_BUSFLAG_NATIVE_PINS
	};
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz=SPI_MASTER_FREQ_40M,
		.mode=3,
		.spics_io_num=-1,
		.queue_size=ILI9481_SPI_QUEUE_SIZE,
		.pre_cb=ili9481_pre_cb,
	};

	if (spi_bus_initialize(driver->spi_host, &buscfg, driver->dma_chan) != ESP_OK) {
		ESP_LOGE(TAG, "spi bus initialize failed");
		return ESP_FAIL;
	}
	if (spi_bus_add_device(driver->spi_host, &devcfg, &driver->spi) != ESP_OK) {
		ESP_LOGE(TAG, "spi bus add device failed");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "driver initialized");
	return ESP_OK;
}


void ili9481_reset(ili9481_driver_t *driver) {
	gpio_set_level(driver->pin_rst, 0);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	gpio_set_level(driver->pin_rst, 1);
	vTaskDelay(130 / portTICK_PERIOD_MS);
}


void ili9481_lcd_init(ili9481_driver_t *driver) {
	const uint8_t caset[4] = {
		0x00,
		0x00,
		(driver->display_width - 1) >> 8,
		(driver->display_width - 1) & 0xff
	};
	const uint8_t raset[4] = {
		0x00,
		0x00,
		(driver->display_height - 1) >> 8,
		(driver->display_height - 1) & 0xff
	};
	const ili9481_command_t init_sequence[] = {
		// Sleep
		{ILI9481_CMD_SLPIN, 10, 0, NULL},                    // Sleep
		{ILI9481_CMD_SWRESET, 200, 0, NULL},                 // Reset
		{ILI9481_CMD_SLPOUT, 120, 0, NULL},                  // Sleep out

		{ILI9481_CMD_MADCTL, 0, 1, (const uint8_t *)"\x00"}, // Page / column address order
		{ILI9481_CMD_COLMOD, 0, 1, (const uint8_t *)"\x55"}, // 16 bit RGB
		{ILI9481_CMD_INVON, 0, 0, NULL},                     // Inversion on
		{ILI9481_CMD_CASET, 0, 4, (const uint8_t *)&caset},  // Set width
		{ILI9481_CMD_RASET, 0, 4, (const uint8_t *)&raset},  // Set height

		// Porch setting
		{ILI9481_CMD_PORCTRL, 0, 5, (const uint8_t *)"\x0c\x0c\x00\x33\x33"},
		// Set VGH to 12.54V and VGL to -9.6V
		{ILI9481_CMD_GCTRL, 0, 1, (const uint8_t *)"\x14"},
		// Set VCOM to 1.475V
		{ILI9481_CMD_VCOMS, 0, 1, (const uint8_t *)"\x37"},
		// Enable VDV/VRH control
		{ILI9481_CMD_VDVVRHEN, 0, 2, (const uint8_t *)"\x01\xff"},
		// VAP(GVDD) = 4.45+(vcom+vcom offset+vdv)
		{ILI9481_CMD_VRHSET, 0, 1, (const uint8_t *)"\x12"},
		// VDV = 0V
		{ILI9481_CMD_VDVSET, 0, 1, (const uint8_t *)"\x20"},
		// AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V
		{ILI9481_CMD_PWCTRL1, 0, 2, (const uint8_t *)"\xa4\xa1"},
		// 60 fps
		{ILI9481_CMD_FRCTR2, 0, 1, (const uint8_t *)"\x0f"},
		// Gama 2.2
		{ILI9481_CMD_GAMSET, 0, 1, (const uint8_t *)"\x01"},
		// Gama curve
		{ILI9481_CMD_PVGAMCTRL, 0, 14, (const uint8_t *)"\xd0\x08\x11\x08\x0c\x15\x39\x33\x50\x36\x13\x14\x29\x2d"},
		{ILI9481_CMD_NVGAMCTRL, 0, 14, (const uint8_t *)"\xd0\x08\x10\x08\x06\x06\x39\x44\x51\x0b\x16\x14\x2f\x31"},

		// Little endian
		{ILI9481_CMD_RAMCTRL, 0, 2, (const uint8_t *)"\x00\xc8"},
		{ILI9481_CMDLIST_END, 0, 0, NULL},                   // End of commands
	};
	ili9481_run_commands(driver, init_sequence);
	ili9481_clear(driver, 0x0000);
	const ili9481_command_t init_sequence2[] = {
		{ILI9481_CMD_DISPON, 100, 0, NULL},                  // Display on
		{ILI9481_CMD_SLPOUT, 100, 0, NULL},                  // Sleep out
		{ILI9481_CMD_CASET, 0, 4, caset},
		{ILI9481_CMD_RASET, 0, 4, raset},
		{ILI9481_CMD_RAMWR, 0, 0, NULL},
		{ILI9481_CMDLIST_END, 0, 0, NULL},                   // End of commands
	};
	ili9481_run_commands(driver, init_sequence2);
}


void ili9481_start_command(ili9481_driver_t *driver) {
	gpio_set_level(driver->pin_dc, 0);
}


void ili9481_start_data(ili9481_driver_t *driver) {
	gpio_set_level(driver->pin_dc, 1);
}

void ili9481_run_command(ili9481_driver_t *driver, const ili9481_command_t *command) {
	spi_transaction_t *rtrans;
	ili9481_wait_until_queue_empty(driver);
	spi_transaction_t trans;
	memset(&trans, 0, sizeof(trans));
	trans.length = 8; // 8 bits
	trans.tx_buffer = &command->command;
	trans.user = &driver->command;
	spi_device_queue_trans(driver->spi, &trans, portMAX_DELAY);
	spi_device_get_trans_result(driver->spi, &rtrans, portMAX_DELAY);

	if (command->data_size > 0) {
		memset(&trans, 0, sizeof(trans));
		trans.length = command->data_size * 8;
		trans.tx_buffer = command->data;
		trans.user = &driver->data;
		spi_device_queue_trans(driver->spi, &trans, portMAX_DELAY);
		spi_device_get_trans_result(driver->spi, &rtrans, portMAX_DELAY);
	}

	if (command->wait_ms > 0) {
		vTaskDelay(command->wait_ms / portTICK_PERIOD_MS);
	}
}

void ili9481_run_commands(ili9481_driver_t *driver, const ili9481_command_t *sequence) {
	while (sequence->command != ILI9481_CMDLIST_END) {
		ili9481_run_command(driver, sequence);
		sequence++;
	}
}

void ili9481_clear(ili9481_driver_t *driver, ili9481_color_t color) {
	ili9481_fill_area(driver, color, 0, 0, driver->display_width, driver->display_height);
}

void ili9481_fill_area(ili9481_driver_t *driver, ili9481_color_t color, uint16_t start_x, uint16_t start_y, uint16_t width, uint16_t height) {
	for (size_t i = 0; i < driver->buffer_size * 2; ++i) {
		driver->buffer[i] = color;
	}
	ili9481_set_window(driver, start_x, start_y, start_x + width - 1, start_y + height - 1);

	size_t bytes_to_write = width * height * 2;
	size_t transfer_size = driver->buffer_size * 2 * sizeof(ili9481_color_t);

	spi_transaction_t trans;
	memset(&trans, 0, sizeof(trans));
	trans.tx_buffer = driver->buffer;
	trans.user = &driver->data;
	trans.length = transfer_size * 8;
	trans.rxlength = 0;

	spi_transaction_t *rtrans;

	while (bytes_to_write > 0) {
		if (driver->queue_fill >= ILI9481_SPI_QUEUE_SIZE) {
			spi_device_get_trans_result(driver->spi, &rtrans, portMAX_DELAY);
			driver->queue_fill--;
		}
		if (bytes_to_write < transfer_size) {
			transfer_size = bytes_to_write;
		}
		spi_device_queue_trans(driver->spi, &trans, portMAX_DELAY);
		driver->queue_fill++;
		bytes_to_write -= transfer_size;
	}

	ili9481_wait_until_queue_empty(driver);
}

void ili9481_set_window(ili9481_driver_t *driver, uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
	uint8_t caset[4];
	uint8_t raset[4];
	caset[0] = (uint8_t)(start_x >> 8);
	caset[1] = (uint8_t)(start_x & 0xff);
	caset[2] = (uint8_t)(end_x >> 8);
	caset[3] = (uint8_t)(end_x & 0xff);
	raset[0] = (uint8_t)(start_y >> 8);
	raset[1] = (uint8_t)(start_y & 0xff);
	raset[2] = (uint8_t)(end_y >> 8);
	raset[3] = (uint8_t)(end_y & 0xff);
	ili9481_command_t sequence[] = {
		{ILI9481_CMD_CASET, 0, 4, caset},
		{ILI9481_CMD_RASET, 0, 4, raset},
		{ILI9481_CMD_RAMWR, 0, 0, NULL},
		{ILI9481_CMDLIST_END, 0, 0, NULL},
	};
	ili9481_run_commands(driver, sequence);
}

void ili9481_write_pixels(ili9481_driver_t *driver, ili9481_color_t *pixels, size_t length) {
	ili9481_wait_until_queue_empty(driver);

	spi_transaction_t *trans = driver->current_buffer == driver->buffer_a ? &driver->trans_a : &driver->trans_b;
	memset(trans, 0, sizeof(&trans));
	trans->tx_buffer = driver->current_buffer;
	trans->user = &driver->data;
	trans->length = length * sizeof(ili9481_color_t) * 8;
	trans->rxlength = 0;

	spi_device_queue_trans(driver->spi, trans, portMAX_DELAY);
	driver->queue_fill++;
}

void ili9481_wait_until_queue_empty(ili9481_driver_t *driver) {
	spi_transaction_t *rtrans;
	while (driver->queue_fill > 0) {
		spi_device_get_trans_result(driver->spi, &rtrans, portMAX_DELAY);
		driver->queue_fill--;
	}
}

void ili9481_swap_buffers(ili9481_driver_t *driver) {
	ili9481_write_pixels(driver, driver->current_buffer, driver->buffer_size);
	driver->current_buffer = driver->current_buffer == driver->buffer_a ? driver->buffer_b : driver->buffer_a;
}


uint8_t ili9481_dither_table[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void ili9481_randomize_dither_table() {
	uint16_t *dither_table = (uint16_t *)ili9481_dither_table;
	for (size_t i = 0; i < sizeof(ili9481_dither_table) / 2; ++i) {
		dither_table[i] = rand() & 0xffff;
	}
}


void ili9481_draw_gray2_bitmap(uint8_t *src_buf, ili9481_color_t *target_buf, uint8_t r, uint8_t g, uint8_t b, int x, int y, int src_w, int src_h, int target_w, int target_h) {
	if (x >= target_w || y >= target_h || x + src_w <= 0 || y + src_h <= 0) {
		return;
	}

	const size_t src_size = src_w * src_h;
	const size_t target_size = target_w * target_h;
	const size_t line_w = MIN(src_w + x, target_w) - MAX(x, 0);
	const size_t src_skip = src_w - line_w;
	const size_t target_skip = target_w - line_w;
	size_t src_pos = 0;
	size_t target_pos = 0;
	size_t x_pos = 0;
	size_t y_pos = 0;

	if (y < 0) {
		src_pos = (-y) * src_w;
	}
	if (x < 0) {
		src_pos -= x;
	}
	if (y > 0) {
		target_pos = y * target_w;
	}
	if (x > 0) {
		target_pos += x;
	}

	while (src_pos < src_size && target_pos < target_size) {
		uint8_t src_r, src_g, src_b;
		uint8_t target_r, target_g, target_b;
		ili9481_color_to_rgb(target_buf[target_pos], &src_r, &src_g, &src_b);
		uint8_t gray2_color = (src_buf[src_pos >> 2] >> ((src_pos & 0x03) << 1)) & 0x03;
		switch(gray2_color) {
			case 1:
				target_r = r >> 1;
				target_g = g >> 1;
				target_b = b >> 1;
				src_r = (src_r >> 1) + target_r;
				src_g = (src_g >> 1) + target_g;
				src_b = (src_b >> 1) + target_b;
				target_buf[target_pos] = ili9481_rgb_to_color_dither(src_r, src_g, src_b, x_pos, y_pos);
				break;
			case 2:
				target_r = r >> 2;
				target_g = g >> 2;
				target_b = b >> 2;
				src_r = (src_r >> 2) + target_r + target_r + target_r;
				src_g = (src_g >> 2) + target_g + target_g + target_g;
				src_b = (src_b >> 2) + target_b + target_b + target_b;
				target_buf[target_pos] = ili9481_rgb_to_color_dither(src_r, src_g, src_b, x_pos, y_pos);
				break;
			case 3:
				target_buf[target_pos] = ili9481_rgb_to_color_dither(r, g, b, x_pos, y_pos);
				break;
			default:
				break;
		}

		x_pos++;

		if (x_pos == line_w) {
			x_pos = 0;
			y_pos++;
			src_pos += src_skip;
			target_pos += target_skip;
		}
		src_pos++;
		target_pos++;
	}
}
*/
