# Enhanced-xv6

#  System Calls
Add the system call getSysCount and the corresponding user program syscount. getSysCount counts the number of times a specific system call was called by a process and prints it. The system call to count is provided through the syscount program by the bits of an integer mask provided as syscount <mask> command [args] where command [args] is any other valid command in XV6.

The specified command [args] runs till it exits, while the number of times the system call corresponding to the <mask> is called is counted by syscount. For example, to get the number of times the ith system call was called, the mask is specified as 1 << i. You may assume that only 1 bit will be turned on (i.e. the number of times one specific syscall is called will be counted.) The enumeration of syscalls is provided in kernel/syscall.h.

After the command [args] exits, the number of times the chosen syscall was called is printed out, along with its name in the following format: PID <caller pid> called <syscall name> <n> times. 

For example:

syscount 32768 grep hello README.md

PID 6 called open 1 times.


Here 1 << 15 = 32678, which corresponds to the open syscall and the pid of the process is 6.

NOTE: The number of times the corresponding system call is called by the children of the process called with syscount should also be added to the same total. You may assume that we will count the number of times one syscall is counted and that a maximum of 31 system calls will exist at any point.

# Wake me up when my timer ends

In this specification you’ll add a feature to xv6 that periodically alerts a process as it uses CPU time. This might be useful for compute-bound processes that want to limit how much CPU time they chew up, or for processes that want to compute but also want to take some periodic action. More generally, you’ll be implementing a primitive form of user-level interrupt/fault handlers like a SIGCHILD handler.

You should add a new sigalarm(interval, handler) system call. If an application calls sigalarm(n, fn) , then after every n  ”ticks” of CPU time that the program consumes, the kernel will cause application function fn  to be called. When fn  returns, the application will resume where it left off.

Add another system call sigreturn(), to reset the process state to before the handler was called. This system call needs to be made at the end of the handler so the process can resume where it left off.


# Scheduling
The default scheduling policy in xv6 is round-robin-based. In this task, you’ll implement two other scheduling policies and incorporate them in xv6. The kernel should only use one scheduling policy declared at compile time, with a default of round robin in case none are specified.

Modify the makefile to support the SCHEDULER macro to compile the specified scheduling algorithm. Use the flags for compilation:-

    Lottery Based Scheduling: LBS
    Multi Level Feedback Queue: MLFQ
    Your compilation process should look something like this: make clean; make qemu 
    SCHEDULER=MLFQ.

# Lottery scheduling
    Implement a preemptive lottery based scheduling policy that assigns a time slice to the process randomly in proportion to the number of tickets it owns. That is, the probability that the process runs in a given time slice is proportional to the number of tickets owned by it. You may use any method to generate (pseudo)random numbers.

Implement a system call int settickets(int number) , which sets the number of tickets of the calling process. By default, each process should get one ticket; calling this routine makes it such that a process can raise the number of tickets it receives, and thus receive a higher proportion of CPU cycles. For example, a program can do the following to increase its tickets from the default of 1 to 2:

int newTicketNum = settickets(2);
if (newTicketNum == -1) {
	// changing tickets failed
	fprintf(2, "could not change tickets to 2 for process with pid %d\n", getpid());
}

This is the traditional lottery based scheduling policy, however, last time processes protested that coming early or late did not affect their winning chances. This time, if a process is considered the winner of the lottery, it is only if there are no other processes with the same number of tickets but an earlier arrival time.
Example: If there are three processes:
A, arrived at t=0s with 3 tickets,
B, arrived at t=3s with 4 tickets,
C, arrived at t=4s with 3 tickets,
and C is chosen as the winner at t=5s, the result is overturned and handed to A because it arrived earlier but has the same number of tickets.

Note: You’ll need to assign tickets to a process when it is created. Also, you’ll need to make sure a child process starts with the same number of tickets as its parent. Only processes that are in the RUNNABLE state can participate in the lottery. The time slice is 1 tick.
