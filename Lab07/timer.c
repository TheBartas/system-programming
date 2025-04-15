#include "timer.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>


static pthread_key_t counter_key;
static pthread_once_t counter_once = PTHREAD_ONCE_INIT;

static void free_memory(void* buffer) {
    free(buffer);
}

static void create_key(void) {
    pthread_key_create(&counter_key, free_memory);
}

void start() {
    pthread_once(&counter_once, create_key);

    struct timespec* start = pthread_getspecific(counter_key);
    if (start == NULL) {
        start = malloc(sizeof(struct timespec));
        if (!start) {
            return;
        }
        pthread_setspecific(counter_key, start);
    }

    clock_gettime(CLOCK_MONOTONIC, start);
}

double stop() {
    struct timespec stop;
    clock_gettime(CLOCK_MONOTONIC, &stop);

    struct timespec* start = pthread_getspecific(counter_key);
    if (start == NULL) return -1;

    double diff = (stop.tv_sec - start->tv_sec) * 1000.0 + (stop.tv_nsec - start->tv_nsec) / 1000000.0;

    return diff;
}
