#ifndef PICORUBY_WS2812_H
#define PICORUBY_WS2812_H

#include <stdint.h>

int ws2812_init(int pin, int pixel_size);
void ws2812_set_pixel_at_rgb(int index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_clear();
void ws2812_show();
void ws2812_cleanup();

#endif // PICORUBY_WS2812_H
