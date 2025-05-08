#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <crypt.h>
#include <string.h>
#define WRONG_NUMARG -2
#define PASSWD_NFND -3

// salt= \$6\$5MfvmFOaDU\$

int main(int argc, char * argv[]) {
    if (argc != 3) {
        fprintf(stderr, "[Error] Wrong number of command line arguments <%d>. Should be 2 [argc: 3]\n", argc - 1);
        return WRONG_NUMARG;
    }

    char *passwd = argv[1];
    char *salt = argv[2];
    struct crypt_data _crypt_data;

    printf("passwd:  %s\n", passwd);
    printf("salt:    %s\n", salt);

    char *encrpt = NULL; 
    if ((encrpt = crypt_r(passwd, salt, &_crypt_data)) == NULL) {
        fprintf(stderr, "Error matching password: password not found [%s]", encrpt);
        return PASSWD_NFND;
    }
    printf("Encrypted: %s \n", encrpt);


    const char *rl_res = "$6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1";
    printf("Correct: %d\n", strcmp(rl_res, encrpt));


    return 0;
}