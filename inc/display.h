#ifndef DISPLAY_H
#define DISPLAY_H

#include "ssd1306.h"
#include "hardware/i2c.h"
#include <stdint.h>

#define I2C_DISPLAY i2c1
#define SDA_DISPLAY 14
#define SCL_DISPLAY 15
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

void display_init(ssd1306_t *oled);
void display_show_distance(ssd1306_t *oled, uint16_t distance);
void display_show_status(ssd1306_t *oled, const char *status_text);

#endif