#include <stdio.h>
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

    char buf[64];
    memset(buf, 0, sizeof(buf));
    assert(read(fd, buf, sizeof(buf) - 1) == len);
    assert(strcmp(buf, payload) == 0);
    assert(close(fd) == 0);

    struct stat st;
    assert(stat(path, &st) == 0);
    assert(st.st_size == (off_t)len);
    assert(unlink(path) == 0);

    printf("posix_musl ok pid=%lld\n", (long long)pid);
    return 0;
}
