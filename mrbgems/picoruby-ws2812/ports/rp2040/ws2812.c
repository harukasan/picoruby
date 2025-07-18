#include <stdlib.h>
#include <string.h>
#include <mrubyc.h>
#include "../../include/ws2812.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "ws2812.pio.h"

#define WS2812_MAX_PIXELS 255

typedef struct {
    int pin;
    int pixel_size;
    PIO pio;
    uint sm;
    int dma_channel;
    int program_offset;
    uint32_t pixel_data[WS2812_MAX_PIXELS];
    bool transfer_in_progress;
    bool in_use;
} rp2040_ws2812_data_t;

static rp2040_ws2812_data_t ws2812_instance;

static inline uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           (uint32_t)(b);
}

int ws2812_init(int pin, int pixel_size) {    
    rp2040_ws2812_data_t* data = &ws2812_instance;
    if (data->in_use) {
        // skip initialization if already in use
        return 0;
    }

    if (pixel_size > WS2812_MAX_PIXELS) {
        return -1;
    }

    memset(data, 0, sizeof(rp2040_ws2812_data_t));    
    data->pin = pin;
    data->pixel_size = pixel_size;
    data->in_use = true;
    data->pio = pio0;
    data->sm = 0;
    data->transfer_in_progress = false;
    
    data->program_offset = pio_add_program(data->pio, &ws2812_program);
    ws2812_program_init(data->pio, data->sm, data->program_offset, pin, 800000, false);
    
    data->dma_channel = dma_claim_unused_channel(true);
    
    dma_channel_config c = dma_channel_get_default_config(data->dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(data->pio, data->sm, true));

    dma_channel_configure(data->dma_channel, &c, &data->pio->txf[data->sm], NULL, 0, false);

    return 0;
}

void ws2812_set_pixel_at_rgb(int index, uint8_t r, uint8_t g, uint8_t b) {
    rp2040_ws2812_data_t* data = &ws2812_instance;
    if (index >= data->pixel_size) return;
    data->pixel_data[index] = rgb_to_grb(r, g, b) << 8u;
}

void ws2812_clear() {
    rp2040_ws2812_data_t* data = &ws2812_instance;
    memset(data->pixel_data, 0, data->pixel_size * sizeof(uint32_t));
}

static bool ws2812_is_busy() {
    rp2040_ws2812_data_t* data = &ws2812_instance;
    
    if (!data->transfer_in_progress) return false;

    if (!dma_channel_is_busy(data->dma_channel) &&
        pio_sm_is_tx_fifo_empty(data->pio, data->sm)) {
        data->transfer_in_progress = false;
        return false;
    }
    return true;
}

void ws2812_show() {
    rp2040_ws2812_data_t* data = &ws2812_instance;
    
    while (ws2812_is_busy()) {
        tight_loop_contents();
    }    
    data->transfer_in_progress = true;
    dma_channel_set_read_addr(data->dma_channel, data->pixel_data, false);
    dma_channel_set_trans_count(data->dma_channel, data->pixel_size, true);
}
