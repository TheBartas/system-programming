#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char * argv[]) {
    int ind = atoi(argv[1]);

    for (int i = 0; i < ind; i++) {
        printf("%s\n", argv[2]);
        close(1);
        close(2);
        int h = open("/dev/null", O_WRONLY);
        dup2(h, 1);
        dup2(h, 2);
        printf("%s\n", argv[2]);
    }
}