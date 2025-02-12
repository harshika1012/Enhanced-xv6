// // // // #include "kernel/types.h"
// // // // #include "kernel/stat.h"
// // // // #include "user/user.h"
// // // // #include "kernel/fcntl.h"

// // // // char *syscall_names[] = {
// // // //   "", // 0 is reserved
// // // //   "fork",
// // // //   "exit",
// // // //   "wait",
// // // //   "pipe",
// // // //   "read",
// // // //   "kill",
// // // //   "exec",
// // // //   "fstat",
// // // //   "chdir",
// // // //   "dup",
// // // //   "getpid",
// // // //   "sbrk",
// // // //   "sleep",
// // // //   "uptime",
// // // //   "open",
// // // //   "write",
// // // //   "mknod",
// // // //   "unlink",
// // // //   "link",
// // // //   "mkdir",
// // // //   "close",
// // // //   "getsyscount"
// // // // };

// // // // int
// // // // main(int argc, char *argv[])
// // // // {
// // // //   if(argc < 3){
// // // //     fprintf(2, "Usage: syscount mask command [args]\n");
// // // //     exit(1);
// // // //   }

// // // //   int mask = atoi(argv[1]);
// // // //   int counts[32] = {0};
// // // //   int bit_pos = -1;
  
// // // //   // Find which bit is set in mask
// // // //   for(int i = 0; i < 32; i++) {
// // // //     if(mask & (1 << i)) {
// // // //       if(bit_pos != -1) {
// // // //         fprintf(2, "Error: Multiple bits set in mask\n");
// // // //         exit(1);
// // // //       }
// // // //       bit_pos = i;
// // // //     }
// // // //   }

// // // //   if(bit_pos == -1 || bit_pos >= sizeof(syscall_names)/sizeof(syscall_names[0])) {
// // // //     fprintf(2, "Invalid syscall mask\n");
// // // //     exit(1);
// // // //   }

// // // //   int pid = fork();
// // // //   if(pid < 0) {
// // // //     fprintf(2, "fork failed\n");
// // // //     exit(1);
// // // //   }
  
// // // //   if(pid == 0) {  // Child
// // // //     // Prepare arguments for exec
// // // //     char **exec_args = &argv[2];
// // // //     exec_args[argc-2] = 0;  // Ensure null termination
    
// // // //     exec(exec_args[0], exec_args);
// // // //     fprintf(2, "exec %s failed\n", exec_args[0]);
// // // //     exit(1);
// // // //   }

// // // //   // Parent
// // // //   wait(0);
  
// // // //   if(getsyscount(counts) < 0) {
// // // //     fprintf(2, "getsyscount failed\n");
// // // //     exit(1);
// // // //   }

// // // //   printf("PID %d called %s %d times\n", 
// // // //          pid, syscall_names[bit_pos], counts[bit_pos]);
  
// // // //   exit(0);
// // // // }



// // // #include "../kernel/types.h"
// // // #include "../kernel/stat.h"
// // // #include "user/user.h"
// // // #include "../kernel/fcntl.h"

// // // char *syscall_names[] = {
// // //     "", // 0 is reserved
// // //     "fork",
// // //     "exit",
// // //     "wait",
// // //     "pipe",
// // //     "read",
// // //     "kill",
// // //     "exec",
// // //     "fstat",
// // //     "chdir",
// // //     "dup",
// // //     "getpid",
// // //     "sbrk",
// // //     "sleep",
// // //     "uptime",
// // //     "open",
// // //     "write",
// // //     "mknod",
// // //     "unlink",
// // //     "link",
// // //     "mkdir",
// // //     "close",
// // //     "getsyscount"
// // // };

// // // int
// // // main(int argc, char *argv[])
// // // {
// // //     if(argc < 3){
// // //         fprintf(2, "Usage: syscount mask command [args]\n");
// // //         exit(1);
// // //     }

// // //     int syscall_num = atoi(argv[1]);
// // //     int counts[32] = {0};

// // //     // Validate syscall number
  
// // //     // if(syscall_num <= 0 || syscall_num >= sizeof(syscall_names)/sizeof(syscall_names[0])) {
// // //     //     fprintf(2, "Invalid syscall number\n");
// // //     //     exit(1);
// // //     // }
// // //     int shifts = 0;   // Count of shifts

