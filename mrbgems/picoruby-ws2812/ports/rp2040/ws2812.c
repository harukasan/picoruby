#include <stdlib.h>
#include <string.h>
#include "../../include/ws2812.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "ws2812.pio.h"

typedef struct {
    PIO pio;
    uint sm;
    uint pin;
    uint dma_channel;
    uint program_offset;
    uint32_t* pixel_data;
    uint num_pixels;
    bool transfer_in_progress;
} rp2040_ws2812_data_t;

static inline uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           (uint32_t)(b);
}

static void start_dma_transfer(rp2040_ws2812_data_t* data) {
    dma_channel_set_read_addr(data->dma_channel, data->pixel_data, false);
    dma_channel_set_trans_count(data->dma_channel, data->num_pixels, true);
}

int ws2812_init(ws2812_t* ws, uint pin, uint pixel_size) {
    rp2040_ws2812_data_t* data = malloc(sizeof(rp2040_ws2812_data_t));
    memset(data, 0, sizeof(rp2040_ws2812_data_t));
    
    ws->pin = pin;
    ws->pixel_size = pixel_size;
    ws->platform_data = data;
    
    data->pio = pio0;
    int sm = pio_claim_unused_sm(data->pio, false);
    if (sm < 1) {
        data->pio = pio1;
        sm = pio_claim_unused_sm(data->pio, false);
        if (sm < 1) {
            free(data);
            ws->platform_data = NULL;
            ws->pixel_size = 0;
            return -1;
        }
    }

    data->pin = pin;
    data->num_pixels = pixel_size;
    data->transfer_in_progress = false;
    
    data->program_offset = pio_add_program(data->pio, &ws2812_program);
    ws2812_program_init(data->pio, data->sm, data->program_offset, data->pin, 800000, false);
    
    data->dma_channel = dma_claim_unused_channel(true);
    data->pixel_data = malloc(data->num_pixels * sizeof(uint32_t));
    ws2812_clear(ws);
    
    dma_channel_config c = dma_channel_get_default_config(data->dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(data->pio, data->sm, true));

    dma_channel_configure(data->dma_channel, &c, &data->pio->txf[data->sm], NULL, 0, false);

    return 0;
}

void ws2812_set_pixel_rgb(ws2812_t* ws, uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= ws->pixel_size) return;
    rp2040_ws2812_data_t* data = (rp2040_ws2812_data_t*)ws->platform_data;
    data->pixel_data[index] = rgb_to_grb(r, g, b) << 8u;
}

void ws2812_clear(ws2812_t* ws) {
    rp2040_ws2812_data_t* data = (rp2040_ws2812_data_t*)ws->platform_data;
    memset(data->pixel_data, 0, data->num_pixels * sizeof(uint32_t));
}

static bool ws2812_is_busy(ws2812_t* ws) {
    rp2040_ws2812_data_t* data = (rp2040_ws2812_data_t*)ws->platform_data;
    
    if (!data->transfer_in_progress) return false;

    if (!dma_channel_is_busy(data->dma_channel) &&
        pio_sm_is_tx_fifo_empty(data->pio, data->sm)) {
        data->transfer_in_progress = false;
        return false;
    }
    return true;
}

void ws2812_show(ws2812_t* ws) {
    rp2040_ws2812_data_t* data = (rp2040_ws2812_data_t*)ws->platform_data;
    
    while (ws2812_is_busy(ws)) {
        tight_loop_contents();
    }    
    data->transfer_in_progress = true;
    start_dma_transfer(data);
}

void ws2812_cleanup(ws2812_t* ws) {
    rp2040_ws2812_data_t* data = (rp2040_ws2812_data_t*)ws->platform_data;
    if (data) {
        if (data->pixel_data != NULL) {
            free(data->pixel_data);
            data->pixel_data = NULL;
        }
        dma_channel_unclaim(data->dma_channel);
        pio_remove_program(data->pio, &ws2812_program, data->program_offset);
        pio_sm_unclaim(data->pio, data->sm);
        free(data);
    }
    ws->platform_data = NULL;
}
