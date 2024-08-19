#include <stdio.h>

#define NVS_NAMESPACE "storage"
#define MAX_ITERATIONS 4

typedef struct
{
    float temperature;
    time_t timestamp;
} TemperatureMeasurement;

void startDeepSleep();
void setDeepSleep(int sleep_time_sec);
void resetIterationCounter();
void saveMeasurement(float temperature, int iteration);
float *getMeasurements(int max_measurements);
int32_t getIterationCounter();
void setMeasurementCount(int count);
void resetMeasurements(int num_measurements);
void saveParameters(int measurements, int period, int date);
void getParameters(int *measurements, int *period, int *date);