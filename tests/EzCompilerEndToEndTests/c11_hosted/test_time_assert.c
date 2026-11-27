#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

int main(void) {
    time_t now = time(NULL);
    assert(now > (time_t)0);

    // Deterministic epoch timestamp conversion: 2024-01-01 00:00:00 UTC = 1704067200
    time_t epoch_sample = (time_t)1704067200;
    struct tm *utc = gmtime(&epoch_sample);
    assert(utc != NULL);
    assert(utc->tm_year == 124); // 2024 - 1900
    assert(utc->tm_mon == 0);    // January
    assert(utc->tm_mday == 1);
    assert(utc->tm_hour == 0);
    assert(utc->tm_min == 0);
    assert(utc->tm_sec == 0);

    char tbuf[64];
    size_t n = strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", utc);
    assert(n == 19);
    assert(strcmp(tbuf, "2024-01-01 00:00:00") == 0);

    // Roundtrip via localtime & mktime (use memcpy to copy struct tm portably)
    struct tm *lt = localtime(&epoch_sample);
    assert(lt != NULL);
    struct tm local_copy;
    memcpy(&local_copy, lt, sizeof(struct tm));
    time_t rebuilt = mktime(&local_copy);
    assert(rebuilt == epoch_sample);

    puts("time_assert ok");
    return 0;
}
