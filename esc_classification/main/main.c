/**
 * @brief Convert I2S audio input to MFCC features
 * 
 * This program:
 * 1. Configures I2S to capture audio from INMP441 microphone
 * 2. Processes the audio through MFCC (Mel-frequency cepstral coefficients)
 * 3. Outputs MFCC features that can be used for audio classification
 */

#include <stdint.h>
#include <stdio.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "string.h"
#include <esp_mfcc_iface.h>
#include <esp_mfcc_models.h>

#define I2S_SD 12
#define I2S_WS 10
#define I2S_SCK 11

#define bufferLen 1024
#define SAMPLE_RATE 16000

#define TAG "INMP441"

int16_t sBuffer[bufferLen];
i2s_chan_handle_t rx_handle;

esp_mfcc_opts_t opts = {
    .winstep_ms = 20,        // Frame stride = 20ms
    .winlen_ms = 20,         // Frame length = 20ms
    .nch = 1,                // Mono
    .numcep = 13,            // Số hệ số MFCC cần lấy
    .nfilter = 32,           // Số bộ lọc Mel
    .nfft = 2048,            // FFT length
    .samp_freq = 16000,      // Tần số lấy mẫu
    .low_freq = 300,         // Tần số thấp nhất trong Mel filterbank
    .high_freq = 8000,       // Tần số cao nhất
    .preemph = 0.98f,        // Hệ số pre-emphasis
    .win_type = "hamming",   // Loại cửa sổ
    .append_energy = true,   // Lấy năng lượng vào MFCC[0]
    .use_power = true,       // Dùng công suất (power spectrum)
    .use_log_fbank = 1,      // Lấy log(fbank + ε)
    .log_epsilon = 1e-7,     // Tránh log(0)
    .psram_first = false,
    .remove_dc_offset = true
};

// Declare the MFCC interface
const esp_mfcc_iface_t *mfcc_iface = &esp_fbank_f32;

void i2s_std_config(void);

void app_main(void)
{
    i2s_std_config();
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    
    // Create MFCC runner using the interface
    esp_mfcc_data_t *runner = mfcc_iface->create(&opts);
    if (runner == NULL) {
        printf("Không thể khởi tạo runner MFCC\n");
        return;
    }
    size_t bytesIn = 0;
    esp_err_t ret;
    
    // Calculate expected samples per frame
    int samples_per_frame = (SAMPLE_RATE * opts.winstep_ms) / 1000;
    ESP_LOGI(TAG, "Expected samples per frame: %d", samples_per_frame);
    
    while (1)
    {
        ret = i2s_channel_read(rx_handle, &sBuffer, bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);
        
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "I2S read error: %d", ret);
        }
        else
        {
            // Check if we received enough data
            int samples_received = bytesIn / sizeof(int16_t);
            ESP_LOGD(TAG, "Received %d samples (%d bytes)", samples_received, bytesIn);
            
            if (samples_received > 0) {
                // Process audio through MFCC
                float *mfcc_result = mfcc_iface->run_step(runner, sBuffer, 1);  // 1 là số kênh (mono)
                if (mfcc_result != NULL) {
                    // Log MFCC coefficients (first few values)
                    ESP_LOGI(TAG, "MFCC coefficients: [%.3f, %.3f, %.3f, %.3f, %.3f]", 
                             mfcc_result[0], mfcc_result[1], mfcc_result[2], 
                             mfcc_result[3], mfcc_result[4]);
                    
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Cleanup (this won't be reached in the current infinite loop, but good practice)
    if (runner != NULL) {
        mfcc_iface->destroy(runner);
    }
}

void i2s_std_config(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,  // Changed to 16-bit for better MFCC processing
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = I2S_SLOT_MODE_MONO,             // Changed to mono to match MFCC config
            .slot_mask = I2S_STD_SLOT_LEFT,              // Use left channel only
            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = false,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = true,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_SCK,
            .ws = (gpio_num_t)I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = (gpio_num_t)I2S_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
}
