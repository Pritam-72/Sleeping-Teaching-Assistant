#define _POSIX_C_SOURCE 200809L
#include "ta.h"
#include "shared.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

void *ta_thread(void *arg) {
    (void)arg;
    unsigned int ta_seed = (unsigned int)time(NULL) ^ 0xABCD1234;

    log_event("[TA] Teaching Assistant arrived at office and opened office hours.");

    while (1) {
        int nap = 0;

        pthread_mutex_lock(&chair_mutex);
        if (waiting_students == 0 && running) {
            ta_sleeping = 1;
            nap = 1;
        }
        pthread_mutex_unlock(&chair_mutex);

        if (nap) {
            log_event("[TA] No students waiting. TA is sleeping (Zzz...).");
        }

        /* Blocks when the hallway is empty. A post that already happened is not lost. */
        semx_wait(&students_sem);

        pthread_mutex_lock(&chair_mutex);
        if (!running && waiting_students == 0) {
            pthread_mutex_unlock(&chair_mutex);
            break;
        }
        if (waiting_students == 0) {
            pthread_mutex_unlock(&chair_mutex);
            continue;
        }

        ta_sleeping = 0;
        int sid = wait_queue[q_head];
        q_head = (q_head + 1) % num_chairs;
        waiting_students--;
        assert(waiting_students >= 0 && waiting_students <= num_chairs);
        assert(sid >= 1);
        int remaining = waiting_students;
        pthread_mutex_unlock(&chair_mutex);

        log_event("[TA] Calling student %d into office (hallway chairs occupied: %d/%d).",
                  sid, remaining, num_chairs);

        semx_post(&admit_sems[sid]);

        int help_ms = help_min;
        if (help_max > help_min) {
            help_ms += (int)(rand_r(&ta_seed) % (unsigned)(help_max - help_min + 1));
        }
        log_event("[TA] Helping student %d with programming assignment (%d ms)...", sid, help_ms);
        random_sleep_ms(help_ms, help_ms, &ta_seed);
        log_event("[TA] Finished helping student %d.", sid);

        /* done_sems[sid] belongs to this student. The next student is not
         * admitted until this one has taken the completion and left. */
        semx_post(&done_sems[sid]);
        semx_wait(&left_sems[sid]);
    }

    log_event("[TA] Office hours ended. TA leaving office. Goodbye!");
    return NULL;
}
