#include "mfcc.h"
#include "esp_err.h"
#include "esp_mfcc_iface.h"
#include "esp_mfcc_models.h"
#include "esp_log.h"

static const char *TAG = "MFCC_PROCESSOR";

const esp_mfcc_opts_t default_mfcc_opts = {
    .winstep_ms = 20,        // Frame stride = 20ms
    .winlen_ms = 20,         // Frame length = 20ms
    .nch = 1,                // Mono
    .numcep = 13,            // Số hệ số MFCC cần lấy
    .nfilter = 32,           // Số bộ lọc Mel
    .nfft = 2048,            // FFT length
    .samp_freq = 16000,      // Tần số lấy mẫu 16kHz
    .low_freq = 300,         // Tần số thấp nhất trong Mel filterbank
    .high_freq = 8000,       // Tần số cao nhất trong Mel filterbank
    .preemph = 0.98f,        // Hệ số pre-emphasis
    .win_type = "hamming",   // Window type
    .append_energy = true,   // Lấy năng lượng vào MFCC[0]
    .use_power = true,       // Dùng công suất (power spectrum)
    .use_log_fbank = 1,      // Lấy log(fbank + ε)
    .log_epsilon = 1e-7,     // Tránh log(0)
    .psram_first = false,
    .remove_dc_offset = true
};

esp_err_t mfcc_processor_init(mfcc_processor_t *processor, const esp_mfcc_opts_t *opts)
{
    if (processor == NULL) {
        ESP_LOGE(TAG, "Processor handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }


    memset(processor, 0, sizeof(mfcc_processor_t));

    if (opts == NULL) {
        processor->opts = default_mfcc_opts;
    } else {
        processor->opts = *opts;
    }

    processor->mfcc_iface = &esp_fbank_f32;

    processor->runner = processor->mfcc_iface->create(&processor->opts);
    if (processor->runner == NULL) {
        ESP_LOGE(TAG, "Failed to create MFCC runner");
        return ESP_FAIL;
    }

    processor->is_initialized = true;
    ESP_LOGI(TAG, "MFCC processor initialized successfully");
    ESP_LOGI(TAG, "Config: %d coeffs, %dHz, %dms frame, %dms stride", 
             processor->opts.numcep, processor->opts.samp_freq, 
             processor->opts.winlen_ms, processor->opts.winstep_ms);

    return ESP_OK;
}

float *mfcc_processor_run(mfcc_processor_t *processor, int16_t *samples, int16_t num_channels)
{
    if (processor == NULL || !processor->is_initialized) {
        ESP_LOGE(TAG, "Processor not initialized");
        return NULL;
    }

    if (samples == NULL) {
        ESP_LOGE(TAG, "Samples buffer is NULL");
        return NULL;
    }


    float *mfcc_result = processor->mfcc_iface->run_step(processor->runner, samples, num_channels);
    
    if (mfcc_result != NULL) {
        ESP_LOGD(TAG, "MFCC processing successful");
    } else {
        ESP_LOGD(TAG, "MFCC processing returned NULL (may be normal for pipeline)");
    }

    return mfcc_result;
}

void mfcc_processor_clean(mfcc_processor_t *processor)
{
    if (processor == NULL || !processor->is_initialized) {
        ESP_LOGW(TAG, "Processor not initialized");
        return;
    }

    if (processor->mfcc_iface && processor->mfcc_iface->clean && processor->runner) {
        processor->mfcc_iface->clean(processor->runner);
        ESP_LOGD(TAG, "MFCC processor state cleaned");
    }
}

void mfcc_processor_destroy(mfcc_processor_t *processor)
{
    if (processor == NULL) {
        return;
    }

    if (processor->is_initialized && processor->mfcc_iface && processor->runner) {
        processor->mfcc_iface->destroy(processor->runner);
        ESP_LOGI(TAG, "MFCC processor destroyed");
    }

    memset(processor, 0, sizeof(mfcc_processor_t));
}

int mfcc_processor_get_num_coeffs(mfcc_processor_t *processor)
{
    if (processor == NULL || !processor->is_initialized) {
        return 0;
    }
    return processor->opts.numcep;
}

int mfcc_processor_get_samples_per_frame(mfcc_processor_t *processor)
{
    if (processor == NULL || !processor->is_initialized) {
        return 0;
    }
    return (processor->opts.samp_freq * processor->opts.winstep_ms) / 1000;
}