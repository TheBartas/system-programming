// PS1 IS1 320 LAB07
// Bartłomiej Szewczyk
// sb53955@zut.edu.pl
#ifndef _LAB07
#define _LAB07

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#define ERROR_MALLOC 10

typedef struct {
    pthread_t thread_id;
    int ttl;
} thread_t;

void *thread_alloc(int);
void thread_show(thread_t *, int);
void thread_sort(thread_t *, int);
void thread_calc_time(thread_t *, int);
void start();
double stop();

#endif