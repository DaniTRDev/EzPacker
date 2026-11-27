#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    // Character classification including EOF and high-bit unsigned char
    assert(isalpha('A') && isalpha('z') && !isalpha('5'));
    assert(isdigit('0') && isdigit('9') && !isdigit('a'));
    assert(isalnum('9') && isalnum('K') && !isalnum('!'));
    assert(isspace(' ') && isspace('\n') && isspace('\t') && !isspace('x'));
    assert(ispunct('!') && ispunct('.') && !ispunct('A'));
    assert(isxdigit('0') && isxdigit('f') && isxdigit('F') && !isxdigit('g'));
    assert(islower('m') && !islower('M'));
    assert(isupper('M') && !isupper('m'));

    assert(tolower('Q') == 'q');
    assert(tolower('q') == 'q');
    assert(toupper('q') == 'Q');
    assert(toupper('Q') == 'Q');

    assert(!isalpha(EOF));
    assert(!isdigit(EOF));
    assert(tolower(EOF) == EOF);
    assert(!isalpha((unsigned char)0xFF));

    // errno & strerror
    errno = 0;
    assert(errno == 0);
    errno = EINVAL;
    assert(errno == EINVAL);
    const char *msg_inval = strerror(EINVAL);
    const char *msg_range = strerror(ERANGE);
    assert(msg_inval != NULL && strlen(msg_inval) > 0);
    assert(msg_range != NULL && strlen(msg_range) > 0);
    assert(strcmp(msg_inval, msg_range) != 0);

    puts("ctype_errno ok");
    return 0;
}
