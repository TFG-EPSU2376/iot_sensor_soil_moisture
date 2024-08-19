#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "driver/temperature_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "monitoring_client.h"
#include "thread_child.h"
#include "monitoring_utils.c"

#define MAX_PACKET_SIZE 256

void startMeasurement(int max_measurements, measurement_callback_t measurement_callback, send_data_callback_t send_data_callback)
{
    printf("Starting measurement\n");

    static const char *TAG = "IOT_SENSOR";
    ESP_ERROR_CHECK(nvs_flash_init());

    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle));

    int32_t iteration = getIterationCounter();
    printf("Iteration counter: %d\n", (int)iteration);
    float measured_value = measurement_callback(max_measurements);
    bool sendMeasurements = iteration >= max_measurements;
    if (sendMeasurements && send_data_callback)
    {

        float *all_measurements = getMeasurements(max_measurements);

        char packet[MAX_PACKET_SIZE] = {0};
        int offset = 0;
        for (int i = 0; i < max_measurements + 1 && offset < MAX_PACKET_SIZE - 10; i++)
        {
            offset += snprintf(packet + offset, MAX_PACKET_SIZE - offset, "%.2f,", all_measurements[i]);
        }
        offset += snprintf(packet + offset, MAX_PACKET_SIZE - offset, "%.2f,", measured_value);

        if (offset > 0)
        {
            packet[offset - 1] = '\0';
        }
        send_data_callback(packet);
        resetMeasurements(max_measurements);
        free(all_measurements);
        resetIterationCounter(0);
    }
    else
    {
        printf("Saving measurement value: %.2f\n", measured_value);
        saveMeasurement(measured_value, iteration + 1);
    }

    nvs_close(my_handle);
}

void sendThreadMessage(const char *packet)
{
    initialize_thread(false, NULL);
    char message[MAX_PACKET_SIZE];
    snprintf(message, sizeof(message), "{\"type\":\"data\", \"values\":[%s]}", packet);
    sendUdp(message);
}

esp_err_t first_start(float (*measurement_callback)(int num_measurements))
{
    void cb(const bool sended)
    {
        printf("Send thread message completed\n");
    }
    initialize_thread(true, cb);
    sendUdp("{\"type\":\"device_connected\",\"values\":[\"S_SM_1\"]}");

    int num_measurements = 2;
    int period = 10;
    int date = 0;
    saveParameters(num_measurements, period, date);
    setDeepSleep(period);
    resetMeasurements(num_measurements);
    setDeepSleep(2);
    startDeepSleep();
    return ESP_OK;
}
esp_err_t handleWakeUp(float (*measurement_callback)(int num_measurements))
{
    printf("Waking up\n");
    int num_measurements = 0;
    int period = 0;
    int date = 0;
    getParameters(&num_measurements, &period, &date);
    printf("Parameters: %d, %d, %d\n", num_measurements, period, date);
    setDeepSleep(period);

    void cb(const char *packet)
    {
        printf("CB star sending thread message\n");
        sendThreadMessage(packet);
    }
    startMeasurement(num_measurements, measurement_callback, cb);
    return ESP_OK;
}

void client_start(float (*measurement_callback)(int num_measurements))
{
    ESP_ERROR_CHECK(nvs_flash_init());

    esp_reset_reason_t reason = esp_reset_reason();

    switch (reason)
    {
    case ESP_RST_POWERON:
        printf("Inicio desde apagado (power-on)\n");
        first_start(measurement_callback);
        startDeepSleep();
        break;

    case ESP_RST_DEEPSLEEP:
        printf("Despertar desde sueño profundo\n");
        handleWakeUp(measurement_callback);
        startDeepSleep();
        break;

    default:
        break;
    }
}