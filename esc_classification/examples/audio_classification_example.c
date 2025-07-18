/**
 * @file audio_classification_example.c
 * @brief Example showing different ways to use mic_control library
 * 
 * This example demonstrates:
 * 1. Basic usage with callback
 * 2. Manual processing
 * 3. Audio classification with templates
 */

#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "audio_processor.h"

#define TAG "AUDIO_EXAMPLE"


#define ENERGY_THRESHOLD 15.0f

static audio_processor_t audio_proc;
static int detection_count = 0;


void simple_detection_callback(float *mfcc_features, int num_coeffs, void *user_data)
{
    if (mfcc_features != NULL) {
        float energy = mfcc_features[0];
        
        if (energy > ENERGY_THRESHOLD) {
            detection_count++;
            ESP_LOGI(TAG, "Sound detected! Energy=%.2f (count=%d)", energy, detection_count);
            
            // Log all coefficients for analysis
            ESP_LOGI(TAG, "MFCC: [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f]", 
                     mfcc_features[0], mfcc_features[1], mfcc_features[2], 
                     mfcc_features[3], mfcc_features[4], mfcc_features[5], mfcc_features[6]);
        }
    }
}

// Example: Template matching callback
void template_matching_callback(float *mfcc_features, int num_coeffs, void *user_data)
{
    if (mfcc_features != NULL) {
        // Example template for a specific sound (you would load this from storage)
        static const float target_template[13] = {
            12.5f, 8.2f, 3.1f, 2.8f, 1.9f, 1.5f, 1.2f, 
            0.8f, 0.6f, 0.4f, 0.3f, 0.2f, 0.1f
        };
        
        // Calculate Euclidean distance
        float distance = 0.0f;
        for (int i = 0; i < num_coeffs && i < 13; i++) {
            float diff = mfcc_features[i] - target_template[i];
            distance += diff * diff;
        }
        distance = sqrtf(distance);
        
        // Threshold for matching
        if (distance < 5.0f) {
            ESP_LOGI(TAG, "Template match found! Distance=%.2f", distance);
        }
    }
}

// Example function: Manual processing
void manual_processing_example(void)
{
    ESP_LOGI(TAG, "=== Manual Processing Example ===");
    
    // Configure audio processor
    audio_processor_config_t config = {0};
    config.mic_config = (i2s_mic_config_t){
        .sck_pin = GPIO_NUM_11,
        .ws_pin = GPIO_NUM_10,
        .sd_pin = GPIO_NUM_12,
        .sample_rate = 16000,
        .buffer_len = 1024,
        .bit_width = I2S_DATA_BIT_WIDTH_16BIT,
        .slot_mode = I2S_SLOT_MODE_MONO
    };
    // Use default MFCC config
    config.mfcc_config = default_mfcc_opts;
    
    // Initialize
    ESP_ERROR_CHECK(audio_processor_init(&audio_proc, &config));
    ESP_ERROR_CHECK(audio_processor_start(&audio_proc));
    
    // Process 10 frames manually
    for (int i = 0; i < 10; i++) {
        float *mfcc_features;
        esp_err_t ret = audio_processor_process_frame(&audio_proc, &mfcc_features);
        
        if (ret == ESP_OK && mfcc_features != NULL) {
            ESP_LOGI(TAG, "Frame %d: Energy=%.2f", i, mfcc_features[0]);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    

    audio_processor_stop(&audio_proc);
    audio_processor_destroy(&audio_proc);
}


void callback_processing_example(void)
{
    ESP_LOGI(TAG, "=== Callback Processing Example ===");
    
    audio_processor_config_t config = {0};
    config.mic_config = (i2s_mic_config_t){
        .sck_pin = GPIO_NUM_11,
        .ws_pin = GPIO_NUM_10,
        .sd_pin = GPIO_NUM_12,
        .sample_rate = 16000,
        .buffer_len = 1024,
        .bit_width = I2S_DATA_BIT_WIDTH_16BIT,
        .slot_mode = I2S_SLOT_MODE_MONO
    };
    config.mfcc_config = default_mfcc_opts;
    config.mfcc_callback = simple_detection_callback;
    config.user_data = NULL;
    
    ESP_ERROR_CHECK(audio_processor_init(&audio_proc, &config));
    ESP_ERROR_CHECK(audio_processor_start(&audio_proc));
    

    ESP_LOGI(TAG, "Listening for sounds... (30 seconds)");
    vTaskDelay(pdMS_TO_TICKS(30000));
    

    audio_processor_stop(&audio_proc);
    audio_processor_destroy(&audio_proc);
    
    ESP_LOGI(TAG, "Total detections: %d", detection_count);
}


void individual_components_example(void)
{
    ESP_LOGI(TAG, "=== Individual Components Example ===");
    
    // Initialize I2S microphone
    i2s_mic_t mic;
    ESP_ERROR_CHECK(i2s_mic_init(&mic, NULL)); // Use default config
    ESP_ERROR_CHECK(i2s_mic_start(&mic));
    
    // Initialize MFCC processor
    mfcc_processor_t mfcc;
    ESP_ERROR_CHECK(mfcc_processor_init(&mfcc, NULL)); // Use default config
    
    // Process a few frames
    for (int i = 0; i < 5; i++) {
        // Read audio samples
        size_t samples_read;
        int16_t *samples = i2s_mic_read(&mic, &samples_read, 1000);
        
        if (samples != NULL && samples_read > 0) {
            // Process through MFCC
            float *mfcc_features = mfcc_processor_run(&mfcc, samples, 1);
            
            if (mfcc_features != NULL) {
                ESP_LOGI(TAG, "Frame %d: %d samples -> MFCC[0]=%.2f", 
                         i, samples_read, mfcc_features[0]);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    // Cleanup
    i2s_mic_stop(&mic);
    i2s_mic_destroy(&mic);
    mfcc_processor_destroy(&mfcc);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Audio Classification Examples");
    ESP_LOGI(TAG, "============================");
    
    manual_processing_example();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Example 2: Callback-based processing
    callback_processing_example();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Example 3: Individual components
    individual_components_example();
    
    ESP_LOGI(TAG, "All examples completed!");
    
    // Keep the task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
