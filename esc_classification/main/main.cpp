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
#include <algorithm>
#include <inttypes.h>
extern "C" {
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "string.h"
#include <esp_mfcc_iface.h>
#include <esp_mfcc_models.h>
#include "model.h"
}
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#define I2S_SD 12
#define I2S_WS 10
#define I2S_SCK 11
#define NUM_LABELS MODEL_OUTPUT_SIZE

static uint8_t* tensor_arena = nullptr;
#define bufferLen 1024
#define SAMPLE_RATE 16000
#define LABEL_COUNT 4
#define TAG "INMP441"

int16_t sBuffer[bufferLen];
i2s_chan_handle_t rx_handle;

const char* labels[LABEL_COUNT] = {
    "Fan", "Washing machine", "Vacuum cleaner", "Noise"
};

const char* classify_mfcc(const float* mfcc_input) {
    tflite::ErrorReporter* error_reporter = tflite::GetMicroErrorReporter();

    const tflite::Model* model_ptr = tflite::GetModel(model);
    if (model_ptr->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report("Model schema version mismatch");
        return "InvalidModel";
    }

    static tflite::MicroMutableOpResolver<13> resolver;
    static bool resolver_initialized = false;
    if (!resolver_initialized) {
        resolver.AddConv2D();         
        resolver.AddFullyConnected();
        resolver.AddSoftmax();
        resolver.AddReshape();
        resolver.AddAdd();
        resolver.AddMul();
        resolver.AddExpandDims();     
        resolver.AddRelu();           
        resolver.AddMaxPool2D();      
        resolver.AddAveragePool2D();  
        resolver_initialized = true;
    }
    static tflite::MicroInterpreter* interpreter = nullptr;
    static bool interpreter_initialized = false;
    
    if (!interpreter_initialized) {
        if (tensor_arena == nullptr) {
            tensor_arena = (uint8_t*)heap_caps_malloc(MODEL_TENSOR_ARENA_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (tensor_arena == nullptr) {
                printf("Failed to allocate tensor arena in PSRAM, trying internal RAM...\n");
                tensor_arena = (uint8_t*)malloc(MODEL_TENSOR_ARENA_SIZE);
                if (tensor_arena == nullptr) {
                    printf("Failed to allocate tensor arena!\n");
                    return "MemoryError";
                }
            } else {
                printf("Tensor arena allocated in PSRAM: %p\n", tensor_arena);
            }
        }
        
        printf("Initializing TensorFlow Lite interpreter...\n");
        interpreter = new tflite::MicroInterpreter(
            model_ptr, resolver, tensor_arena, MODEL_TENSOR_ARENA_SIZE
        );
        
        printf("Allocating tensors (arena size: %d bytes)...\n", MODEL_TENSOR_ARENA_SIZE);
        TfLiteStatus allocate_status = interpreter->AllocateTensors();
        if (allocate_status != kTfLiteOk) {
            error_reporter->Report("AllocateTensors() failed");
            printf("AllocateTensors failed with status: %d\n", allocate_status);
            return "AllocateError";
        }
        printf("TensorFlow Lite interpreter initialized successfully!\n");
        
        TfLiteTensor* input_tensor = interpreter->input(0);
        TfLiteTensor* output_tensor = interpreter->output(0);
        
        printf("Input tensor info:\n");
        printf("  Type: %s\n", input_tensor->type == kTfLiteInt8 ? "int8" : "other");
        printf("  Scale: %f\n", input_tensor->params.scale);
        printf("  Zero point: %" PRId32 "\n", input_tensor->params.zero_point);
        
        printf("Output tensor info:\n");  
        printf("  Type: %s\n", output_tensor->type == kTfLiteInt8 ? "int8" : "other");
        printf("  Scale: %f\n", output_tensor->params.scale);
        printf("  Zero point: %" PRId32 "\n", output_tensor->params.zero_point);
        
        interpreter_initialized = true;
    }
    TfLiteTensor* input = interpreter->input(0);
    if (input == nullptr || input->type != kTfLiteInt8) {
        error_reporter->Report("Invalid input tensor - expected int8");
        return "InvalidInput";
    }

    float input_scale = input->params.scale;
    int32_t input_zero_point = input->params.zero_point;

    printf("Quantizing MFCC input (scale: %f, zero_point: %" PRId32 "):\n", input_scale, input_zero_point);
    

    printf("First 10 MFCC values: ");
    for (int i = 0; i < 10 && i < MODEL_INPUT_SIZE; i++) {
        printf("%.4f ", mfcc_input[i]);
    }
    printf("\n");
    

    float mfcc_min = mfcc_input[0], mfcc_max = mfcc_input[0], mfcc_sum = 0;
    for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
        if (mfcc_input[i] < mfcc_min) mfcc_min = mfcc_input[i];
        if (mfcc_input[i] > mfcc_max) mfcc_max = mfcc_input[i];
        mfcc_sum += mfcc_input[i];
    }
    float mfcc_mean = mfcc_sum / MODEL_INPUT_SIZE;
    printf("MFCC stats - Min: %.4f, Max: %.4f, Mean: %.4f\n", mfcc_min, mfcc_max, mfcc_mean);
    
    for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
        int32_t quantized_value = static_cast<int32_t>(round(mfcc_input[i] / input_scale + input_zero_point));
        quantized_value = std::max(static_cast<int32_t>(-128), std::min(static_cast<int32_t>(127), quantized_value));
        input->data.int8[i] = static_cast<int8_t>(quantized_value);
        if (i < 5) { 
            printf("  MFCC[%d]: %f -> %d\n", i, mfcc_input[i], input->data.int8[i]);
        }
    }
    
    // Debug: Check quantize
    int count_neg128 = 0, count_pos127 = 0, count_normal = 0;
    for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
        if (input->data.int8[i] == -128) count_neg128++;
        else if (input->data.int8[i] == 127) count_pos127++;
        else count_normal++;
    }
    printf("Quantized distribution - Normal: %d, Saturated low (-128): %d, Saturated high (127): %d\n", 
           count_normal, count_neg128, count_pos127);

    if (interpreter->Invoke() != kTfLiteOk) {
        error_reporter->Report("Inference failed");
        return "InvokeError";
    }

    TfLiteTensor* output = interpreter->output(0);
    if (output == nullptr || output->type != kTfLiteInt8) {
        error_reporter->Report("Invalid output tensor - expected int8");
        return "OutputError";
    }

    float output_scale = output->params.scale;
    int32_t output_zero_point = output->params.zero_point;


    int max_index = 0;
    float max_score = (output->data.int8[0] - output_zero_point) * output_scale;

    printf("Dequantized output scores:\n");
    printf("  %s: %f (raw: %d)\n", labels[0], max_score, output->data.int8[0]);
    
    for (int i = 1; i < NUM_LABELS; i++) {
        float score = (output->data.int8[i] - output_zero_point) * output_scale;
        printf("  %s: %f (raw: %d)\n", labels[i], score, output->data.int8[i]);
        if (score > max_score) {
            max_score = score;
            max_index = i;
        }
    }
    
    printf("Raw output values: [%d, %d, %d, %d]\n", 
           output->data.int8[0], output->data.int8[1], 
           output->data.int8[2], output->data.int8[3]);
    printf("Output scale: %f, zero_point: %" PRId32 "\n", output_scale, output_zero_point);
    
    printf("Predicted class: %s (score: %f)\n", labels[max_index], max_score);

    return labels[max_index];
}
esp_mfcc_opts_t opts = {
    .winstep_ms = 20,        
    .winlen_ms = 20,         
    .nch = 1,                
    .numcep = 13,            
    .nfilter = 32,           
    .nfft = 2048,            
    .samp_freq = 16000,     
    .low_freq = 300,         
    .high_freq = 0,       
    .preemph = 0.98f,        
    .win_type = (char*)"hamming",  
    .append_energy = true,   
    .use_power = true,     
    .use_log_fbank = 1,      
    .log_epsilon = 1e-7,     
    .psram_first = false,
    .remove_dc_offset = true
};

const esp_mfcc_iface_t *mfcc_iface = &esp_fbank_f32;

void i2s_std_config(void);

extern "C" void app_main(void)
{
    i2s_std_config();
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    
    esp_mfcc_data_t *runner = mfcc_iface->create(&opts);
    if (runner == NULL) {
        printf("Không thể khởi tạo runner MFCC\n");
        return;
    }
    size_t bytesIn = 0;
    esp_err_t ret;
    
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
            int samples_received = bytesIn / sizeof(int16_t);
            ESP_LOGD(TAG, "Received %d samples (%d bytes)", samples_received, bytesIn);
            
            if (samples_received > 0) {
                float *mfcc_result = mfcc_iface->run_step(runner, sBuffer, 1);  // 1 là số kênh (mono)
                if (mfcc_result != NULL) {
                    const char* result = classify_mfcc(mfcc_result);
                    printf("Classification result: %s\n", result);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (runner != NULL) {
        mfcc_iface->destroy(runner);
    }
}

void i2s_std_config(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,  
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = I2S_SLOT_MODE_MONO,            
            .slot_mask = I2S_STD_SLOT_LEFT,              
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
