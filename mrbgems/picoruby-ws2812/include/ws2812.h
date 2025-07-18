#ifndef PICORUBY_WS2812_H
#define PICORUBY_WS2812_H

#include <stdint.h>
#include <mrubyc.h>

typedef struct {
    unsigned int pin;
    unsigned int pixel_size;
    void* platform_data;
} ws2812_t;

int ws2812_init(ws2812_t* ws, unsigned int pin, unsigned int pixel_size);
void ws2812_set_pixel_rgb(ws2812_t* ws, uint index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_clear(ws2812_t* ws);
void ws2812_show(ws2812_t* ws);
void ws2812_cleanup(ws2812_t* ws);

#endif // PICORUBY_WS2812_H
