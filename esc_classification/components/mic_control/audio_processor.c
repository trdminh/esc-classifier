#include "audio_processor.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "AUDIO_PROCESSOR";

// Processing task
static void audio_processor_task(void *arg)
{
    audio_processor_t *processor = (audio_processor_t *)arg;
    
    while (processor->is_running) {
        size_t samples_read;
        int16_t *samples = i2s_mic_read(&processor->mic, &samples_read, 1000); // 1 second timeout
        
        if (samples != NULL && samples_read > 0) {
            // Process through MFCC
            float *mfcc_features = mfcc_processor_run(&processor->mfcc, samples, 1);
            
            if (mfcc_features != NULL && processor->mfcc_callback != NULL) {
                int num_coeffs = mfcc_processor_get_num_coeffs(&processor->mfcc);
                processor->mfcc_callback(mfcc_features, num_coeffs, processor->user_data);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to prevent watchdog timeout
    }
    
    vTaskDelete(NULL);
}

esp_err_t audio_processor_init(audio_processor_t *processor, const audio_processor_config_t *config)
{
    if (processor == NULL) {
        ESP_LOGE(TAG, "Processor handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(processor, 0, sizeof(audio_processor_t));
    
    esp_err_t ret;
    
    // Initialize I2S microphone
    const i2s_mic_config_t *mic_config = (config != NULL) ? &config->mic_config : NULL;
    ret = i2s_mic_init(&processor->mic, mic_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S microphone");
        return ret;
    }
    
    // Initialize MFCC processor
    const esp_mfcc_opts_t *mfcc_config = (config != NULL) ? &config->mfcc_config : NULL;
    ret = mfcc_processor_init(&processor->mfcc, mfcc_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MFCC processor");
        i2s_mic_destroy(&processor->mic);
        return ret;
    }
    
    // Set callback if provided
    if (config != NULL) {
        processor->mfcc_callback = config->mfcc_callback;
        processor->user_data = config->user_data;
    }
    
    ESP_LOGI(TAG, "Audio processor initialized successfully");
    return ESP_OK;
}

esp_err_t audio_processor_start(audio_processor_t *processor)
{
    if (processor == NULL) {
        ESP_LOGE(TAG, "Processor handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (processor->is_running) {
        ESP_LOGW(TAG, "Audio processor already running");
        return ESP_OK;
    }
    
    // Start I2S microphone
    esp_err_t ret = i2s_mic_start(&processor->mic);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start I2S microphone");
        return ret;
    }
    
    // Start processing task
    processor->is_running = true;
    BaseType_t task_ret = xTaskCreate(audio_processor_task, "audio_proc", 4096, processor, 5, &processor->task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio processing task");
        processor->is_running = false;
        i2s_mic_stop(&processor->mic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Audio processor started");
    return ESP_OK;
}

esp_err_t audio_processor_stop(audio_processor_t *processor)
{
    if (processor == NULL) {
        ESP_LOGE(TAG, "Processor handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!processor->is_running) {
        ESP_LOGW(TAG, "Audio processor not running");
        return ESP_OK;
    }
    
    // Stop processing task
    processor->is_running = false;
    if (processor->task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(200)); // Give task time to exit
        processor->task_handle = NULL;
    }
    
    // Stop I2S microphone
    esp_err_t ret = i2s_mic_stop(&processor->mic);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop I2S microphone");
        return ret;
    }
    
    ESP_LOGI(TAG, "Audio processor stopped");
    return ESP_OK;
}

esp_err_t audio_processor_process_frame(audio_processor_t *processor, float **mfcc_features)
{
    if (processor == NULL) {
        ESP_LOGE(TAG, "Processor handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t samples_read;
    int16_t *samples = i2s_mic_read(&processor->mic, &samples_read, 1000);
    
    if (samples == NULL || samples_read == 0) {
        ESP_LOGD(TAG, "No samples available");
        if (mfcc_features != NULL) {
            *mfcc_features = NULL;
        }
        return ESP_OK;
    }
    
    // Process through MFCC
    float *features = mfcc_processor_run(&processor->mfcc, samples, 1);
    
    if (mfcc_features != NULL) {
        *mfcc_features = features;
    }
    
    return ESP_OK;
}

void audio_processor_destroy(audio_processor_t *processor)
{
    if (processor == NULL) {
        return;
    }
    
    // Stop processing if running
    if (processor->is_running) {
        audio_processor_stop(processor);
    }
    
    // Clean up processors
    mfcc_processor_destroy(&processor->mfcc);
    i2s_mic_destroy(&processor->mic);
    
    memset(processor, 0, sizeof(audio_processor_t));
    ESP_LOGI(TAG, "Audio processor destroyed");
}

const esp_mfcc_opts_t *audio_processor_get_mfcc_config(audio_processor_t *processor)
{
    if (processor == NULL) {
        return NULL;
    }
    return &processor->mfcc.opts;
}
