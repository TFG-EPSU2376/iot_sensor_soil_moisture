#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#include "measurement_soil_moisture.h"

#define SENSOR_ADC_UNIT ADC_UNIT_1
#define SENSOR_ADC_CHANNEL ADC_CHANNEL_0
#define SENSOR_ADC_ATTEN ADC_ATTEN_DB_11
#define POWER_PIN GPIO_NUM_0

#define MIN_ADC_VALUE 2675
#define MAX_ADC_VALUE 3950

static const char *TAG = "soil_moisture";

int interpolate_moisture(int adc_value)
{
    if (adc_value <= MIN_ADC_VALUE)
        return 100;
    if (adc_value >= MAX_ADC_VALUE)
        return 0;

    int moisture = ((long)(MAX_ADC_VALUE - adc_value) * 100) / (MAX_ADC_VALUE - MIN_ADC_VALUE);

    return moisture;
}

float doSoilMoistureMeasurement()
{
    // GPIO Init for sensor power
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << POWER_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0};
    gpio_config(&io_conf);

    // ADC Init
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = SENSOR_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // ADC Config
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = SENSOR_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, SENSOR_ADC_CHANNEL, &config));

    // Power on the sensor
    gpio_set_level(POWER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for sensor to stabilize

    // Read ADC
    int adc_reading = 0;
    for (int i = 0; i < 5; i++)
    {
        int temp_reading;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, SENSOR_ADC_CHANNEL, &temp_reading));
        adc_reading += temp_reading;
        vTaskDelay(pdMS_TO_TICKS(10)); // Small pause between readings
    }
    adc_reading /= 5;

    // Interpolate moisture
    int moisture = interpolate_moisture(adc_reading);
    ESP_LOGI(TAG, "ADC Reading: %d, Moisture: %d%%", adc_reading, moisture);
    gpio_set_level(POWER_PIN, 0);

    return moisture;
}