#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "user/user.h"
#pragma GCC push_options
#pragma GCC optimize ("O0") // Causing wierd errors of moving things here and there

void usless_work() {
    for (int i = 0; i < 1000 * 900000; i++) {
        asm volatile("nop"); // avoid compiler optimizing away loop
    }
}


void test0(){
    settickets(600);// So that parent will get the higher priority and the forks can run at once
    printf("TEST 0\n"); // To check the randomness
    int prog1_tickets = 1;
    int prog2_tickets = 4;
    int prog3_tickets = 16;
    int prog4_tickets = 8;
    printf("Child 1 has %d tickets.\nChild 2 has %d tickets\nChild 3 has %d tickets\nChild 4 has %d tickets\n",
           prog1_tickets, prog2_tickets, prog3_tickets, prog4_tickets);

    if (fork() == 0) {
        settickets(prog1_tickets);
        printf("Child 1 started %d\n",getpid());
        sleep(1);
        usless_work();
        printf("Child 1 exited %d\n",getpid());
        exit(0);

    }
    if (fork() == 0) {
        settickets(prog2_tickets);
        printf("Child 2 started %d\n",getpid());
        sleep(1);
        usless_work();
        printf("Child 2 exited %d\n",getpid());
        exit(0);
    }
    if (fork() == 0) {
        settickets(prog3_tickets);
        printf("Child 3 started %d\n",getpid());
        sleep(1);
        usless_work();
        printf("Child 3 exited %d\n",getpid());
        exit(0);
    }
    if (fork() == 0) {
        settickets(prog4_tickets);
        printf("Child 4 started %d\n",getpid());
        sleep(1);
        usless_work();
        printf("Child 4 exited %d\n",getpid());
        exit(0);
    }
    wait(0);
    wait(0);
    wait(0);
    wait(0);
    printf("The correct order should be ideally 3,4,2,1.\n");

}

void test1(){
    printf("TEST1\n"); // To check the FCFS part of the implementation
    int tickets = 30; // To check for this finish times
    settickets(30); // So that now, the parent will always get the main priority to set up its children
    sleep(1); // So that this will have a different ctime than others. Ctime is not entirely very accurate

    printf("Child 1 started %d\n",getpid());
    if (fork() == 0) {
        settickets(tickets);
        usless_work();
        printf("Child 1 ended %d\n",getpid());
        exit(0);
    }
    printf("Child 2 started %d\n",getpid());
    if (fork() == 0) {
        settickets(tickets);
        usless_work();
        printf("Child 2 ended %d\n",getpid());
        exit(0);
    }
    printf("Child 3 started %d\n",getpid());
    if (fork() == 0) {
        settickets(tickets);
        usless_work();
        printf("Child 3 ended %d\n",getpid());
        exit(0);
    }
    wait(0);
    wait(0);
    wait(0);
    printf("The order should be 3 and 2 then 1 since all tickets have same value\n");
}
int main() {
    test0();
    test1();
    printf("Finished all tests\n");

    return 0;
}



// void scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu();

//   c->proc = 0;
//   for (;;)
//   {
//     // Avoid deadlock by ensuring that devices can interrupt.
//     intr_on();

//     #ifdef LBS

//       int total_tickets = 0;
//     // First pass: count total tickets of RUNNABLE processes
//     for(p = proc; p < &proc[NPROC]; p++) {
//       acquire(&p->lock);
//       if(p->state == RUNNABLE) {
//         total_tickets += p->tickets;
//       }
//       release(&p->lock);
//     }
//     // printf("Total Tickets %d\n",total_tickets);
//     if(total_tickets > 0) {
//       // Generate a random number between 0 and total_tickets - 1
//       int winner = rand() % total_tickets;

//       struct proc *chosen = 0;
//       int current_tickets = 0;

//       // Second pass: find the winning process
//       for(p = proc; p < &proc[NPROC]; p++) {
//         acquire(&p->lock);
//         if(p->state == RUNNABLE) {
//           current_tickets += p->tickets;
//           if(current_tickets > winner && (!chosen || p->tickets == chosen->tickets)) {
//             if(!chosen || p->creation_time < chosen->creation_time) {
//               if(chosen)
//                 release(&chosen->lock);
//               chosen = p;
//               continue;
//             }
//           }
//         }
//         release(&p->lock);
//       }

//       if(chosen) {
//         chosen->state = RUNNING;
//         c->proc = chosen;
//         swtch(&c->context, &chosen->context);
//         c->proc = 0;
//         release(&chosen->lock);
//       }
//     }
//     // int total_tickets = 0;
//     //     struct proc* runnable_procs[NPROC];
//     //     int runnable_count = 0;

//     //     // Single pass: count total tickets and collect runnable processes
//     //     for (p = proc; p < &proc[NPROC]; p++) {
//     //         acquire(&p->lock);
//     //         if (p->state == RUNNABLE) {
//     //             total_tickets += p->tickets;
//     //             runnable_procs[runnable_count++] = p;
//     //             // Don't release the lock yet
//     //         } else {
//     //             release(&p->lock);
//     //         }
//     //     }

