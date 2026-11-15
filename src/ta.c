#define _POSIX_C_SOURCE 200809L
#include "ta.h"
#include "shared.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

void *ta_thread(void *arg) {
    (void)arg;
    unsigned int ta_seed = (unsigned int)time(NULL) ^ 0xABCD1234;

    log_event("[TA] Teaching Assistant arrived at office and opened office hours.");

    while (1) {
        /* Check if any students are waiting; if none, announce sleeping */
        pthread_mutex_lock(&chair_mutex);
        if (waiting_students == 0 && running) {
            ta_sleeping = 1;
            log_event("[TA] No students waiting. TA sits at desk and takes a nap (Zzz...).");
        }
        pthread_mutex_unlock(&chair_mutex);

        /* Wait to be awakened by an arriving student or shutdown signal */
        sem_wait(&students_sem);

        /* Check for shutdown condition */
        pthread_mutex_lock(&chair_mutex);
        if (!running && waiting_students == 0) {
            pthread_mutex_unlock(&chair_mutex);
            break;
        }

        /* TA is now awake and helping */
        ta_sleeping = 0;

        /* Decrement waiting students on hallway chairs */
        waiting_students--;
        assert(waiting_students >= 0 && waiting_students <= num_chairs);
        int remaining = waiting_students;
        pthread_mutex_unlock(&chair_mutex);

        log_event("[TA] Calling next student into office (hallway chairs occupied: %d/%d).",
                  remaining, num_chairs);

        /* Admit waiting student into office */
        sem_post(&ta_sem);

        /* Simulate helping the student */
        int help_ms = 150 + (rand_r(&ta_seed) % 250);
        log_event("[TA] Helping student with programming assignment (%d ms)...", help_ms);
        random_sleep_ms(help_ms, help_ms, &ta_seed);
        log_event("[TA] Finished helping student.");

        /* Signal student that help session is completed */
        sem_post(&done_sem);
    }

    log_event("[TA] Office hours ended. TA leaving office. Goodbye!");
    return NULL;
}

/* Admission signal */
