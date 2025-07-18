#ifndef MFCC_CONVERTER_H
#define MFCC_CONVERTER_H

#include "esp_mfcc_iface.h"
#include "esp_mfcc_models.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MFCC processor handle
 */
typedef struct {
    const esp_mfcc_iface_t *mfcc_iface;
    esp_mfcc_data_t *runner;
    esp_mfcc_opts_t opts;
    bool is_initialized;
} mfcc_processor_t;

/**
 * @brief Default MFCC configuration for 16kHz mono audio
 */
extern const esp_mfcc_opts_t default_mfcc_opts;

/**
 * @brief Initialize MFCC processor
 * 
 * @param processor Pointer to MFCC processor handle
 * @param opts MFCC configuration options (NULL for default)
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mfcc_processor_init(mfcc_processor_t *processor, const esp_mfcc_opts_t *opts);

/**
 * @brief Process audio samples to MFCC features
 * 
 * @param processor MFCC processor handle
 * @param samples Audio samples (int16_t array)
 * @param num_channels Number of audio channels (1 for mono, 2 for stereo)
 * @return Pointer to MFCC coefficients array (float*), or NULL if no output available
 * @note Do NOT call free() on returned pointer - it's managed internally
 */
float *mfcc_processor_run(mfcc_processor_t *processor, int16_t *samples, int16_t num_channels);

/**
 * @brief Clean MFCC processor state
 * 
 * @param processor MFCC processor handle
 */
void mfcc_processor_clean(mfcc_processor_t *processor);

/**
 * @brief Destroy MFCC processor and free resources
 * 
 * @param processor MFCC processor handle
 */
void mfcc_processor_destroy(mfcc_processor_t *processor);

/**
 * @brief Get number of MFCC coefficients
 * 
 * @param processor MFCC processor handle
 * @return Number of MFCC coefficients
 */
int mfcc_processor_get_num_coeffs(mfcc_processor_t *processor);

/**
 * @brief Get expected samples per frame
 * 
 * @param processor MFCC processor handle
 * @return Expected samples per frame
 */
int mfcc_processor_get_samples_per_frame(mfcc_processor_t *processor);

#ifdef __cplusplus
}
#endif

#endif // MFCC_CONVERTER_H
