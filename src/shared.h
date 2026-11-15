#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <semaphore.h>

#define DEFAULT_STUDENTS 5
#define DEFAULT_CHAIRS 3
#define DEFAULT_HELPS 3

/* Shared state variables */
extern int waiting_students;
extern int num_chairs;
extern volatile int running;
extern int ta_sleeping;

/* Synchronization primitives */
extern pthread_mutex_t chair_mutex;
extern sem_t students_sem; // Student signals TA that someone is waiting
extern sem_t ta_sem;       // TA signals waiting student to enter the office
extern sem_t done_sem;     // TA signals student that help session is completed

/* Structure passed to each student thread */
typedef struct {
    int id;
    int helps_left;
    int total_helps;
    unsigned int seed;
} StudentArgs;

#endif /* SHARED_H */