//     //     if (total_tickets > 0) {
//     //         // Generate a random number between 0 and total_tickets - 1
//     //         int winner = rand() % total_tickets;
//     //         struct proc *chosen = 0;
//     //         int current_tickets = 0;

//     //         // Find the winning process
//     //         for (int i = 0; i < runnable_count; i++) {
//     //             p = runnable_procs[i];
//     //             current_tickets += p->tickets;
//     //             if (current_tickets > winner) {
//     //                 if (!chosen || p->creation_time < chosen->creation_time) {
//     //                     chosen = p;
//     //                 }
//     //                 break;
//     //             }
//     //         }
//     //         // printf("winner %d\n",winner);
//     //         // Release locks for non-chosen processes
//     //         for (int i = 0; i < runnable_count; i++) {
//     //             if (runnable_procs[i] != chosen) {
//     //                 release(&runnable_procs[i]->lock);
//     //             }
//     //         }

//     //         if (chosen) {
//     //             // Update process statistics
//     //             chosen->run_count++;
//     //             chosen->total_tickets_used += chosen->tickets;

//     //             // Run the chosen process
//     //             chosen->state = RUNNING;
//     //             c->proc = chosen;
//     //             swtch(&c->context, &chosen->context);
//     //             c->proc = 0;

//     //             // Dynamic ticket adjustment (optional)
//     //             if (chosen->run_count % 10 == 0 && chosen->tickets > 1) {
//     //                 chosen->tickets--;
//     //             }
//     //             // printf("came");
//     //             release(&chosen->lock);
//     //         }
//     //     } else {
//     //         // Release all locks if no runnable processes
//     //         for (int i = 0; i < runnable_count; i++) {
//     //             release(&runnable_procs[i]->lock);
//     //         }
//     //     }
//     // #elif MLFQ
//       // printf("came\n");
//       //  mlfq_init();
//       // ticks_since_boost++;
//       //   if (ticks_since_boost >= 48) {
//       //       priority_boost();
//       //   }

//       //   // Find the highest priority non-empty queue
//       //   int current_priority = 0;
//       //   while (current_priority < NPRIO && priority_queues[current_priority].size == 0) {
//       //       current_priority++;
//       //   }
//       //   // printf("cameee\n");
//       //   if (current_priority < NPRIO) {
//       //       p = dequeue(&priority_queues[current_priority]);
//       //       if (p) {
//       //           acquire(&p->lock);
//       //           if (p->state == RUNNABLE) {
//       //               p->state = RUNNING;
//       //               c->proc = p;
//       //               p->ticks_in_queue = 0;

//       //               swtch(&c->context, &p->context);

//       //               // Process is done running for now.
//       //               // It should have changed its p->state before coming back.
//       //               c->proc = 0;

//       //               if (p->state == RUNNABLE) {
//       //                   // If process used its full time slice, move it to a lower priority queue
//       //                   if (p->ticks_in_queue >= time_slices[current_priority]) {
//       //                       if (current_priority < NPRIO - 1) {
//       //                           p->priority = current_priority + 1;
//       //                       }
//       //                       p->ticks_in_queue = 0;
//       //                   }
//       //                   // Requeue the process
//       //                   enqueue(&priority_queues[p->priority], p);
//       //               }
//       //           }
//       //           release(&p->lock);
//       //       }
//       //   }
//       #endif
//       #ifdef MLFQ
//   mlfq_init();
//   ticks_since_boost++;
//   if (ticks_since_boost >= 48) {
//       priority_boost();
//   }

//   // Find the highest priority non-empty queue
//   int current_priority = 0;
//   while (current_priority < NPRIO && priority_queues[current_priority].size == 0) {
//       current_priority++;
//   }

//   if (current_priority < NPRIO) {
//       p = dequeue(&priority_queues[current_priority]);
//       if (p) {
//           acquire(&p->lock);
//           if (p->state == RUNNABLE) {
//               p->state = RUNNING;
//               c->proc = p;
//               p->ticks_in_queue = 0;

//               swtch(&c->context, &p->context);

//               c->proc = 0;

//               if (p->state == RUNNABLE) {
//                   // If process used its full time slice, move it to a lower priority queue
//                   if (p->ticks_in_queue >= time_slices[current_priority]) {
//                       if (current_priority < NPRIO - 1) {
//                           p->priority = current_priority + 1;
//                       }
//                       p->ticks_in_queue = 0;
//                   }
//                   // Requeue the process
//                   enqueue(&priority_queues[p->priority], p);
//               }
//           }
//           release(&p->lock);
//       }
//   }
// #endif

//     #ifdef RR

//     for (p = proc; p < &proc[NPROC]; p++)
//     {
//       acquire(&p->lock);
//       if (p->state == RUNNABLE)
//       {
//         // printf("hiii");
//         // Switch to chosen process.  It is the process's job
//         // to release its lock and then reacquire it
//         // before jumping back to us.
//         p->state = RUNNING;
//         c->proc = p;
//         swtch(&c->context, &p->context);

//         // Process is done running for now.
//         // It should have changed its p->state before coming back.
//         c->proc = 0;
//       }
//       release(&p->lock);
//     }

//     #endif

//   }
// }
