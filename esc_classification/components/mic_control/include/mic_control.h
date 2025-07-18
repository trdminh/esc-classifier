#pragma once

// Main header - include this to use the mic_control library
#include "audio_processor.h"

// Individual components (if needed)
#include "i2s_mic.h"
#include "mfcc.h"

// TensorFlow Lite classifier (C-compatible interface)
#ifdef __cplusplus
#include "tflite_classifier.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible classifier interface for pure C files
#ifndef __cplusplus
typedef struct tflite_classifier_t tflite_classifier_t;
typedef struct {
    int class_id;
    float confidence;
    const char* class_name;
} classification_result_t;

// C-compatible function declarations
esp_err_t tflite_classifier_init_c(tflite_classifier_t** classifier);
esp_err_t tflite_classifier_classify_c(tflite_classifier_t* classifier, 
                                     float* mfcc_features, 
                                     int num_coeffs,
                                     classification_result_t* result);
void tflite_classifier_destroy_c(tflite_classifier_t* classifier);
#endif

// Legacy compatibility (if needed)
#define I2S_SAMPLE_RATE    16000
#define I2S_BUFFER_SIZE    1024
#define I2S_NUM_CHANNELS   1

#ifdef __cplusplus
}
#endif
