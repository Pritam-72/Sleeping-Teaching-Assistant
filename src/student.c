#define _POSIX_C_SOURCE 200809L
#include "student.h"
#include "shared.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void *student_thread(void *arg) {
    StudentArgs *s = (StudentArgs *)arg;

    log_event("[Student %d] Started programming assignments (needs help %d times).",
              s->id, s->helps_left);

    while (s->helps_left > 0) {
        /* 1. Simulate working / programming */
        log_event("[Student %d] Programming assignment in progress...", s->id);
        random_sleep_ms(200, 600, &s->seed);

        /* 2. Arrive at TA office for help */
        log_event("[Student %d] Encountered a bug! Heading to TA's office for help.", s->id);

        pthread_mutex_lock(&chair_mutex);
        if (waiting_students < num_chairs) {
            /* Take a seat in the hallway */
            waiting_students++;
            assert(waiting_students >= 0 && waiting_students <= num_chairs);
            int occupied = waiting_students;
            int was_sleeping = ta_sleeping;

            if (was_sleeping) {
                /* Arriving student finds TA sleeping and will awaken TA */
                ta_sleeping = 0;
            }

            pthread_mutex_unlock(&chair_mutex);

            if (was_sleeping) {
                log_event("[Student %d] Found TA sleeping! Student awakens TA (hallway chairs occupied: %d/%d).",
                          s->id, occupied, num_chairs);
            } else {
                log_event("[Student %d] TA is currently helping another student. Student sits on a chair in the hallway (chairs occupied: %d/%d).",
                          s->id, occupied, num_chairs);
            }

            /* Signal TA that a student is waiting (or wake TA) */
            sem_post(&students_sem);

            /* Wait in chair until TA calls this student */
            sem_wait(&ta_sem);

            log_event("[Student %d] Entering office and getting help from TA.", s->id);

            /* Wait until TA finishes the help session */
            sem_wait(&done_sem);

            s->helps_left--;
            log_event("[Student %d] Finished receiving help from TA (%d/%d helps remaining). Returning to lab.",
                      s->id, s->helps_left, s->total_helps);

        } else {
            /* No hallway chairs available */
            pthread_mutex_unlock(&chair_mutex);
            log_event("[Student %d] All %d hallway chairs are full! Student leaves and will come back later.",
                      s->id, num_chairs);

            /* Come back after a delay */
            random_sleep_ms(300, 700, &s->seed);
        }
    }

    log_event("[Student %d] Finished all programming assignments! Exiting.", s->id);
    return NULL;
}
