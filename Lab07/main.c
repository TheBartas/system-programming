#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "thread.h"
#include "timer.h"

void *inf_factorial() {
    start();
    unsigned long long factorial = 1;
    for (int i = 1;; i++) {
        factorial *= i;
    }
    return NULL;
}

void sigint_handler(int sigNo) {
    double ttl = stop();
    time_t _time_t; 
    struct tm *_tm;

    time(&_time_t);
    _tm = localtime(&_time_t);

    printf("\t\t[ %ld ] [ %f (ms)]\n", pthread_self(), ttl);

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    int t_flag = 0, n_flag = 0, i_flag = 0;
    int ret, t, n;
    const char *optstring = "n:t:i";

    while((ret = getopt(argc, argv, optstring)) != -1) {
        switch (ret) {
            case 'n': n = atoi(optarg); n_flag = 1; break;
            case 't': t = atoi(optarg); t_flag = 1; break;
            case 'i': i_flag = 1; break;
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

    int thread_counter = 0;
    thread_t * _thread = thread_alloc(n);

    srand(time(NULL));
    time_t _time_t; 
    struct tm *_tm;

    time(&_time_t);
    _tm = localtime(&_time_t);

    for (int i = 0;i < n;i++) {
        _thread[i].ttl = rand() % t + 1; 
        if(pthread_create(&_thread[i].thread_id, NULL, inf_factorial, NULL) != 0) {
            fprintf(stderr, "Error creating thread! Iter: [%d]", i);
            free(_thread);
            return -1;
        };
        printf("[ %ld ] [ %d (s)]\n", _thread[i].thread_id, _thread[i].ttl);
        thread_counter++;
    }

    thread_sort(_thread, n);

    if (i_flag == 1) thread_show(_thread, n);

    printf("---------------------------------\n\n");

    thread_calc_time(_thread, n);

    if (i_flag == 1) thread_show(_thread, n);

    struct sigaction act;
    act.sa_handler = sigint_handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&(act.sa_mask));
    sigaction(SIGINT, &act, NULL);

    int idx = 0;
    while (thread_counter != 0) {
        thread_counter--;
        int ttl = _thread[idx].ttl;
        sleep(ttl);
        pthread_kill(_thread[idx].thread_id, SIGINT);
        pthread_join(_thread[idx].thread_id, NULL);
        idx++;
    }

    free(_thread);
    return 0;
}