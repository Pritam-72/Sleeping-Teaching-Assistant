#define _POSIX_C_SOURCE 200809L
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec start_time;

void init_logger(void) {
    /* log_mutex is already initialized. Re-init would be undefined. */
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
    printf("[%7.3fs] [tid %p] ", elapsed, (void *)(uintptr_t)pthread_self());

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
    if (max_ms < 0) {
        return;
    }
    int range = max_ms - min_ms + 1;
    int duration_ms = min_ms;
    if (range > 1) {
        duration_ms = min_ms + (int)(rand_r(seed) % (unsigned)range);
    }

    struct timespec ts;
    ts.tv_sec = duration_ms / 1000;
    ts.tv_nsec = (long)(duration_ms % 1000) * 1000000L;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

static sem_t *semx_ptr(SemX *s) {
    return s->named ? s->named : &s->unnamed;
}

int semx_init(SemX *s, unsigned int value, const char *tag) {
    memset(s, 0, sizeof *s);

#ifdef __APPLE__
    /* Unnamed sem_init is unimplemented. Name must start with '/' and stay short. */
    int n = snprintf(s->name, sizeof s->name, "/s%x%s", (unsigned)getpid(), tag);
    if (n < 0 || n >= (int)sizeof s->name) {
        errno = ENAMETOOLONG;
        return -1;
    }
    sem_unlink(s->name);
    s->named = sem_open(s->name, O_CREAT | O_EXCL, 0600, value);
    if (s->named == SEM_FAILED) {
        s->named = NULL;
        return -1;
    }
#else
    (void)tag;
    if (sem_init(&s->unnamed, 0, value) != 0) {
        return -1;
    }
#endif
    s->live = 1;
    return 0;
}

void semx_destroy(SemX *s) {
    if (!s || !s->live) {
        return;
    }
#ifdef __APPLE__
    if (s->named && s->named != SEM_FAILED) {
        sem_close(s->named);
        sem_unlink(s->name);
    }
    s->named = NULL;
#else
    sem_destroy(&s->unnamed);
#endif
    s->live = 0;
}

void semx_wait(SemX *s) {
    while (sem_wait(semx_ptr(s)) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            abort();
        }
    }
}

void semx_post(SemX *s) {
    while (sem_post(semx_ptr(s)) == -1) {
        if (errno != EINTR) {
            perror("sem_post");
            abort();
        }
    }
}
