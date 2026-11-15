# PLAN.md — The Sleeping Teaching Assistant

A step-by-step execution plan. Check off tasks as you complete them. Estimated total: **4–5 days** part-time.

---

## Phase 0 — Setup (0.5 day)

- [x] Create folder `src/`, `Makefile`, `README.md`
- [x] Write Makefile with flags: `-Wall -Wextra -std=c11 -pthread -g`
- [x] Add Makefile target `tsan` using `-fsanitize=thread`
- [x] Add Makefile target `asan` using `-fsanitize=address,undefined`
- [x] Review APIs: `pthread_create/join`, `pthread_mutex_*`, `sem_init/wait/post/destroy`

**Exit check:** `make` builds without warnings.

---

## Phase 1 — Skeleton (0.5 day)

- [x] `util.c`: thread-safe `log_event(fmt, ...)` with timestamp (guarded by its own mutex)
- [x] `util.c`: `random_sleep_ms(min_ms, max_ms)` using `rand_r` (thread-safe, per-thread seed)
- [x] `main.c`: parse CLI args (`-s students`, `-c chairs`, `-n helps_per_student`)
- [x] Create 1 TA thread and N student threads, then join all
- [x] Structured `ta_thread` and `student_thread` functions with clear logging

**Exit check:** N students and the TA print start/finish lines; program exits.

---

## Phase 2 — Core Synchronization (1 day)

- [x] Declare shared state: `waiting_students`, `running`, `chair_mutex`, `students_sem` (0), `ta_sem` (0), `done_sem` (0)
- [x] Initialize in `main`, destroy at the end
- [x] **TA loop:** `sem_wait(students_sem)` → lock, `waiting--`, unlock → `sem_post(ta_sem)` → help (random sleep) → `sem_post(done_sem)`
- [x] **Student loop:** random programming sleep → lock → if `waiting < chairs` then `waiting++`, unlock, `sem_post(students_sem)`, `sem_wait(ta_sem)`, get help, `sem_wait(done_sem)`
- [x] Log every transition: sleeping, woken, seated, called in, being helped, done
- [x] Rule check: no `sem_wait` is ever called while holding `chair_mutex`

**Exit check:** with 1 student, the log shows TA sleeps → wakes → helps → sleeps again.

---

## Phase 3 — Edge Cases (0.5–1 day)

- [x] Full chairs: unlock, log "no chair free", sleep a random time, retry
- [x] Per-student `helps_left` counter; thread exits when it reaches 0
- [x] Graceful shutdown: after all students are joined, set `running = 0`, `sem_post(students_sem)`, join the TA
- [x] TA checks `running` after waking and exits if no student is waiting
- [x] Destroy mutex and semaphores cleanly

**Exit check:** program exits with code 0 for 1, 3, 10 and 50 students; never hangs.

---

## Phase 4 — Testing and Validation (1 day)

- [x] Add `assert(waiting_students >= 0 && waiting_students <= num_chairs)` after every modification
- [x] Scenario tests:
  - [x] 1 student → sleep/wake cycle correct
  - [x] 4 simultaneous students → 1 helped, 3 seated, 1 turned away
  - [x] 10 students, long help time → turn-aways and retries
  - [x] Stress tests with multiple student loads
- [x] Verify "one student at a time" (no overlapping "being helped" lines)
- [x] Run ThreadSanitizer: `make tsan && ./ta_tsan` (0 warnings, 0 races)
- [x] Run AddressSanitizer & LeakSanitizer: `make asan && ./ta_asan` (0 leaks)
- [x] Loop test: 20 back-to-back automated stress runs passed without deadlock

**Exit check:** zero sanitizer warnings, zero leaks, clean runs.

---

## Phase 5 — Polish (0.5 day)

- [x] Write README: build, run, options, sample output, design explanation
- [x] Clean up code comments and naming
- [x] Provide complete Makefile targets (`all`, `tsan`, `asan`, `clean`, `test`)

---

## Phase 6 — Optional Extensions

- [ ] **FIFO fairness:** circular queue of waiting students with a per-student semaphore
- [ ] **Multiple TAs:** N TA threads sharing the same semaphores
- [ ] **Statistics:** average wait time, turn-away count, TA idle percentage
- [ ] **Office-hours timer:** TA exits when the session ends

---

## Timeline Summary

| Day | Focus |
|-----|-------|
| 1 | Phase 0 + Phase 1 |
| 2 | Phase 2 |
| 3 | Phase 3 |
| 4 | Phase 4 |
| 5 | Phase 5 (+ extensions if time allows) |

---

## Common Pitfalls Checklist

- [x] Holding the mutex while blocking on a semaphore (deadlock) — avoided by strict order
- [x] Reading `waiting` outside the mutex (race) — always accessed under `chair_mutex`
- [x] Using `rand()` across threads (not thread-safe, use `rand_r`) — used `rand_r` with per-thread seeds
- [x] Forgetting the final `sem_post` so the TA never exits — handled in `main.c` shutdown protocol
- [x] Printing from multiple threads without a logging lock (garbled output) — guarded by `log_mutex`
- [x] Not joining threads before destroying sync primitives — all joins happen before destroy