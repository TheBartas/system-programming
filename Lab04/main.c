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
#define MICROSECOND 1000000L


int getcmdlen(int argc, char *argv[], int optind) {
    int command_length = 2;
    for (int i = optind; i < argc; i++) {
        int len = strlen(argv[i]);
        command_length += len + 1;
        if (len == 1) 
            command_length++;
    }
    return command_length;
}

void concat_cmd(char *cmd, char *argv[], int optind) {
    cmd[0] = '\0';
    int i = optind;

    while (argv[i] != NULL) {
        if (strlen(argv[i]) == 1) {
            strcat(cmd, " -");
            strcat(cmd, argv[i]);
        } else {
            strcat(cmd, " ");
            strcat(cmd, argv[i]);
        }
        i++;
    }
}

void create_args(char * args[], char * cmd) {
    char * token = strtok(cmd, " ");
    int i = 0;
    while (token != NULL) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;
}


int main(int argc, char* argv[]) {
    int ret, v_flag = 0, inv = 1;
    const char* optstring = "vt:";
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

    if (v_flag == 1) {
        close(1);
        close(2);
        int h = open("/dev/null", O_WRONLY);
        dup2(h, 1);
        dup2(h, 2);
    }

    int len = getcmdlen(argc, argv, optind);
    cmd = malloc(sizeof * cmd * len);
    concat_cmd(cmd, argv, optind);

    int ind = argc - optind;

    char * args[ind + 1];
    create_args(args, cmd);

    long int mean_real = 0, mean_user = 0, mean_sys = 0;
    struct timespec start, stop;
    double real_res = 0;

    for (int i = 0; i < inv; i++) {
        clock_gettime(CLOCK_REALTIME, &start);

        pid_t pid = fork();

        if (pid == 0) {
            int err = execvp(args[0], args); 
            if (err == -1) {
                fprintf(stderr, "Something went wrong with execvp(...) args: <%s>.", argv[optind]);
                exit(1);
            }   
        } else {
            int status_location;
            struct rusage _rusage;
            pid_t child_pid = wait3(&status_location, 0, &_rusage);

            clock_gettime(CLOCK_REALTIME, &stop);
            long int user_sec = _rusage.ru_utime.tv_sec, user_usec = _rusage.ru_utime.tv_usec;
            long int sys_sec = _rusage.ru_stime.tv_sec, sys_usec = _rusage.ru_stime.tv_usec;

            mean_user += _rusage.ru_utime.tv_usec;
            mean_sys += _rusage.ru_stime.tv_usec;

            real_res = (stop.tv_sec - start.tv_sec) + ( stop.tv_nsec - start.tv_nsec ) / BILLION;
            real_res = (stop.tv_sec - start.tv_sec) * MICROSECOND + (stop.tv_nsec - start.tv_nsec) / 1000;

            printf("Executing user instructions:   %ld [s] | %.06ld [ms]\n", user_sec, user_usec);
            printf("Executing system instructions: %ld [s] | %.06ld [ms]\n", sys_sec, sys_usec);
            printf("Real time: %.06f [ms].\n", real_res);
        }
    } 

    if (inv > 1) {
        printf("Avg. user time:   %.06ld [ms].\n", mean_user / inv);
        printf("Avg. system time: %.06ld [ms].\n", mean_sys / inv);
    }

    
    free(cmd);

    return 0;
}