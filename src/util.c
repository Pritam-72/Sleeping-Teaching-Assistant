#define _POSIX_C_SOURCE 200809L
#include "util.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec start_time;

void init_logger(void) {
    pthread_mutex_init(&log_mutex, NULL);
    clock_gettime(CLOCK_MONOTONIC, &start_time);
}

void destroy_logger(void) {
    pthread_mutex_destroy(&log_mutex);
}

void log_event(const char *fmt, ...) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double elapsed = (now.tv_sec - start_time.tv_sec) + 
                     (now.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    pthread_mutex_lock(&log_mutex);
    printf("[%7.3fs] ", elapsed);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

void random_sleep_ms(int min_ms, int max_ms, unsigned int *seed) {
    if (min_ms >= max_ms) {
        min_ms = max_ms;
    }
    int range = max_ms - min_ms + 1;
    int duration_ms = min_ms + (rand_r(seed) % range);

    struct timespec ts;
    ts.tv_sec = duration_ms / 1000;
    ts.tv_nsec = (long)(duration_ms % 1000) * 1000000L;

    nanosleep(&ts, NULL);
}
