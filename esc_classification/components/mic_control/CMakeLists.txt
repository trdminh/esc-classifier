idf_component_register(
    SRCS "audio_processor.c" "i2s_mic.c" "mfcc.c"
    INCLUDE_DIRS "include"
    REQUIRES esp-sr driver freertos esp-tflite-micro
)
