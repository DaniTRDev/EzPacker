#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void analyze_sensor(SensorRecord *rec) {
    int sum = 0;
    int min_val = rec->readings[0];
    int max_val = rec->readings[0];

    for (int i = 0; i < rec->sample_count; i++) {
        int val = rec->readings[i];
        sum += val;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    rec->min_reading = min_val;
    rec->max_reading = max_val;
    int avg = sum / rec->sample_count;
    rec->avg_reading = avg;

    int spread = max_val - min_val;
    int score = 100 - spread;
    if (score < 0) score = 0;
    if (score > 100) score = 100;
    rec->health_score = score;
}

int main(void) {
    SensorRecord rec;
    memset(&rec, 0, sizeof(SensorRecord));
    rec.sample_count = 6;
    rec.readings[0] = 42;
    rec.readings[1] = 44;
    rec.readings[2] = 43;
    rec.readings[3] = 45;
    rec.readings[4] = 42;
    rec.readings[5] = 43;

    analyze_sensor(&rec);
    printf("done analyze: avg=%d, score=%d\n", rec.avg_reading, rec.health_score);
    assert(rec.min_reading == 42);
    assert(rec.max_reading == 45);
    assert(rec.avg_reading == 43);
    assert(rec.health_score == 97);
    return 0;
}
