#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    char buf[64];
    memset(buf, 'X', sizeof(buf));
    assert(buf[0] == 'X' && buf[63] == 'X');

    strcpy(buf, "EzPacker");
    assert(strlen(buf) == 8);
    assert(strnlen(buf, 4) == 4);
    assert(strnlen(buf, 32) == 8);

    strcat(buf, "-Musl");
    assert(strcmp(buf, "EzPacker-Musl") == 0);
    assert(strncmp(buf, "EzPacker-XYZ", 9) == 0);
    assert(strchr(buf, '-') == buf + 8);
    assert(strrchr(buf, 'e') == buf + 6);
    assert(strstr(buf, "Musl") == buf + 9);
    assert(strstr(buf, "Missing") == NULL);

    // Overlapping memmove (forward & backward)
    char ov[32] = "0123456789ABCDEF";
    memmove(ov + 4, ov, 10); // forward overlap: dst > src
    assert(memcmp(ov, "01230123456789EF", 16) == 0);

    strcpy(ov, "0123456789ABCDEF");
    memmove(ov, ov + 4, 10); // backward overlap: dst < src
    assert(memcmp(ov, "456789ABCDABCDEF", 16) == 0);

    // memchr & memcmp
    const char *mc = (const char *)memchr("abcdef", 'd', 6);
    assert(mc != NULL && *mc == 'd');
    assert(memchr("abcdef", 'z', 6) == NULL);
    assert(memcmp("abc", "abd", 2) == 0);
    assert(memcmp("abc", "abd", 3) < 0);

    // strtok + strdup
    char *dup = strdup("alpha,beta,gamma");
    assert(dup != NULL);
    char *tok = strtok(dup, ",");
    assert(tok && strcmp(tok, "alpha") == 0);
    tok = strtok(NULL, ",");
    assert(tok && strcmp(tok, "beta") == 0);
    tok = strtok(NULL, ",");
    assert(tok && strcmp(tok, "gamma") == 0);
    assert(strtok(NULL, ",") == NULL);
    free(dup);

    puts("string_mem ok");
    return 0;
}
