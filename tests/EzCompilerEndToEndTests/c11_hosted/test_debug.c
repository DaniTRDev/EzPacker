#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define MAX_SENSORS 8
#define MAX_NAME_LEN 32
#define MAX_SAMPLES 6

typedef struct {
    int sensor_id;
    char name[MAX_NAME_LEN];
    char tag[16];
    int readings[MAX_SAMPLES];
    int sample_count;
    int min_reading;
    int max_reading;
    int avg_reading;
    int health_score;
} SensorRecord;

typedef struct {
    SensorRecord *records;
    size_t count;
    size_t capacity;
    time_t batch_timestamp;
} TelemetryBatch;

int main(void) {
    printf("Entering main\n");
    TelemetryBatch *batch = (TelemetryBatch *)malloc(sizeof(TelemetryBatch));
    assert(batch != NULL);
    batch->records = (SensorRecord *)malloc(MAX_SENSORS * sizeof(SensorRecord));
    assert(batch->records != NULL);
    batch->count = 0;
    batch->capacity = MAX_SENSORS;
    batch->batch_timestamp = time(NULL);

    SensorRecord *rec = &batch->records[0];
    memset(rec, 0, sizeof(SensorRecord));
    rec->sensor_id = 101;
    strncpy(rec->name, "Core Temp", MAX_NAME_LEN - 1);
    rec->sample_count = 6;
    rec->readings[0] = 42;
    rec->readings[1] = 44;
    rec->readings[2] = 43;
    rec->readings[3] = 45;
    rec->readings[4] = 42;
    rec->readings[5] = 43;

    int sum = 0;
    int min_val = rec->readings[0];
    int max_val = rec->readings[0];
    for (int i = 0; i < rec->sample_count; i++) {
        int v = rec->readings[i];
        sum += v;
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
    }
    rec->min_reading = min_val;
    rec->max_reading = max_val;
    rec->avg_reading = sum / rec->sample_count;
    rec->health_score = 100 - (max_val - min_val);

    printf("Sensor ID: %d, Name: %s\n", rec->sensor_id, rec->name);
    printf("Reading Min: %d, Max: %d\n", rec->min_reading, rec->max_reading);
    printf("Analysis Avg: %d, Health Score: %d%%\n", rec->avg_reading, rec->health_score);

    assert(rec->min_reading == 42);
    assert(rec->max_reading == 45);
    assert(rec->avg_reading == 43);
    assert(rec->health_score == 97);

    free(batch->records);
    free(batch);
    printf("Clean exit\n");
    return 0;
}
