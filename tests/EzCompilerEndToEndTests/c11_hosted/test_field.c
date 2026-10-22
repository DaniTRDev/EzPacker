#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    int sensor_id;
} SensorRecord;

typedef struct {
    SensorRecord *records;
    size_t count;
} TelemetryBatch;

int main(void) {
    TelemetryBatch *batch = (TelemetryBatch *)malloc(sizeof(TelemetryBatch));
    assert(batch != NULL);
    batch->records = (SensorRecord *)malloc(8 * sizeof(SensorRecord));
    assert(batch->records != NULL);
    batch->count = 0;

    printf("A: batch->count=%llu\n", (unsigned long long)batch->count);
    SensorRecord *rec = &batch->records[batch->count];
    printf("B: rec=%p\n", rec);
    rec->sensor_id = 101;
    printf("C: rec->sensor_id=%d\n", rec->sensor_id);
    assert(rec->sensor_id == 101);

    int id = 100 + (int)batch->count + 1;
    rec->sensor_id = id;
    printf("D: rec->sensor_id=%d\n", rec->sensor_id);
    assert(rec->sensor_id == 101);

    free(batch->records);
    free(batch);
    return 0;
}
