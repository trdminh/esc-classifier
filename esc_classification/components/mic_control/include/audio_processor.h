#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include "mfcc.h"
#include "i2s_mic.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio processor handle combining I2S microphone and MFCC
 */
typedef struct {
    i2s_mic_t mic;
    mfcc_processor_t mfcc;
    bool is_running;
    TaskHandle_t task_handle;
    void (*mfcc_callback)(float *mfcc_features, int num_coeffs, void *user_data);
    void *user_data;
} audio_processor_t;

/**
 * @brief Audio processor configuration
 */
typedef struct {
    i2s_mic_config_t mic_config;
    esp_mfcc_opts_t mfcc_config;
    uint32_t process_interval_ms;    // Processing interval in milliseconds
    void (*mfcc_callback)(float *mfcc_features, int num_coeffs, void *user_data);
    void *user_data;
} audio_processor_config_t;

/**
 * @brief Initialize audio processor
 * 
 * @param processor Audio processor handle
 * @param config Configuration (NULL for default)
 * @return ESP_OK on success
 */
esp_err_t audio_processor_init(audio_processor_t *processor, const audio_processor_config_t *config);

/**
 * @brief Start audio processing task
 * 
 * @param processor Audio processor handle
 * @return ESP_OK on success
 */
esp_err_t audio_processor_start(audio_processor_t *processor);

/**
 * @brief Stop audio processing task
 * 
 * @param processor Audio processor handle
 * @return ESP_OK on success
 */
esp_err_t audio_processor_stop(audio_processor_t *processor);

/**
 * @brief Process single frame (blocking call)
 * 
 * @param processor Audio processor handle
 * @param mfcc_features Output buffer for MFCC features (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t audio_processor_process_frame(audio_processor_t *processor, float **mfcc_features);

/**
 * @brief Destroy audio processor and free resources
 * 
 * @param processor Audio processor handle
 */
void audio_processor_destroy(audio_processor_t *processor);

/**
 * @brief Get MFCC configuration
 * 
 * @param processor Audio processor handle
 * @return Pointer to MFCC configuration
 */
const esp_mfcc_opts_t *audio_processor_get_mfcc_config(audio_processor_t *processor);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PROCESSOR_H
