#include <stdlib.h>
#include <string.h>
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../include/ws2812.h"

#define RMT_RESOLUTION_HZ (10000000)
#define RMT_MEM_BLOCK_SYMBOLS (64)
#define RMT_TRANS_QUEUE_DEPTH (4)

// WS2812のタイミング（ナノ秒）
#define WS2812_T0H_NS (300)
#define WS2812_T0L_NS (900)
#define WS2812_T1H_NS (600)
#define WS2812_T1L_NS (600)
#define WS2812_RESET_NS (50000)

typedef struct {
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t rmt_encoder;
    uint8_t* pixel_buffer;
    uint pixel_count;
} esp32_ws2812_data_t;

static size_t encoder_callback(const void *data, size_t data_size,
    size_t symbols_written, size_t symbols_free, rmt_symbol_word_t *symbols, 
    bool *done, void *arg) {
    
    if (symbols_free < 8) return 0;

    const rmt_symbol_word_t symbol_zero = {
        .level0 = 1,
        .duration0 = (uint16_t)(((uint64_t)WS2812_T0H_NS * RMT_RESOLUTION_HZ) / 1000000000),
        .level1 = 0,
        .duration1 = (uint16_t)(((uint64_t)WS2812_T0L_NS * RMT_RESOLUTION_HZ) / 1000000000),
    };
    const rmt_symbol_word_t symbol_one = {
        .level0 = 1,
        .duration0 = (uint16_t)(((uint64_t)WS2812_T1H_NS * RMT_RESOLUTION_HZ) / 1000000000),
        .level1 = 0,
        .duration1 = (uint16_t)(((uint64_t)WS2812_T1L_NS * RMT_RESOLUTION_HZ) / 1000000000),
    };
    const rmt_symbol_word_t symbol_reset = {
        .level0 = 0,
        .duration0 = (uint16_t)(((uint64_t)WS2812_RESET_NS * RMT_RESOLUTION_HZ) / 1000000000),
        .level1 = 0,
        .duration1 = (uint16_t)(((uint64_t)WS2812_RESET_NS * RMT_RESOLUTION_HZ) / 1000000000),
    };

    size_t data_pos = symbols_written / 8;
    uint8_t *data_bytes = (uint8_t*)data;
    
    if (data_pos < data_size) {
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
            if (data_bytes[data_pos] & bitmask) {
                symbols[symbol_pos++] = symbol_one;
            } else {
                symbols[symbol_pos++] = symbol_zero;
            }
        }
        return symbol_pos;
    } else {
        symbols[0] = symbol_reset;
        *done = 1;
        return 1;
    }
}

void ws2812_init(ws2812_t* ws, uint pin, uint pixel_size) {
    esp32_ws2812_data_t* data = malloc(sizeof(esp32_ws2812_data_t));
    memset(data, 0, sizeof(esp32_ws2812_data_t));
    
    ws->pin = pin;
    ws->pixel_size = pixel_size;
    ws->platform_data = data;
    
    data->pixel_count = pixel_size;
    data->pixel_buffer = malloc(pixel_size * 3); // RGB = 3 bytes per pixel
    memset(data->pixel_buffer, 0, pixel_size * 3);
    
    // RMTチャンネル設定
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = pin,
        .mem_block_symbols = RMT_MEM_BLOCK_SYMBOLS,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = RMT_TRANS_QUEUE_DEPTH,
    };
    rmt_new_tx_channel(&tx_chan_config, &data->rmt_channel);
    
    // エンコーダ設定
    const rmt_simple_encoder_config_t simple_encoder_cfg = {
        .callback = encoder_callback
    };
    rmt_new_simple_encoder(&simple_encoder_cfg, &data->rmt_encoder);
    
    rmt_enable(data->rmt_channel);
}

void ws2812_set_pixel_rgb(ws2812_t* ws, uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= ws->pixel_size) return;
    
    esp32_ws2812_data_t* data = (esp32_ws2812_data_t*)ws->platform_data;
    uint8_t* pixel = &data->pixel_buffer[index * 3];
    
    // WS2812はGRB順序
    pixel[0] = g;
    pixel[1] = r;
    pixel[2] = b;
}

void ws2812_clear(ws2812_t* ws) {
    esp32_ws2812_data_t* data = (esp32_ws2812_data_t*)ws->platform_data;
    memset(data->pixel_buffer, 0, data->pixel_count * 3);
}

void ws2812_show(ws2812_t* ws) {
    esp32_ws2812_data_t* data = (esp32_ws2812_data_t*)ws->platform_data;
    
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    rmt_transmit(data->rmt_channel, data->rmt_encoder, 
                 data->pixel_buffer, data->pixel_count * 3, &tx_config);
    rmt_tx_wait_all_done(data->rmt_channel, portMAX_DELAY);
}

bool ws2812_is_busy(ws2812_t* ws) {
    // ESP32 RMTの場合は同期的に動作するため常にfalse
    return false;
}

void ws2812_cleanup(ws2812_t* ws) {
    esp32_ws2812_data_t* data = (esp32_ws2812_data_t*)ws->platform_data;
    if (data) {
        if (data->rmt_channel) {
            rmt_disable(data->rmt_channel);
            rmt_del_channel(data->rmt_channel);
        }
        if (data->rmt_encoder) {
            rmt_del_encoder(data->rmt_encoder);
        }
        if (data->pixel_buffer) {
            free(data->pixel_buffer);
        }
        free(data);
    }
    ws->platform_data = NULL;
} 