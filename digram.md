# Diagrams — The Sleeping Teaching Assistant

Four Mermaid diagrams: build roadmap, TA flow, student flow, and a runtime sequence.

---

## 1. Project Build Roadmap

```mermaid
flowchart TD
    A([Start]) --> P0["Phase 0: Setup<br/>repo, Makefile, -pthread"]
    P0 --> P1["Phase 1: Skeleton<br/>spawn TA + N students, logger"]
    P1 --> P2["Phase 2: Core Sync<br/>mutex + students_sem + ta_sem"]
    P2 --> C1{"TA sleeps and wakes<br/>correctly?"}
    C1 -- No --> P2
    C1 -- Yes --> P3["Phase 3: Edge Cases<br/>full chairs, retry, shutdown"]
    P3 --> C2{"Exits cleanly<br/>for 1, 3, 10, 50 students?"}
    C2 -- No --> P3
    C2 -- Yes --> P4["Phase 4: Testing<br/>TSan, Helgrind, Valgrind, 100-run loop"]
    P4 --> C3{"Zero warnings<br/>and zero leaks?"}
    C3 -- No --> P2
    C3 -- Yes --> P5["Phase 5: Polish<br/>README, sample output, tag v1.0"]
    P5 --> P6["Phase 6 (optional)<br/>FIFO, multi-TA, stats"]
    P6 --> Z([Done])
    P5 --> Z
```

---

## 2. TA Thread Flow

```mermaid
flowchart TD
    S([TA thread starts]) --> W["sem_wait(students_sem)<br/>TA sleeps here"]
    W --> R{"running == 0 ?"}
    R -- Yes --> X([TA exits])
    R -- No --> L1["lock(mutex)"]
    L1 --> D["waiting--"]
    D --> U1["unlock(mutex)"]
    U1 --> P["sem_post(ta_sem)<br/>call student in"]
    P --> H["help_student()<br/>random sleep"]
    H --> W
```

---

## 3. Student Thread Flow

```mermaid
flowchart TD
    S([Student starts]) --> Q{"helps_left > 0 ?"}
    Q -- No --> E([Student exits])
    Q -- Yes --> PR["program()<br/>random sleep"]
    PR --> L["lock(mutex)"]
    L --> CH{"waiting < NUM_CHAIRS ?"}
    CH -- No --> UL1["unlock(mutex)"]
    UL1 --> LOG["log: no chair free<br/>come back later"]
    LOG --> Q
    CH -- Yes --> INC["waiting++"]
    INC --> UL2["unlock(mutex)"]
    UL2 --> SP["sem_post(students_sem)<br/>wake TA if asleep"]
    SP --> SW["sem_wait(ta_sem)<br/>sit in hallway until called"]
    SW --> GH["get_help()<br/>random sleep"]
    GH --> DEC["helps_left--"]
    DEC --> Q
```

---

## 4. Runtime Sequence (TA asleep, then busy)

```mermaid
sequenceDiagram
    participant S1 as Student 1
    participant S2 as Student 2
    participant M as mutex / waiting
    participant SS as students_sem
    participant TS as ta_sem
    participant TA as TA

    Note over TA,SS: TA sleeping on sem_wait(students_sem)
    S1->>M: lock, waiting < 3, waiting++, unlock
    S1->>SS: sem_post (wakes TA)
    S1->>TS: sem_wait (blocks)
    SS-->>TA: TA wakes
    TA->>M: lock, waiting--, unlock
    TA->>TS: sem_post (admits Student 1)
    TS-->>S1: Student 1 enters
    Note over TA,S1: TA helping Student 1
    S2->>M: lock, waiting < 3, waiting++, unlock
    S2->>SS: sem_post
    S2->>TS: sem_wait (sits in hallway)
    TA-->>S1: help finished
    TA->>SS: sem_wait (returns immediately, count is 1)
    TA->>M: lock, waiting--, unlock
    TA->>TS: sem_post (admits Student 2)
    TS-->>S2: Student 2 enters
```

---

## 5. Synchronization Primitives Map

```mermaid
flowchart LR
    subgraph Shared State
        W["waiting (int)"]
        M["mutex"]
        R["running (flag)"]
    end
    subgraph Semaphores
        SS["students_sem = 0<br/>counts waiting students"]
        TS["ta_sem = 0<br/>admits one student"]
    end
    ST["Student threads x N"] -- "lock/unlock" --> M
    TA["TA thread"] -- "lock/unlock" --> M
    M -. protects .-> W
    ST -- "sem_post" --> SS
    TA -- "sem_wait" --> SS
    TA -- "sem_post" --> TS
    ST -- "sem_wait" --> TS
    MAIN["main()"] -- "sets 0, then sem_post for shutdown" --> R
```