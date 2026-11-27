#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

int main(void) {
    pid_t pid = getpid();
    assert(pid > 0);

    const char *path = "ez_musl_posix_tmp.dat";
    unlink(path);

    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    assert(fd >= 0);

    const char *payload = "POSIX-musl-fd-test";
    ssize_t len = (ssize_t)strlen(payload);
    assert(write(fd, payload, (size_t)len) == len);
    assert(lseek(fd, 0, SEEK_SET) == 0);

    char *buf = (char *)malloc(64);
    assert(buf != NULL);
    memset(buf, 0, 64);
    assert(read(fd, buf, 63) == len);
    assert(strcmp(buf, payload) == 0);
    free(buf);

    /* Exercise fdopen + fclose alongside POSIX open/read/write/malloc
       (regression test for Win32 COFF archive duplicate-symbol collisions). */
    FILE *fp = fdopen(fd, "r+");
    assert(fp != NULL);
    assert(fclose(fp) == 0);

    struct stat st;
    assert(stat(path, &st) == 0);
    assert(st.st_size == (off_t)len);
    assert(unlink(path) == 0);

    printf("posix_musl ok pid=%lld\n", (long long)pid);
    return 0;
}
