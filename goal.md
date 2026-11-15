# GOAL.md — The Sleeping Teaching Assistant

## Primary Goal
Build a correct, deadlock-free, race-free C program that simulates a TA and multiple students using **POSIX threads (pthreads)**, **mutex locks**, and **semaphores**.

## Success Criteria (Definition of Done)
| # | Criterion | How to verify |
|---|-----------|---------------|
| 1 | TA sleeps when no students are present | Log shows "TA is sleeping" when queue empty |
| 2 | A student arriving at a sleeping TA wakes the TA | Log shows "Student X wakes the TA" |
| 3 | Students wait on hallway chairs when TA is busy | Log shows "Student X waits (chairs occupied: n/3)" |
| 4 | Max 3 students wait at once | Counter never exceeds 3 (assert in code) |
| 5 | Student leaves and returns later when all 3 chairs are full | Log shows "no chair free, will come back later" |
| 6 | Only one student is helped at a time | No two "being helped" lines overlap |
| 7 | Waiting students are served in FIFO order (stretch) | Log order matches arrival order |
| 8 | No data races | Clean run under `-fsanitize=thread` / Helgrind |
| 9 | No deadlock / starvation | Program runs 10+ minutes or N helps and exits cleanly |
| 10 | Clean shutdown | All threads joined, semaphores/mutex destroyed, no leaks (Valgrind) |

## Learning Goals
- Understand producer-consumer / bounded-buffer synchronization.
- Use semaphores for signaling and mutexes for protecting shared state.
- Reason about race conditions, deadlock and lost wake-ups.

## Technical Goals
- Language: **C (C11)**, compiled with `gcc -pthread -Wall -Wextra`.
- Configurable: number of students, number of chairs (default 3), helping time, arrival time.
- Readable timestamped logging.
- Makefile + short README.

## Milestones
1. **M1 – Skeleton:** threads created and joined, no sync.
2. **M2 – Core sync:** semaphores + mutex, TA sleeps/wakes, 3 chairs.
3. **M3 – Edge cases:** full-chairs path, student return loop, graceful exit.
4. **M4 – Validation:** stress test, TSan/Helgrind, Valgrind.
5. **M5 – Polish:** CLI args, README, demo output.

## Non-Goals
- GUI, networking, persistence.
- Multiple TAs (can be a stretch extension).