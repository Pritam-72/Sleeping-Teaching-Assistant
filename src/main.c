#define _POSIX_C_SOURCE 200809L
#include "shared.h"
#include "ta.h"
#include "student.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int waiting_students = 0;
int num_chairs = DEFAULT_CHAIRS;
volatile int running = 1;
int ta_sleeping = 1;
int prog_min = 200, prog_max = 600;
int help_min = 150, help_max = 400;
int retry_min = 300, retry_max = 700;
int *wait_queue = NULL;
int q_head = 0, q_tail = 0;
pthread_mutex_t chair_mutex;
SemX students_sem;
SemX *admit_sems = NULL;
SemX *done_sems = NULL;
SemX *left_sems = NULL;

static void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -s <num>         Number of students (default: %d)\n", DEFAULT_STUDENTS);
    printf("  -c <num>         Number of hallway chairs (default: %d)\n", DEFAULT_CHAIRS);
    printf("  -n <num>         Help sessions per student (default: %d)\n", DEFAULT_HELPS);
    printf("  -p <ms|min:max>  Programming delay before each visit (default: %d:%d)\n", 200, 600);
    printf("  -t <ms|min:max>  How long the TA helps one student (default: %d:%d)\n", 150, 400);
    printf("  -r <ms|min:max>  Delay before retrying a full hallway (default: %d:%d)\n", 300, 700);
    printf("  -h               Show this help message\n");
}

static int parse_positive(const char *s, int *out, int max_ok) {
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (s[0] == '\0' || end == s || *end != '\0' || errno != 0 || v <= 0 || v > max_ok) {
        return -1;
    }
    *out = (int)v;
    return 0;
}

static int parse_range(const char *s, int *lo, int *hi) {
    int a = 0;
    int b = 0;
    char extra = 0;
    if (sscanf(s, "%d:%d%c", &a, &b, &extra) == 2) {
        if (a < 0 || b < a || b > 600000) {
            return -1;
        }
        *lo = a;
        *hi = b;
        return 0;
    }
    if (sscanf(s, "%d%c", &a, &extra) == 1 && a >= 0 && a <= 600000) {
        *lo = a;
        *hi = a;
        return 0;
    }
    return -1;
}

static void destroy_student_sems(int nstudents) {
    if (!admit_sems || !done_sems || !left_sems) {
        return;
    }
    for (int i = 1; i <= nstudents; i++) {
        semx_destroy(&admit_sems[i]);
        semx_destroy(&done_sems[i]);
        semx_destroy(&left_sems[i]);
    }
}

