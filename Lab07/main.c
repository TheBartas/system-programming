#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
    int t_flag = 0, n_flag = 0;
    int ret, t, n;
    const char *optstring = "n:t:";

    while((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 'n': n = atoi(optarg); n_flag = 1; break;
            case 't': t = atoi(optarg); t_flag = 1; break;
            case '?': return -1;
            default: abort();
        }
    }

    if (t_flag == 0 || n_flag == 0) {
        fprintf(stderr, "[Error] Need to use both of flags! Used: [-t] <%d> | [-n] <%d>.\n", t_flag, n_flag);
        return -1;
    }

    if (t < 0 || n < 0) {
        fprintf(stderr, "[Error] Time [-t] <%d> and number [-n] <%d> cannot be less than 0!\n", t, n);
        return -1;
    } 

    int *times = NULL;
    times = malloc(sizeof * times * n);

    for (int i = 0; i < n; i++) {
        times[i] = rand() % n + 1;
    }

    for (int i = 0; i < n; i++) {
        printf("%d\n", times[i]);
    }



    printf("Gut!\n");
    free(times);
    return 0;
}