// // //     // Keep right-shifting the number until it becomes 1
// // //     printf("Given number = %d\n",syscall_num);
// // //     while (syscall_num > 1) {
// // //         syscall_num = syscall_num >> 1;
// // //         shifts++;
// // //     }
// // //   syscall_num = shifts;
// // //     printf("Syscall number = %d\n",shifts);
// // //     int pid = fork();
// // //     if(pid < 0) {
// // //         fprintf(2, "fork failed\n");
// // //         exit(1);
// // //     }

// // //     if(pid == 0) { // Child
// // //         // Prepare arguments for exec
// // //         char **exec_args = &argv[2];
// // //         exec_args[argc-2] = 0; // Ensure null termination
// // //         exec(exec_args[0], exec_args);
// // //         fprintf(2, "exec %s failed\n", exec_args[0]);
// // //         exit(1);
// // //     }

// // //     // Parent
// // //     printf("Count = %d\n",counts[syscall_num]);
// // //     wait(0);
// // //     if(getsyscount(counts) < 0) {
// // //         fprintf(2, "getsyscount failed\n");
// // //         exit(1);
// // //     }
// // //     printf("Count = %d\n",counts[syscall_num]);
// // //     // struct proc* p = myproc();
// // //     // printf("syscall count for syscall %d is %d\n", syscall_num, p->syscall_counts[syscall_num]);
// // //     printf("PID %d called %s %d times\n",
// // //            pid, syscall_names[syscall_num], counts[syscall_num]);
// // //     exit(0);
// // // }



// // #include "../kernel/types.h"
// // #include "../kernel/stat.h"
// // #include "user/user.h"
// // #include "../kernel/fcntl.h"

// // char *syscall_names[] = {
// //     "", // 0 is reserved
// //     "fork",
// //     "exit",
// //     "wait",
// //     "pipe",
// //     "read",
// //     "kill",
// //     "exec",
// //     "fstat",
// //     "chdir",
// //     "dup",
// //     "getpid",
// //     "sbrk",
// //     "sleep",
// //     "uptime",
// //     "open",
// //     "write",
// //     "mknod",
// //     "unlink",
// //     "link",
// //     "mkdir",
// //     "close",
// //     "getsyscount"
// // };

// // int
// // main(int argc, char *argv[])
// // {
// //     if(argc < 3){
// //         fprintf(2, "Usage: syscount mask command [args]\n");
// //         exit(1);
// //     }

// //     int mask = atoi(argv[1]);
// //     int counts[32] = {0};
    
// //     // Find which bit is set in the mask
// //     int syscall_num = -1;
// //     for(int i = 0; i < 32; i++) {
// //         if(mask & (1 << i)) {
// //             syscall_num = i;
// //             break;
// //         }
// //     }
    
// //     // Debug print to verify syscall number
// //     printf("Debug: mask %d maps to syscall %d (%s)\n", 
// //            mask, syscall_num, syscall_names[syscall_num]);

// //     if(syscall_num == -1 || syscall_num >= sizeof(syscall_names)/sizeof(syscall_names[0])) {
// //         fprintf(2, "Invalid syscall number\n");
// //         exit(1);
// //     }

// //     // Clear the counts array before fork
// //     memset(counts, 0, sizeof(counts));

// //     int pid = fork();
// //     if(pid < 0) {
// //         fprintf(2, "fork failed\n");
// //         exit(1);
// //     }

// //     if(pid == 0) { // Child
// //         char **exec_args = &argv[2];
// //         exec_args[argc-2] = 0; // Ensure null termination
// //         exec(exec_args[0], exec_args);
// //         fprintf(2, "exec %s failed\n", exec_args[0]);
// //         exit(1);
// //     }

// //     // Parent
// //     wait(0); // Wait for child to finish

// //     if(getsyscount(counts) < 0) {
// //         fprintf(2, "getsyscount failed\n");
// //         exit(1);
// //     }

// //     // Debug print all non-zero counts
// //     printf("Debug: All non-zero syscall counts:\n");
// //     for(int i = 0; i < 32; i++) {
// //         if(counts[i] > 0) {
// //             printf("  Syscall %d (%s): %d times\n", 
// //                    i, syscall_names[i], counts[i]);
// //         }
// //     }

// //     printf("PID %d called %s %d times\n",
// //            pid, syscall_names[syscall_num], counts[syscall_num]);
// //     exit(0);
// // }

#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user/user.h"
#include "../kernel/fcntl.h"


#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

