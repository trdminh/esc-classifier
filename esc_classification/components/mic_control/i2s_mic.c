#include "i2s_mic.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "I2S_MIC";

const i2s_mic_config_t default_i2s_mic_config = {
    .sck_pin = GPIO_NUM_11,
    .ws_pin = GPIO_NUM_10,
    .sd_pin = GPIO_NUM_12,
    .sample_rate = 16000,
    .buffer_len = 1024,
    .bit_width = I2S_DATA_BIT_WIDTH_16BIT,
    .slot_mode = I2S_SLOT_MODE_MONO
};

esp_err_t i2s_mic_init(i2s_mic_t *mic, const i2s_mic_config_t *config)
{
    if (mic == NULL) {
        ESP_LOGE(TAG, "Microphone handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize microphone structure
    memset(mic, 0, sizeof(i2s_mic_t));

    // Use default config if not provided
    if (config == NULL) {
        mic->config = default_i2s_mic_config;
    } else {
        mic->config = *config;
    }

    // Allocate buffer
    mic->buffer = (int16_t*)malloc(mic->config.buffer_len * sizeof(int16_t));
    if (mic->buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        return ESP_ERR_NO_MEM;
    }

    // Configure I2S channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &mic->rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        free(mic->buffer);
        return ret;
    }

    // Configure I2S standard mode
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(mic->config.sample_rate),
        .slot_cfg = {
            .data_bit_width = mic->config.bit_width,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = mic->config.slot_mode,
            .slot_mask = (mic->config.slot_mode == I2S_SLOT_MODE_MONO) ? 
                         I2S_STD_SLOT_LEFT : I2S_STD_SLOT_BOTH,
            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = false,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = true,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = mic->config.sck_pin,
            .ws = mic->config.ws_pin,
            .dout = I2S_GPIO_UNUSED,
            .din = mic->config.sd_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(mic->rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S standard mode: %s", esp_err_to_name(ret));
        i2s_del_channel(mic->rx_handle);
        free(mic->buffer);
        return ret;
    }

    mic->is_initialized = true;
    return ESP_OK;
}

esp_err_t i2s_mic_start(i2s_mic_t *mic)
{
    if (mic == NULL || !mic->is_initialized) {
        ESP_LOGE(TAG, "Microphone not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = i2s_channel_enable(mic->rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2S microphone started");
    return ESP_OK;
}

int16_t *i2s_mic_read(i2s_mic_t *mic, size_t *samples_read, uint32_t timeout_ms)
{
    if (mic == NULL || !mic->is_initialized) {
        ESP_LOGE(TAG, "Microphone not initialized");
        return NULL;
    }

    if (samples_read == NULL) {
        ESP_LOGE(TAG, "samples_read pointer is NULL");
        return NULL;
    }

    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(mic->rx_handle, mic->buffer, 
                                     mic->config.buffer_len * sizeof(int16_t), 
                                     &bytes_read, 
                                     pdMS_TO_TICKS(timeout_ms));

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
        *samples_read = 0;
        return NULL;
    }

    *samples_read = bytes_read / sizeof(int16_t);
    
    if (*samples_read == 0) {
        ESP_LOGD(TAG, "No samples read");
        return NULL;
    }

    ESP_LOGD(TAG, "Read %d samples (%d bytes)", *samples_read, bytes_read);
    return mic->buffer;
}

esp_err_t i2s_mic_stop(i2s_mic_t *mic)
{
    if (mic == NULL || !mic->is_initialized) {
        ESP_LOGE(TAG, "Microphone not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = i2s_channel_disable(mic->rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2S microphone stopped");
    return ESP_OK;
}

void i2s_mic_destroy(i2s_mic_t *mic)
{
    if (mic == NULL) {
        return;
    }

    if (mic->is_initialized && mic->rx_handle) {
        i2s_channel_disable(mic->rx_handle);
        i2s_del_channel(mic->rx_handle);
        ESP_LOGI(TAG, "I2S microphone destroyed");
    }

    if (mic->buffer) {
        free(mic->buffer);
    }

    memset(mic, 0, sizeof(i2s_mic_t));
}

uint32_t i2s_mic_get_buffer_size(i2s_mic_t *mic)
{
    if (mic == NULL || !mic->is_initialized) {
        return 0;
    }
    return mic->config.buffer_len;
}