int main(int argc, char *argv[]) {
    int total_students = DEFAULT_STUDENTS;
    int total_helps = DEFAULT_HELPS;
    int mutex_ready = 0;
    int queue_sem_ready = 0;
    int ta_started = 0;
    int started = 0;
    int rc = 1;
    pthread_t ta_tid = 0;
    pthread_t *student_tids = NULL;
    StudentArgs *student_args = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "s:c:n:p:t:r:h")) != -1) {
        switch (opt) {
            case 's':
                if (parse_positive(optarg, &total_students, MAX_STUDENTS) != 0) {
                    fprintf(stderr, "Error: -s must be an integer from 1 to %d.\n", MAX_STUDENTS);
                    return 1;
                }
                break;
            case 'c':
                if (parse_positive(optarg, &num_chairs, 100000) != 0) {
                    fprintf(stderr, "Error: -c must be a positive integer.\n");
                    return 1;
                }
                break;
            case 'n':
                if (parse_positive(optarg, &total_helps, 100000) != 0) {
                    fprintf(stderr, "Error: -n must be a positive integer.\n");
                    return 1;
                }
                break;
            case 'p':
                if (parse_range(optarg, &prog_min, &prog_max) != 0) {
                    fprintf(stderr, "Error: -p expects <ms> or <min:max>.\n");
                    return 1;
                }
                break;
            case 't':
                if (parse_range(optarg, &help_min, &help_max) != 0) {
                    fprintf(stderr, "Error: -t expects <ms> or <min:max>.\n");
                    return 1;
                }
                break;
            case 'r':
                if (parse_range(optarg, &retry_min, &retry_max) != 0) {
                    fprintf(stderr, "Error: -r expects <ms> or <min:max>.\n");
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    init_logger();

    log_event("=================================================");
    log_event("       THE SLEEPING TEACHING ASSISTANT          ");
    log_event("=================================================");
    log_event("Configuration:");
    log_event("  - Total Students:         %d", total_students);
    log_event("  - Hallway Chairs:         %d", num_chairs);
    log_event("  - Helps Required/Student: %d", total_helps);
    log_event("  - Programming delay:      %d-%d ms", prog_min, prog_max);
    log_event("  - Help duration:          %d-%d ms", help_min, help_max);
    log_event("=================================================");

    if (pthread_mutex_init(&chair_mutex, NULL) != 0) {
        perror("pthread_mutex_init failed");
        goto out;
    }
    mutex_ready = 1;

    if (semx_init(&students_sem, 0, "q") != 0) {
        perror("semaphore init failed");
        goto out;
    }
    queue_sem_ready = 1;

    wait_queue = calloc((size_t)num_chairs, sizeof *wait_queue);
    admit_sems = calloc((size_t)total_students + 1, sizeof *admit_sems);
    done_sems = calloc((size_t)total_students + 1, sizeof *done_sems);
    left_sems = calloc((size_t)total_students + 1, sizeof *left_sems);
    student_tids = calloc((size_t)total_students, sizeof *student_tids);
    student_args = calloc((size_t)total_students, sizeof *student_args);
    if (!wait_queue || !admit_sems || !done_sems || !left_sems || !student_tids || !student_args) {
        fprintf(stderr, "Memory allocation error\n");
        goto out;
    }

    for (int i = 1; i <= total_students; i++) {
        char tag[16];
        snprintf(tag, sizeof tag, "a%d", i);
        if (semx_init(&admit_sems[i], 0, tag) != 0) {
            perror("admit semaphore init failed");
            goto out;
        }
        snprintf(tag, sizeof tag, "d%d", i);
        if (semx_init(&done_sems[i], 0, tag) != 0) {
            perror("done semaphore init failed");
            goto out;
        }
        snprintf(tag, sizeof tag, "l%d", i);
        if (semx_init(&left_sems[i], 0, tag) != 0) {
            perror("left semaphore init failed");
            goto out;
        }
    }

    if (pthread_create(&ta_tid, NULL, ta_thread, NULL) != 0) {
        perror("pthread_create ta_thread failed");
        goto out;
    }
    ta_started = 1;

    unsigned int base_seed = (unsigned int)time(NULL);
    for (int i = 0; i < total_students; i++) {
        student_args[i].id = i + 1;
        student_args[i].helps_left = total_helps;
        student_args[i].total_helps = total_helps;
        student_args[i].seed = base_seed + (unsigned)(i * 997) + 13;
        if (pthread_create(&student_tids[i], NULL, student_thread, &student_args[i]) != 0) {
            perror("pthread_create student_thread failed");
            goto out;
        }
        started++;
    }

    rc = 0;

out:
    if (student_tids) {
        for (int i = 0; i < started; i++) {
            pthread_join(student_tids[i], NULL);
        }
    }
    if (rc == 0) {
        log_event("=================================================");
        log_event("All students have completed their assignments!");
        log_event("Notifying TA to wrap up office hours...");
        log_event("=================================================");
    }
    if (ta_started) {
        pthread_mutex_lock(&chair_mutex);
        running = 0;
        pthread_mutex_unlock(&chair_mutex);
        semx_post(&students_sem);
        pthread_join(ta_tid, NULL);
    }
    if (rc == 0) {
        log_event("Simulation completed successfully. Clean shutdown.");
    }

    free(student_tids);
    free(student_args);
    free(wait_queue);
    wait_queue = NULL;
    destroy_student_sems(total_students);
    free(admit_sems);
    free(done_sems);
    free(left_sems);
    admit_sems = NULL;
    done_sems = NULL;
    left_sems = NULL;
    if (queue_sem_ready) {
        semx_destroy(&students_sem);
    }
    if (mutex_ready) {
        pthread_mutex_destroy(&chair_mutex);
    }
    destroy_logger();
    return rc;
}
