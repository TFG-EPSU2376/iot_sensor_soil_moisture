#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "measurement_soil_moisture.h"
#include "monitoring_client.h"

static const char *TAG = "SENSOR_SOIL_MOISTURE";
void app_main(void)
{
    ESP_LOGI(TAG, "Starting soil moisture sensor");
    client_start(doSoilMoistureMeasurement);
}