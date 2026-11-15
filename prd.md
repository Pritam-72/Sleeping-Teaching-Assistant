# PRD — The Sleeping Teaching Assistant

## 1. Overview
A concurrent simulation of a university TA helping students during office hours. The TA's office fits one student at a time; 3 hallway chairs hold waiting students. The TA naps when idle. Implemented in C using POSIX threads, mutex locks, and semaphores.

## 2. Problem Statement
Coordinate one TA thread and N student threads so that:
- The TA sleeps when nobody needs help and is woken by an arriving student.
- Students wait in at most 3 chairs; extra students leave and retry later.
- Shared state (number of waiting students) is never corrupted.

## 3. Actors
| Actor | Thread | Behavior |
|-------|--------|----------|
| TA | 1 thread | Loop: sleep until a student is waiting → take a student → help for random time → repeat |
| Student | N threads | Loop: program (random time) → arrive → get help / wait / leave and retry |

## 4. Functional Requirements
- **FR1** TA blocks (sleeps) when no students are waiting.
- **FR2** Student arriving when TA is asleep wakes the TA.
- **FR3** Student arriving when TA is busy and a chair is free sits and waits.
- **FR4** Student arriving when all chairs are taken leaves and returns after a random delay.
- **FR5** TA helps exactly one student at a time for a random duration.
- **FR6** Each student is helped a configurable number of times, then terminates.
- **FR7** Program terminates cleanly after all students are done (TA is signalled to exit).
- **FR8** All events are logged with timestamps and thread IDs.

## 5. Non-Functional Requirements
- Deadlock-free, starvation-aware, data-race-free.
- Builds on Linux/macOS with `gcc -pthread`.
- No busy waiting (use blocking primitives only).
- Parameters via CLI: `./ta -s <students> -c <chairs> -n <helps_per_student>`.

## 6. Synchronization Design

### Shared state
```c
int waiting = 0;                 // students currently in hallway chairs
#define NUM_CHAIRS 3
pthread_mutex_t mutex;           // protects `waiting`
sem_t students_sem;              // counts waiting students (init 0) — wakes TA
sem_t ta_sem;                    // TA "ready" signal (init 0) — admits a student
```

### TA pseudocode
```c
while (running) {
    sem_wait(&students_sem);          // sleep until a student arrives
    pthread_mutex_lock(&mutex);
    waiting--;                        // take a student from chair
    pthread_mutex_unlock(&mutex);
    sem_post(&ta_sem);                // invite student in
    help_student();                   // sleep(random)
}
```

### Student pseudocode
```c
while (helps_left > 0) {
    program();                        // sleep(random)
    pthread_mutex_lock(&mutex);
    if (waiting < NUM_CHAIRS) {
        waiting++;
        pthread_mutex_unlock(&mutex);
        sem_post(&students_sem);      // notify / wake TA
        sem_wait(&ta_sem);            // wait to be called
        get_help();                   // sleep(random)
        helps_left--;
    } else {
        pthread_mutex_unlock(&mutex); // no chair: come back later
    }
}
```

### Why this works
- `students_sem` = number of waiting students; TA blocks at 0 (sleeping).
- `ta_sem` = permission to enter; ensures one student at a time.
- `mutex` guards `waiting` so the chair-count check and update are atomic.

### Shutdown
After all students join, main sets `running = 0` and posts `students_sem`; TA wakes, sees the flag, and exits.

## 7. Architecture / File Structure
```
sleeping-ta/
├── src/
│   ├── main.c        # arg parsing, thread creation, join, cleanup
│   ├── ta.c / ta.h   # TA thread
│   ├── student.c / .h
│   └── util.c / .h   # logging, random sleep
├── Makefile
├── README.md
├── prd.md
└── goal.md
```

## 8. Roadmap

### Phase 0 — Setup (0.5 day)
- Create repo, Makefile (`-Wall -Wextra -pthread -g`), folder structure.
- Review pthread, `sem_init`, `sem_wait/post`, mutex APIs. (Note: macOS lacks unnamed `sem_init`; use `sem_open` or GCD.)

### Phase 1 — Skeleton (0.5 day)
- Parse args, spawn TA + N student threads, join them.
- Add thread-safe logger with timestamps.

### Phase 2 — Core Synchronization (1 day)
- Add mutex, `students_sem`, `ta_sem`.
- Implement TA loop and student arrival logic.
- Verify sleep/wake and one-at-a-time help.

### Phase 3 — Edge Cases (0.5–1 day)
- Full-chairs path with retry delay.
- Per-student help counter and exit condition.
- Graceful TA shutdown; destroy mutex/semaphores.

### Phase 4 — Testing & Validation (1 day)
- Scenarios: 1 student; 3 students; 10 students (overflow); 50 students with short delays (stress).
- Add `assert(waiting >= 0 && waiting <= NUM_CHAIRS)`.
- Run ThreadSanitizer (`-fsanitize=thread`), Helgrind, Valgrind.
- Check for lost wake-ups and deadlock by long runs.

### Phase 5 — Polish & Extensions (optional)
- FIFO fairness with a circular queue + per-student semaphores.
- Multiple TAs.
- Statistics: average wait, turned-away count, TA idle time.
- README with build/run instructions and sample output.

## 9. Test Plan
| Test | Expected |
|------|----------|
| Single student, empty office | TA wakes, helps, sleeps again |
| 4 simultaneous students | 1 helped, 3 seated, 4th leaves and retries |
| N students, all done | Program exits with code 0, no hang |
| TSan/Helgrind | No reported races |
| Valgrind | No leaks |

## 10. Risks & Mitigations
| Risk | Mitigation |
|------|------------|
| Race on `waiting` | Always access under mutex |
| Deadlock (holding mutex while blocking on a semaphore) | Never call `sem_wait` while holding `mutex` |
| Lost wake-up | Use counting semaphore, not a flag |
| TA never exits | Shutdown flag + final `sem_post` |
| Starvation | Optional FIFO queue extension |

## 11. Deliverables
- Source code, Makefile, README
- Sample log output demonstrating all scenarios
- `prd.md`, `goal.md`