#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <semaphore.h>

#define DEFAULT_STUDENTS 5
#define DEFAULT_CHAIRS 3
#define DEFAULT_HELPS 3
#define MAX_STUDENTS 512

/* Counting or binary semaphore.
 * Linux: unnamed sem_init. macOS: sem_init returns ENOSYS, so sem_open. */
typedef struct {
    sem_t *named;
    sem_t unnamed;
    char name[32];
    int live;
} SemX;

/* Shared state. Chair count, queue, and ta_sleeping are guarded by chair_mutex. */
extern int waiting_students;
extern int num_chairs;
extern volatile int running;
extern int ta_sleeping;

extern int prog_min, prog_max;
extern int help_min, help_max;
extern int retry_min, retry_max;

extern int *wait_queue;
extern int q_head, q_tail;

extern pthread_mutex_t chair_mutex;

/* Posted once per student who takes a chair. The TA sleeps on this. */
extern SemX students_sem;

/* Per student id (index 1..N). Private, so a completion cannot be stolen
 * and the hallway is served in arrival order. */
extern SemX *admit_sems;
extern SemX *done_sems;
extern SemX *left_sems;

typedef struct {
    int id;
    int helps_left;
    int total_helps;
    unsigned int seed;
} StudentArgs;

#endif /* SHARED_H */
