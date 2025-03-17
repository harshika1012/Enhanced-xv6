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


