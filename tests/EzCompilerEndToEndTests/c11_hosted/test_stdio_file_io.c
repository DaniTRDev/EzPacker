#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

int main(void) {
    const char *fname = "ez_musl_stdio_io_tmp.txt";
    remove(fname);

    // Missing file must return NULL and set errno == ENOENT
    errno = 0;
    FILE *missing = fopen("ez_musl_does_not_exist_12345.txt", "rb");
    assert(missing == NULL);
    assert(errno == ENOENT);

    // Write file
    FILE *fw = fopen(fname, "wb");
    assert(fw != NULL);
    const char *line1 = "Line1: musl stdio\n";
    const char *line2 = "Line2: fseek/ftell\n";
    assert(fwrite(line1, 1, strlen(line1), fw) == strlen(line1));
    assert(fwrite(line2, 1, strlen(line2), fw) == strlen(line2));
    assert(fflush(fw) == 0);
    assert(fclose(fw) == 0);

    // Read, seek, tell, rewind
    FILE *fr = fopen(fname, "rb");
    assert(fr != NULL);
    assert(fseek(fr, 0, SEEK_END) == 0);
    long total = ftell(fr);
    assert(total == (long)(strlen(line1) + strlen(line2)));

    rewind(fr);
    assert(ftell(fr) == 0);

    char rbuf[64];
    memset(rbuf, 0, sizeof(rbuf));
    assert(fgets(rbuf, sizeof(rbuf), fr) != NULL);
    assert(strcmp(rbuf, line1) == 0);

    memset(rbuf, 0, sizeof(rbuf));
    size_t nread = fread(rbuf, 1, sizeof(rbuf) - 1, fr);
    assert(nread == strlen(line2));
    assert(strcmp(rbuf, line2) == 0);
    assert(feof(fr) || fgetc(fr) == EOF);
    assert(fclose(fr) == 0);

    assert(remove(fname) == 0);
    puts("stdio_file_io ok");
    return 0;
}
