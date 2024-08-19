#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "monitoring_client.h"
#include "thread_child.h"

#include "monitoring_utils.h"

void setDeepSleep(int sleep_time_sec)
{

    printf("Setting deep sleep with timer wakeup, %ds\n", sleep_time_sec);
    esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000);
}

void startDeepSleep()
{
    printf("Sleeping with timer wakeup");
    esp_deep_sleep_start();
}

void resetIterationCounter()
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_i32(my_handle, "iteration", 0));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}
void saveMeasurement(float value, int iteration)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle));
    char key[16];
    snprintf(key, sizeof(key), "value_%d", iteration);
    ESP_ERROR_CHECK(nvs_set_i32(my_handle, "iteration", iteration));
    ESP_ERROR_CHECK(nvs_set_blob(my_handle, key, &value, sizeof(float)));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}

void saveParameters(int measurements, int period, int date)
{
    printf("Saving parameters\n");
    printf("Measurements: %d\n", measurements);
    printf("Period: %d\n", period);
    printf("Date: %d\n", date);
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_i32(my_handle, "max_mes", measurements));
    ESP_ERROR_CHECK(nvs_set_i32(my_handle, "int_sec", period));
    ESP_ERROR_CHECK(nvs_set_i32(my_handle, "date", date));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}

void getParameters(int *measurements, int *period, int *date)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle));
    *measurements = 0;
    *period = 0;
    *date = 0;
    esp_err_t err = nvs_get_i32(my_handle, "max_mes", measurements);
    if (err == ESP_OK)
    {
        err = nvs_get_i32(my_handle, "int_sec", period);
        if (err == ESP_OK)
        {
            err = nvs_get_i32(my_handle, "date", date);
        }
    }
    if (err == ESP_OK)
    {
        return;
    }
    else
    {
        printf("Error reading parameters\n");
    }
}

float *getMeasurements(int max_measurements)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle));
    int *count = malloc(sizeof(int));
    float *values = malloc(max_measurements * sizeof(float));
    *count = 0;

    for (int i = 0; i < max_measurements; i++)
    {
        char key[32];
        snprintf(key, sizeof(key), "value_%d", i);
        float temp;
        size_t length = sizeof(float);
        esp_err_t err = nvs_get_blob(my_handle, key, &temp, &length);
        if (err == ESP_OK)
        {
            values[*count] = temp;
            (*count)++;
        }
    }

    nvs_close(my_handle);
    return values;
}

int32_t getIterationCounter()
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle));
    int iteration;
    esp_err_t err = nvs_get_i32(my_handle, "iteration", &iteration);
    if (err == ESP_OK)
    {
        return iteration;
    }
    else
    {
        return 0;
    }
}
void setMeasurementCount(int count)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_i32(my_handle, "count", count));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}

void resetMeasurements(int num_measurements)
{
    setMeasurementCount(0);
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle));
    for (int i = 0; i < num_measurements; i++)
    {
        char key[16];
        snprintf(key, sizeof(key), "temp_%d", i);
        nvs_erase_key(my_handle, key);
    }
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}
