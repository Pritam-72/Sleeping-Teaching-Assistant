#define _POSIX_C_SOURCE 200809L
#include "shared.h"
#include "ta.h"
#include "student.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

/* Global variables definition */
int waiting_students = 0;
int num_chairs = DEFAULT_CHAIRS;
volatile int running = 1;
int ta_sleeping = 0;

pthread_mutex_t chair_mutex;
sem_t students_sem;
sem_t ta_sem;
sem_t done_sem;

static void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -s <num>    Number of students (default: %d)\n", DEFAULT_STUDENTS);
    printf("  -c <num>    Number of hallway chairs (default: %d)\n", DEFAULT_CHAIRS);
    printf("  -n <num>    Number of help sessions per student (default: %d)\n", DEFAULT_HELPS);
    printf("  -h          Show this help message\n");
}

int main(int argc, char *argv[]) {
    int total_students = DEFAULT_STUDENTS;
    int total_helps = DEFAULT_HELPS;

    int opt;
    while ((opt = getopt(argc, argv, "s:c:n:h")) != -1) {
        switch (opt) {
            case 's':
                total_students = atoi(optarg);
                break;
            case 'c':
                num_chairs = atoi(optarg);
                break;
            case 'n':
                total_helps = atoi(optarg);
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    if (total_students <= 0 || num_chairs <= 0 || total_helps <= 0) {
        fprintf(stderr, "Error: All parameters must be positive integers.\n");
        return 1;
    }

    /* Initialize logger */
    init_logger();

    log_event("=================================================");
    log_event("       THE SLEEPING TEACHING ASSISTANT          ");
    log_event("=================================================");
    log_event("Configuration:");
    log_event("  - Total Students:         %d", total_students);
    log_event("  - Hallway Chairs:         %d", num_chairs);
    log_event("  - Helps Required/Student: %d", total_helps);
    log_event("=================================================");

    /* Initialize synchronization primitives */
    if (pthread_mutex_init(&chair_mutex, NULL) != 0) {
        perror("pthread_mutex_init failed");
        return 1;
    }
    if (sem_init(&students_sem, 0, 0) != 0) {
        perror("sem_init students_sem failed");
        return 1;
    }
    if (sem_init(&ta_sem, 0, 0) != 0) {
        perror("sem_init ta_sem failed");
        return 1;
    }
    if (sem_init(&done_sem, 0, 0) != 0) {
        perror("sem_init done_sem failed");
        return 1;
    }

    /* Allocate memory for student threads and arguments */
    pthread_t ta_tid;
    pthread_t *student_tids = malloc(sizeof(pthread_t) * total_students);
    StudentArgs *student_args = malloc(sizeof(StudentArgs) * total_students);

    if (!student_tids || !student_args) {
        fprintf(stderr, "Memory allocation error\n");
        return 1;
    }

    /* Create TA thread */
    if (pthread_create(&ta_tid, NULL, ta_thread, NULL) != 0) {
        perror("pthread_create ta_thread failed");
        return 1;
    }

    /* Create Student threads */
    unsigned int base_seed = (unsigned int)time(NULL);
    for (int i = 0; i < total_students; i++) {
        student_args[i].id = i + 1;
        student_args[i].helps_left = total_helps;
        student_args[i].total_helps = total_helps;
        student_args[i].seed = base_seed + (i * 997) + 13;

        if (pthread_create(&student_tids[i], NULL, student_thread, &student_args[i]) != 0) {
            perror("pthread_create student_thread failed");
            return 1;
        }
    }

    /* Wait for all students to finish their assignments */
    for (int i = 0; i < total_students; i++) {
        pthread_join(student_tids[i], NULL);
    }

    log_event("=================================================");
    log_event("All students have completed their assignments!");
    log_event("Notifying TA to wrap up office hours...");
    log_event("=================================================");

    /* Gracefully signal TA to shut down */
    pthread_mutex_lock(&chair_mutex);
    running = 0;
    pthread_mutex_unlock(&chair_mutex);

    /* Wake TA if napping */
    sem_post(&students_sem);

    /* Wait for TA thread to terminate */
    pthread_join(ta_tid, NULL);

    /* Clean up resources */
    free(student_tids);
    free(student_args);

    sem_destroy(&students_sem);
    sem_destroy(&ta_sem);
    sem_destroy(&done_sem);
    pthread_mutex_destroy(&chair_mutex);

    log_event("Simulation completed successfully. Clean shutdown.");
    destroy_logger();

    return 0;
}
