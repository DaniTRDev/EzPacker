#include "os/os_support.h"
#include <string.h>

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function) {
    char msg[512];
    msg[0] = '\0';
    strcat(msg, "Assertion failed: ");
    if (assertion) strcat(msg, assertion);
    strcat(msg, " (");
    if (file) strcat(msg, file);
    strcat(msg, ":");
    // Append line number
    char line_buf[16];
    int idx = 0;
    unsigned int temp = line;
    if (temp == 0) line_buf[idx++] = '0';
    else {
        char rev[16];
        int r = 0;
        while (temp > 0) {
            rev[r++] = (char)('0' + (temp % 10));
            temp /= 10;
        }
        while (r > 0) line_buf[idx++] = rev[--r];
    }
    line_buf[idx] = '\0';
    strcat(msg, line_buf);
    if (function) {
        strcat(msg, " in ");
        strcat(msg, function);
    }
    strcat(msg, ")\n");

    ez_os_write_stderr(msg, strlen(msg));
    ez_os_exit(1);
}
