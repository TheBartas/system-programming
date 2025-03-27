// PS IS1 32O LAB04
// Bartłomiej Szewczyk
// sb53955@zut.edu.pl
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#define BILLION  1000000000L



void setcmdcom(char * args[], char * argv[], int optind, int argc) {
    int ind = 0;
    for (int i = optind; i < argc; i++) {
        args[ind] = argv[i];
        ind++;
    }

    args[ind] = NULL;
}


int main(int argc, char* argv[]) {
    int ret, v_flag = 0, inv = 1;
    const char* optstring = "+vt:";
    char * cmd = NULL;

    while ((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 'v': v_flag = 1; break;
            case 't': 
                inv = atoi(optarg); 
                if (inv <= 0) {
                    fprintf(stderr, "An argument for \"-t\" must be a number or argmunt: %d (given value) is not larger than 0!\n", inv); 
                    return 1;       
                }
                break;
            case '?': return 1;
            default: abort();
        }
    }

    if (v_flag == 0 && inv == 1)
        return 0;


    int ind = argc - optind;
    char * args[ind + 1];
    setcmdcom(args, argv, optind, argc);

    long int mean_real = 0, mean_user = 0, mean_sys = 0;
    struct timespec start, stop;
    double real_res_ms = 0.0, real_res_s = 0.0;
    double mean_real_ms = 0.0, mean_real_s = 0.0;

    for (int i = 0; i < inv; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        pid_t pid = fork();

        if (pid == 0) {
            if (v_flag == 0) {
                close(1);
                close(2);
                int h = open("/dev/null", O_WRONLY);
                dup2(h, 1);
                dup2(h, 2);
            }
            int err = execvp(args[0], args); 
            if (err == -1) {
                fprintf(stderr, "Something went wrong with execvp(...) args: <%s>.\n", argv[optind]);
                exit(1);
            }   
        } else {
            int status_location;
            struct rusage _rusage;
            pid_t child_pid = wait3(&status_location, 0, &_rusage);

            clock_gettime(CLOCK_MONOTONIC, &stop);
            long int user_sec = _rusage.ru_utime.tv_sec, user_usec = _rusage.ru_utime.tv_usec;
            long int sys_sec = _rusage.ru_stime.tv_sec, sys_usec = _rusage.ru_stime.tv_usec;

            mean_user += _rusage.ru_utime.tv_usec;
            mean_sys += _rusage.ru_stime.tv_usec;

            real_res_s = (stop.tv_sec - start.tv_sec) + (double) (( stop.tv_nsec - start.tv_nsec ) / (double) BILLION);
            real_res_ms = (stop.tv_sec - start.tv_sec) + (double) ((stop.tv_nsec - start.tv_nsec) / (double) 1000000);


            mean_real_ms += real_res_ms;
            mean_real_s += real_res_s;

            printf("\nExecuting user instructions:   %ld [s] | %.06ld [ms]\n", user_sec, user_usec);
            printf("Executing system instructions: %ld [s] | %.06ld [ms]\n", sys_sec, sys_usec);
            printf("Real time: %.06f [s].\n", real_res_s);
            printf("Real time: %.06f [ms].\n", real_res_ms);
            printf("------------------------------------------------------------\n\n");
        }
    } 

    if (inv > 1) {
        printf("\nAvg. user time:   %.06ld [ms].\n", mean_user / inv);
        printf("Avg. system time: %.06ld [ms].\n", mean_sys / inv);
        printf("Avg. real time: %.06f [s].\n", mean_real_s / (double)inv);
        printf("Avg. real time: %.06f [ms].\n", mean_real_ms / (double)inv);

    }

    return 0;
}