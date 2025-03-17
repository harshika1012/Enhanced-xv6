# Enhanced-xv6

## System Calls
I have extended xv6 by adding a new system call, `getSysCount`, along with a user program, `syscount`. This feature allows tracking the number of times a specific system call is invoked by a process and its children.

### Implementation Details:
- The `syscount` program takes a bitmask `<mask>` and a command `[args]` as input: `syscount <mask> command [args]`.
- The bitmask specifies which system call to count using `1 << i`, where `i` is the syscall index in `kernel/syscall.h`.
- After execution, the program prints the number of times the syscall was invoked by the process and its children in the format:
  
  ```
  PID <caller pid> called <syscall name> <n> times.
  ```
  
- Example usage:
  ```
  syscount 32768 grep hello README.md
  ```
  Output:
  ```
  PID 6 called open 1 times.
  ```
- The implementation ensures that child processes' syscall counts are included.

---

## Periodic Process Alerts
I introduced `sigalarm(interval, handler)`, which enables a process to be notified at periodic CPU time intervals.

### Implementation Details:
- `sigalarm(n, fn)`: Calls `fn` after every `n` ticks of CPU time.
- `sigreturn()`: Resets the process state to before the handler was called, ensuring execution resumes normally.
- This feature allows processes to manage CPU usage efficiently and implement periodic actions like user-level interrupt handling.

---

## Scheduling Enhancements
I have implemented two alternative scheduling policies alongside the default round-robin scheduler: **Lottery-Based Scheduling (LBS)** and **Multi-Level Feedback Queue (MLFQ)**.

### Compilation:
The scheduler can be chosen at compile time using:
```sh
make clean; make qemu SCHEDULER=MLFQ
```
Supported policies:
- `SCHEDULER=LBS` (Lottery Scheduling)
- `SCHEDULER=MLFQ` (Multi-Level Feedback Queue)
- Default: Round Robin

### **Lottery-Based Scheduling (LBS)**
- Assigns CPU time randomly in proportion to the number of tickets a process owns.
- Implemented the `settickets(int number)` system call to set a process's ticket count.
- Child processes inherit the parent's ticket count.
- Implemented a **fair tie-breaking** mechanism:
  - If two processes have the same ticket count, priority is given to the process that arrived earlier.
  - Ensures fairness while maintaining the essence of lottery scheduling.
- Only **RUNNABLE** processes participate in the lottery.
- Time slice = **1 tick**.

### **Multi-Level Feedback Queue (MLFQ)**
A simplified preemptive MLFQ scheduler was implemented with the following characteristics:

#### Queue Priorities and Time Slices:
- **Queue 0** (Highest): 1 tick
- **Queue 1**: 4 ticks
- **Queue 2**: 8 ticks
- **Queue 3** (Lowest): 16 ticks

#### Scheduling Rules:
1. A newly created process starts in **Queue 0**.
2. The CPU always prioritizes the highest non-empty queue.
3. If a new high-priority process arrives, it **preempts** a lower-priority running process.
4. A process that uses its full time slice is **demoted** to a lower queue.
5. If a process performs I/O, it re-enters its original queue at the tail.
6. Round-robin is used in **Queue 3** to prevent starvation.
7. **Priority Boosting**: Every **48 ticks**, all processes are moved back to **Queue 0** to prevent starvation.

---

## Conclusion
Through these enhancements, xv6 now supports syscall monitoring, user-level alarms, and improved scheduling policies that provide fairness and efficiency. These features make xv6 more flexible and capable of handling various process scheduling and monitoring scenarios effectively.

