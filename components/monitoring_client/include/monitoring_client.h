#include <stdio.h>
typedef float (*measurement_callback_t)(int num_measurements);
typedef void (*send_data_callback_t)(const char *packet);

void startMeasurement(int max_measurements, measurement_callback_t measurement_callback, send_data_callback_t send_data_callback);
void client_start(float (*measurement_callback)(int num_measurements));