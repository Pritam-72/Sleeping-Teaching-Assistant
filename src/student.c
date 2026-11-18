#define _POSIX_C_SOURCE 200809L
#include "student.h"
#include "shared.h"
#include "util.h"

#include <assert.h>

void *student_thread(void *arg) {
    StudentArgs *s = (StudentArgs *)arg;

    log_event("[Student %d] Started programming assignments (needs help %d times).",
              s->id, s->helps_left);

    while (s->helps_left > 0) {
        log_event("[Student %d] Programming assignment in progress...", s->id);
        random_sleep_ms(prog_min, prog_max, &s->seed);

        log_event("[Student %d] Encountered a bug! Heading to TA's office for help.", s->id);

        pthread_mutex_lock(&chair_mutex);
        if (waiting_students < num_chairs) {
            wait_queue[q_tail] = s->id;
            q_tail = (q_tail + 1) % num_chairs;
            waiting_students++;
            assert(waiting_students >= 0 && waiting_students <= num_chairs);
            int occupied = waiting_students;
            int waking = ta_sleeping;
            if (waking) {
                ta_sleeping = 0;
            }
            pthread_mutex_unlock(&chair_mutex);

            if (waking) {
                log_event("[Student %d] Found TA sleeping! Student awakens TA (hallway chairs occupied: %d/%d).",
                          s->id, occupied, num_chairs);
            } else {
                log_event("[Student %d] TA is not free. Student sits on a hallway chair (chairs occupied: %d/%d).",
                          s->id, occupied, num_chairs);
            }

            semx_post(&students_sem);
            semx_wait(&admit_sems[s->id]);

            log_event("[Student %d] Entering office and getting help from TA.", s->id);
            semx_wait(&done_sems[s->id]);
            s->helps_left--;
            log_event("[Student %d] Finished receiving help from TA (%d/%d helps remaining). Returning to lab.",
                      s->id, s->helps_left, s->total_helps);
            semx_post(&left_sems[s->id]);
        } else {
            pthread_mutex_unlock(&chair_mutex);
            log_event("[Student %d] All %d hallway chairs are full! Student leaves and will come back later.",
                      s->id, num_chairs);
            random_sleep_ms(retry_min, retry_max, &s->seed);
        }
    }

    log_event("[Student %d] Finished all programming assignments! Exiting.", s->id);
    return NULL;
}
