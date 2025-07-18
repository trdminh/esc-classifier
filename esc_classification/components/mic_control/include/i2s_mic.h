#ifndef I2S_MIC_H
#define I2S_MIC_H

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2S microphone configuration
 */
typedef struct {
    gpio_num_t sck_pin;         // Serial clock pin
    gpio_num_t ws_pin;          // Word select pin
    gpio_num_t sd_pin;          // Serial data pin
    uint32_t sample_rate;       // Sample rate in Hz
    uint32_t buffer_len;        // Buffer length in samples
    i2s_data_bit_width_t bit_width;  // Data bit width
    i2s_slot_mode_t slot_mode;  // Slot mode (mono/stereo)
} i2s_mic_config_t;

/**
 * @brief I2S microphone handle
 */
typedef struct {
    i2s_chan_handle_t rx_handle;
    i2s_mic_config_t config;
    int16_t *buffer;
    bool is_initialized;
} i2s_mic_t;

/**
 * @brief Default I2S microphone configuration for INMP441
 */
extern const i2s_mic_config_t default_i2s_mic_config;

/**
 * @brief Initialize I2S microphone
 * 
 * @param mic Pointer to I2S microphone handle
 * @param config I2S configuration (NULL for default)
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t i2s_mic_init(i2s_mic_t *mic, const i2s_mic_config_t *config);

/**
 * @brief Read audio samples from I2S microphone
 * 
 * @param mic I2S microphone handle
 * @param samples_read Pointer to store number of samples read
 * @param timeout_ms Timeout in milliseconds
 * @return Pointer to audio samples buffer, or NULL on error
 */
int16_t *i2s_mic_read(i2s_mic_t *mic, size_t *samples_read, uint32_t timeout_ms);

/**
 * @brief Stop I2S microphone
 * 
 * @param mic I2S microphone handle
 * @return ESP_OK on success
 */
esp_err_t i2s_mic_stop(i2s_mic_t *mic);

/**
 * @brief Start I2S microphone
 * 
 * @param mic I2S microphone handle
 * @return ESP_OK on success
 */
esp_err_t i2s_mic_start(i2s_mic_t *mic);

/**
 * @brief Destroy I2S microphone and free resources
 * 
 * @param mic I2S microphone handle
 */
void i2s_mic_destroy(i2s_mic_t *mic);

/**
 * @brief Get buffer size in samples
 * 
 * @param mic I2S microphone handle
 * @return Buffer size in samples
 */
uint32_t i2s_mic_get_buffer_size(i2s_mic_t *mic);

#ifdef __cplusplus
}
#endif

#endif // I2S_MIC_H
