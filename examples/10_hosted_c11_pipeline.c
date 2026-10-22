#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#define MAX_SENSORS 6
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
    long long batch_timestamp;
} TelemetryBatch;

static void normalize_tag(char *dest, const char *src, size_t max_len) {
    size_t i = 0;
    while (src[i] != '\0' && i + 1 < max_len) {
        unsigned char c = (unsigned char)src[i];
        if (isalpha(c)) {
            dest[i] = (char)toupper(c);
        } else if (c == '-' || c == '.') {
            dest[i] = '_';
        } else {
            dest[i] = src[i];
        }
        i++;
    }
    dest[i] = '\0';
}

static void analyze_sensor(SensorRecord *rec) {
    assert(rec != NULL);
    assert(rec->sample_count > 0);

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
    rec->avg_reading = sum / rec->sample_count;

    int spread = max_val - min_val;
    int score = 100 - spread * 2;
    if (score < 0) score = 0;
    if (score > 100) score = 100;
    rec->health_score = score;
}

static TelemetryBatch *create_batch(size_t capacity) {
    assert(capacity > 0);
    TelemetryBatch *batch = (TelemetryBatch *)malloc(sizeof(TelemetryBatch));
    assert(batch != NULL);

    batch->records = (SensorRecord *)malloc(capacity * sizeof(SensorRecord));
    assert(batch->records != NULL);

    batch->count = 0;
    batch->capacity = capacity;
    batch->batch_timestamp = (long long)time(NULL);
    return batch;
}

static void add_sensor(TelemetryBatch *batch,
                       const char *name,
                       const char *tag,
                       int base_reading) {
    assert(batch != NULL);
    assert(batch->count < batch->capacity);

    size_t idx = batch->count;
    SensorRecord *rec = &batch->records[idx];
    memset(rec, 0, sizeof(SensorRecord));

    rec->readings[0] = base_reading;
    rec->readings[1] = base_reading + 2;
    rec->readings[2] = base_reading - 1;
    rec->readings[3] = base_reading + 4;
    rec->readings[4] = base_reading - 3;
    rec->readings[5] = base_reading + 1;
    rec->sample_count = MAX_SAMPLES;

    int id = 100 + (int)idx + 1;
    rec->sensor_id = id;
    strncpy(rec->name, name, MAX_NAME_LEN - 1);
    normalize_tag(rec->tag, tag, sizeof(rec->tag));

    analyze_sensor(rec);
    batch->count = idx + 1;
}

static int compare_by_reading(const void *a, const void *b) {
    const SensorRecord *ra = (const SensorRecord *)a;
    const SensorRecord *rb = (const SensorRecord *)b;
    if (ra->avg_reading < rb->avg_reading) return -1;
    if (ra->avg_reading > rb->avg_reading) return 1;
    return 0;
}

static void print_separator(void) {
    puts("+-----+----------------------+----------------+---------+---------+---------+--------+--------+");
}

static void print_batch_report(const TelemetryBatch *batch) {
    assert(batch != NULL);

    puts("\n==========================================================================================");
    puts("                    EZPACKER HOSTED C11 TELEMETRY PROCESSING REPORT                       ");
    puts("==========================================================================================");
    printf("Batch Timestamp : %lld\n", batch->batch_timestamp);
    printf("Total Channels  : %u active sensors\n", (unsigned int)batch->count);
    puts("Status          : Normal Operational Parameters\n");

    print_separator();
    puts("| ID  | Sensor Channel       | Tag            | Min     | Max     | Avg     | Health | Status |");
    print_separator();

    int total_sum = 0;
    int total_health = 0;

    for (size_t i = 0; i < batch->count; i++) {
        const SensorRecord *r = &batch->records[i];
        total_sum += r->avg_reading;
        total_health += r->health_score;

        const char *status_str = "[OK]  ";
        if (r->health_score < 75) {
            status_str = "[WARN]";
        }
        if (r->health_score < 50) {
            status_str = "[CRIT]";
        }

        printf("| %-3d | %-20s | %-14s |", r->sensor_id, r->name, r->tag);
        printf(" %-7d | %-7d | %-7d |", r->min_reading, r->max_reading, r->avg_reading);
        printf("   %2d%%  | %s |\n", r->health_score, status_str);
    }

    print_separator();

    if (batch->count > 0) {
        int avg_system_reading = total_sum / (int)batch->count;
        int avg_system_health = total_health / (int)batch->count;
        printf("System Averages : Metric Avg = %d, Overall Health = %d%%\n", avg_system_reading, avg_system_health);
    }
    puts("==========================================================================================\n");
}

static void run_channel_query(const TelemetryBatch *batch, const char *search_tag) {
    assert(batch != NULL);
    assert(search_tag != NULL);

    puts("[EzPacker Pipeline] Searching for target telemetry channel (strcmp)...");
    const SensorRecord *matched = NULL;
    for (size_t i = 0; i < batch->count; i++) {
        if (strcmp(batch->records[i].tag, search_tag) == 0) {
            matched = &batch->records[i];
            break;
        }
    }

    if (matched) {
        printf("[EzPacker Pipeline] Channel Match Found: ID %d -> '%s'\n", matched->sensor_id, matched->name);
        printf("[EzPacker Pipeline] Readings: Min=%d, Max=%d | ", matched->min_reading, matched->max_reading);
        printf("Avg=%d, Health=%d%%\n", matched->avg_reading, matched->health_score);
        puts("[EzPacker Pipeline] Tag Verification: [PASSED]");
    } else {
        printf("[EzPacker Pipeline] Channel '%s' not found.\n", search_tag);
    }
}

static void free_batch(TelemetryBatch *batch) {
    if (batch) {
        if (batch->records) {
            free(batch->records);
        }
        free(batch);
    }
}

int main(void) {
    puts("[EzPacker Pipeline] Initializing C11 Hosted Telemetry Pipeline...");

    TelemetryBatch *batch = create_batch(MAX_SENSORS);

    puts("[EzPacker Pipeline] Ingesting sensor telemetry streams...");
    add_sensor(batch, "CPU Core Temp", "temp.cpu.core", 48);
    add_sensor(batch, "GPU Thermal Diode", "temp.gpu.diode", 65);
    add_sensor(batch, "RAM Bus Voltage", "volt.ddr5.vrm", 12);
    add_sensor(batch, "NVMe Drive Sensor", "disk.nvme.temp", 36);
    add_sensor(batch, "Chassis Intake Fan", "rpm.fan.intake", 1450);
    add_sensor(batch, "Power Delivery Unit", "watt.pdu.draw", 320);

    printf("[EzPacker Pipeline] Successfully ingested %u records.\n", (unsigned int)batch->count);

    puts("[EzPacker Pipeline] Sorting telemetry streams by average reading (qsort)...");
    qsort(batch->records, batch->count, sizeof(SensorRecord), compare_by_reading);

    print_batch_report(batch);

    run_channel_query(batch, "TEMP_CPU_CORE");

    puts("\n[EzPacker Pipeline] Verifying pipeline memory integrity and releasing resources (free)...");
    free_batch(batch);

    puts("[EzPacker Pipeline] Pipeline test completed successfully.");
    return 0;
}