// The syscall names array
char *syscall_names[] = {
    "", // 0 is reserved
    "fork",
    "exit",
    "wait",
    "pipe",
    "read",
    "kill",
    "exec",
    "fstat",
    "chdir",
    "dup",
    "getpid",
    "sbrk",
    "sleep",
    "uptime",
    "open",  // Syscall number 15
    "write",
    "mknod",
    "unlink",
    "link",
    "mkdir",
    "close",
    "getsyscount"
};

int main(int argc, char *argv[])
{
    if(argc < 3){
        fprintf(2, "Usage: syscount mask command [args]\n");
        exit(1);
    }

    int syscall_mask = atoi(argv[1]);  // Get the mask (e.g., 32768)
    int mask = syscall_mask;
    // Create a child process to execute the command
    int pid = fork();
    if(pid < 0) {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if(pid == 0) { // Child process
        // Execute the given command (argv[2] and beyond)
        char **exec_args = &argv[2];
        exec_args[argc-2] = 0;
        exec(exec_args[0], exec_args);
        fprintf(2, "exec %s failed\n", exec_args[0]);
        exit(1);
    }

    // Parent process waits for the child to finish
    int child_status;
    wait(&child_status);

    // Get the syscall count for the given mask
    int count = getsyscount(syscall_mask);
    if(count < 0) {
        fprintf(2, "getsyscount failed\n");
        exit(1);
    }

    // int specific_syscall;
    
    int specific_syscall = -1; // Initialize to invalid value
    int i=0;
    for (i = 0; i < 32; i++) {
        if (mask & (1 << i)) {  // Check if the ith bit is set
            specific_syscall = i;       // Set the specific syscall index
            // break;                      // Assume only one bit is set, so break
        }
    }
    // specific_syscall++;
    printf("Specific %d i %d\n",specific_syscall,i);
    if(specific_syscall != -1 && specific_syscall < NELEM(syscall_names)) {
        printf("PID %d called %s %d %d times\n", pid, syscall_names[specific_syscall], specific_syscall, count);
    } else {
        fprintf(2, "Invalid syscall mask\n");
    }

    exit(0);
}


// #include "../kernel/types.h"
// #include "../kernel/stat.h"
// #include "user/user.h"
// #include "../kernel/fcntl.h"

// #define NELEM(x) (sizeof(x) / sizeof((x)[0]))

// // The syscall names array
// char *syscall_names[] = {
//     "", // 0 is reserved
//     "fork",
//     "exit",
//     "wait",
//     "pipe",
//     "read",
//     "kill",
//     "exec",
//     "fstat",
//     "chdir",
//     "dup",
//     "getpid",
//     "sbrk",
//     "sleep",
//     "uptime",
//     "open",  // Syscall number 15
//     "write",
//     "mknod",
//     "unlink",
//     "link",
//     "mkdir",
//     "close",
//     "getsyscount"
// };

// int main(int argc, char *argv[])
// {
//     if(argc < 3){
//         fprintf(2, "Usage: syscount mask command [args]\n");
//         exit(1);
//     }

//     int syscall_mask = atoi(argv[1]);  // Get the mask (e.g., 32768)
//     // int mask = syscall_mask;
//     // Create a child process to execute the command
//     int pid = fork();
//     if(pid < 0) {
//         fprintf(2, "fork failed\n");
//         exit(1);
//     }

//     if(pid == 0) { // Child process
//         // Execute the given command (argv[2] and beyond)
//         char **exec_args = &argv[2];
//         exec_args[argc-2] = 0;
//         exec(exec_args[0], exec_args);
//         fprintf(2, "exec %s failed\n", exec_args[0]);
//         exit(1);
//     }

//     // Parent process waits for the child to finish
//     int child_status;
//     wait(&child_status);

//     // Get the syscall count for the given mask
//     int count = getsyscount(syscall_mask);
//     if(count < 0) {
//         fprintf(2, "getsyscount failed\n");
//         exit(1);
//     }

//     // Find which syscall we were tracking based on the mask
//     int shifts = 0;
//    int num = syscall_mask;
//    while (num > 1) {
//         num = num >> 1;
//         shifts++;
//     }
//     printf("Specific %d i %d\n",shifts,syscall_mask);
//     if(num != -1 && num < NELEM(syscall_names)) {
//         printf("PID %d called %s %d times\n", pid, syscall_names[shifts], count);
//     } else {
//         fprintf(2, "Invalid syscall mask\n");
//     }

//     exit(0);
// }
