/**
 *
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */


#include "mic_control.h"
#include "esp_err.h"
esp_err_t mic_i2s_init(i2s_chan_handle_t handle)
{
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
	i2s_new_channel(&chan_cfg, NULL, &handle);
	i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
    .slot_cfg = {
        .data_bit_width = I2S_DATA_BIT_WIDTH_24BIT,
        .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
        .slot_mode = I2S_SLOT_MODE_STEREO,
        .slot_mask = I2S_STD_SLOT_BOTH,
        .ws_width = 32,
        .ws_pol = false,
        .bit_shift = false,
        .left_align = true,
        .big_endian = false,
        .bit_order_lsb = true,
    },
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = (gpio_num_t)I2S_PIN_CLK,
        .ws = (gpio_num_t)I2S_PIN_WS,
        .dout = I2S_GPIO_UNUSED,
        .din = (gpio_num_t)I2S_PIN_SD,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        	},
    	},
	};
	ESP_ERROR_CHECK(i2s_channel_init_std_mode(handle, &std_cfg));
	ESP_ERROR_CHECK(i2s_channel_enable(handle));
	return ESP_OK;
}


void mic_i2s_record(i2s_chan_handle_t handle, int sbuffer[I2S_BUFFER_SIZE], size_t bytes_in)
{
	esp_err_t ret = i2s_channel_read(handle, &sbuffer, I2S_BUFFER_SIZE * sizeof(int), &bytes_in, portMAX_DELAY);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Deu erro na leitura: %d", ret);
    }else
    {
        for (size_t i = 0; i < I2S_BUFFER_SIZE; i++)
        {
            int raw_data = sbuffer[i];
            int sample = (raw_data);
            ESP_LOGI(TAG, "sBuffer[%d]: %d, bytesIn: %d", i, sample, bytes_in);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        printf("\n");
        
    }

    vTaskDelay(pdMS_TO_TICKS(100));
	
	
}