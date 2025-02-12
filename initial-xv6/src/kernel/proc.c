#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;
int counter = 0;
extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

#define NPROC 64  // maximum number of processes
#define NPRIO 4  
int ticks_used=0;
// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;
  // memset(p->syscall_counts, 0, sizeof(p->syscall_counts));
  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == UNUSED) 
    {
      goto found;
    }
    else
    {
      release(&p->lock);
    }
  }
  // memset(p->syscall_counts, 0, sizeof(p->syscall_counts));
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;


  p->cur_ticks = 0;          // Initialize current ticks to 0
  p->ticks = 0;              // Initialize the alarm ticks threshold to 0 (no alarm by default)
  p->alarm_on = 0;           // Alarm is not active initially
  p->handler = 0;            // Initialize the handler to 0 (no handler set)
  p->alarm_tf = 0;           // No saved trapframe for alarm yet

  p->priority = 0;
p->time_slice = 1;
p->total_ticks = 0;


   p->tickets = 1;  // Default to 1 ticket
  p->creation_time = counter++;
  //memset(p->syscall_counts, 0, sizeof(p->syscall_counts));
  // Allocate a trapframe page.
  if ((p->trapframe = (struct trapframe *)kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  p->rtime = 0;
  p->etime = 0;
  p->ctime = ticks;
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if (p->trapframe)
    kfree((void *)p->trapframe);
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE,
               (uint64)trampoline, PTE_R | PTE_X) < 0)
  {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->trapframe), PTE_R | PTE_W) < 0)
  {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0)
  {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0)
    {
      return -1;
    }
  }
  else if (n < 0)
  {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }
  memset(np->syscall_counts, 0, sizeof(np->syscall_counts));
  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  {
    freeproc(np);
    release(&np->lock);
    return -1;
  }

  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

   np->tickets = p->tickets;  
  // memset(np->syscall_counts, 0, sizeof(np->syscall_counts));
  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);
  
  // for(int i=0;i<32;i++){
  //   printf("System call %d value before %d\n",i,p->syscall_counts[i]);
  // }
  // np->syscall_mask = p->syscall_mask;
  
  // for(int i = 0; i < NELEM(p->syscall_counts); i++){
  //   np->syscall_counts[i] = p->syscall_counts[i];
  // }
  // for(int i=0;i<32;i++){
  //   printf("System call %d value after %d\n",i,p->syscall_counts[i]);
  // }
  // return pid;
  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++)
  {
    if (pp->parent == p)
    {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status)
{
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd])
    {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;
  p->etime = ticks;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++)
    {
      if (pp->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE)
        {
          for(int i=0;i<32;i++){
            p->syscall_counts[i]+=pp->syscall_counts[i];
          }
          // Found one.
          pid = pp->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                   sizeof(pp->xstate)) < 0)
          {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p))
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}


int gettickets(int number) {
  if(number <= 0)
    return -1;
  myproc()->tickets = number;
  return number;
}


uint random_seed = 1;

uint rand(void) {
  random_seed = random_seed * 1103515245 + 12345;
  return (random_seed / 65536) % 32768;
}

uint32 xorshift32(void) {
    static uint32 x = 123456789;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

struct mlfq_queue {
    struct proc* processes[NPROC];
    int front;
    int rear;
    int size;
};

struct mlfq_queue priority_queues[NPRIO];
int time_slices[NPRIO] = {1, 4, 8, 16};  // time slices for each priority level
int ticks_since_boost = 0;

// Queue operations
// void enqueue(struct mlfq_queue* q, struct proc* p) {
//   printf("hii\n");
//     if (q->size < NPROC) {
//         q->rear = (q->rear + 1) % NPROC;
//         q->processes[q->rear] = p;
//         q->size++;
//     }
//     printf("Enqueue: PID %d Priority %d\n", p->pid, p->priority);
// }

void enqueue(struct mlfq_queue* q, struct proc* p) {
  printf("Enqueueing PID %d into queue %d\n", p->pid, p->priority);
  if (q->size < NPROC) {
    q->rear = (q->rear + 1) % NPROC;
    q->processes[q->rear] = p;
    q->size++;
    printf("Queue size after enqueue: %d\n", q->size);
  } else {
    printf("Queue is full, cannot enqueue\n");
  }
}

// struct proc* dequeue(struct mlfq_queue* q) {
//   printf("hello\n");
//     if (q->size > 0) {
//         struct proc* p = q->processes[q->front];
//         q->front = (q->front + 1) % NPROC;
//         q->size--;
//         return p;
//     }
//     return 0;
// }

struct proc* dequeue(struct mlfq_queue* q) {
  printf("Attempting to dequeue from queue size %d\n", q->size);
  if (q->size > 0) {
    struct proc* p = q->processes[q->front];
    q->front = (q->front + 1) % NPROC;
    q->size--;
    printf("Dequeued PID %d, new queue size: %d\n", p->pid, q->size);
    return p;
  }
  printf("Queue is empty, cannot dequeue\n");
  return 0;
}

void mlfq_init() {
  
    for (int i = 0; i < NPRIO; i++) {
        priority_queues[i].front = 0;
        priority_queues[i].rear = -1;
        priority_queues[i].size = 0;
    }
    // printf("initializing");
}

void priority_boost() {

    for (int i = 1; i < NPRIO; i++) {
      // printf("io\n");
        while (priority_queues[i].size > 0) {
          printf("entered");
            struct proc* p = dequeue(&priority_queues[i]);
            enqueue(&priority_queues[0], p);
            p->priority = 0;
            // p->ticks_in_queue = 0;
        }
    }
    ticks_since_boost = 0;
    // printf("Priority boost executed\n");
}

int is_in_queue(struct proc* p) {
  for (int i = 0; i < NPRIO; i++) {
    for (int j = 0; j < priority_queues[i].size; j++) {
      if (priority_queues[i].processes[(priority_queues[i].front + j) % NPROC] == p) {
        return 1; // Process is already in the queue
      }
    }
  }
  return 0; // Process is not in any queue
}


// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.

void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    #ifdef LBS
      int total_tickets = 0;

      // First pass: count total tickets of RUNNABLE processes
      for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->state == RUNNABLE) {
          total_tickets += p->tickets;
        }
        release(&p->lock);
      }

      // if(total_tickets > 0) {
      //   // Generate a random number between 0 and total_tickets - 1
      //   int winner = rand() % total_tickets;

      //   struct proc *chosen = 0;
      //   int current_tickets = 0;
      //   // printf("Winner = %d\n",winner);
      //   // Second pass: find the winning process
      //   // for(p = proc; p < &proc[NPROC]; p++) {
      //   //   acquire(&p->lock);
      //   //   if(p->state == RUNNABLE) {
      //   //     current_tickets += p->tickets;
      //   //     if(current_tickets > winner) {
      //   //       // Select the process with the smallest creation_time in case of a tie

      //   //       if(!chosen || (p->creation_time < chosen->creation_time && p->tickets == chosen->tickets)) {
      //   //         printf("PID %d with time %d \n",p->pid,p->creation_time);
      //   //         if(chosen){
      //   //           printf("hi\n");
      //   //           release(&chosen->lock); // release lock of previously chosen process
      //   //         }
      //   //         chosen = p;
      //   //         break; // no need to continue once the winner is found
      //   //       }
      //   //     }
      //   //   }
      //   //   release(&p->lock);
      //   // }
      //   int flag = 0;
      //   for(p = proc; p < &proc[NPROC]; p++) {
      //     acquire(&p->lock);
      //     if(p->state == RUNNABLE) {
      //       current_tickets += p->tickets;
      //       if(current_tickets > winner) {
      //         // Select the process with the smallest creation_time in case of a tie
      //         for(chosen=proc ; chosen< &proc[NPROC] ; chosen++){
      //           if(chosen->state == RUNNABLE){
      //             if((p->creation_time > chosen->creation_time && p->tickets == chosen->tickets)) {
      //               printf("PID %d with time %d \n",p->pid,chosen->pid);
      //               flag = 1;
      //               // if(chosen){
      //               //   printf("hi\n");
      //               //   release(&chosen->lock); // release lock of previously chosen process
      //               // }
      //               // chosen = p;
      //               break; // no need to continue once the winner is found
      //             }
      //           }
      //           else{
      //             continue;
      //           }
      //         }
      //         if(flag == 1){
      //           release(&p->lock);
      //           break;
      //         }
      //       }
      //     }
      //     release(&p->lock);
      //   }

      //   if(chosen) {
      //     chosen->state = RUNNING;
      //     c->proc = chosen;
      //     swtch(&c->context, &chosen->context);
      //     c->proc = 0;
      //     release(&chosen->lock);
      //   }
      // }
    if (total_tickets > 0) {
    int winner = rand() % total_tickets;  // Randomly choose a winner ticket
    // printf("Total tickets: %d, Winner: %d\n", total_tickets, winner);  // Debug output
    struct proc *chosen = 0;
    int current_tickets = 0;

    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE) {
            // printf("Process PID %d, tickets: %d\n", p->pid, p->tickets);
            current_tickets += p->tickets;
            // printf("Current tickets: %d\n", current_tickets);

            if (current_tickets > winner) {
                // printf("Process PID %d chosen\n", p->pid);
                if (!chosen) {
                    chosen = p;  // Set chosen, do not release lock yet
                } else {
                    if (p->tickets > chosen->tickets || 
                        (p->tickets == chosen->tickets && p->creation_time < chosen->creation_time)) {
                        release(&chosen->lock);  // Only release previously chosen process
                        chosen = p;
                    } else {
                        release(&p->lock);  // If not chosen, release immediately
                    }
                }

                if (current_tickets > winner + p->tickets) {
                    // If we've selected the process, break the loop, but do NOT release the chosen process yet.
                    break;
                }
            } else {
                release(&p->lock);  // Release if the process is not chosen
            }
        } else {
            release(&p->lock);  // Release if the process is not RUNNABLE
        }
    }

    // No more panic here
    // Ensure a process was chosen before switching
    if (!chosen) {
        panic("No process chosen to run");
    }

    // Switch to the chosen process
    if (chosen) {
        chosen->state = RUNNING;
        c->proc = chosen;
        swtch(&c->context, &chosen->context);
        c->proc = 0;
        release(&chosen->lock);  // Release chosen lock after context switch
    }
}


    #elif MLFQ
      int found = 0;
    // Loop through priority levels
    for(int priority = 0; priority < 4; priority++) {
      // Find a runnable process in the current priority level
      for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->state == RUNNABLE && p->priority == priority) {
          // Switch to chosen process.
          // printf("%d %d\n",p->priority,p->pid);
          // procdump();
          p->state = RUNNING;
          c->proc = p;
          swtch(&c->context, &p->context);

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          

          found = 1;
          p->total_ticks++;
          ticks_used++;
          if(p->time_slice > 0) {
            p->time_slice--;
          }
          if(p->time_slice == 0) {
            // printf("came\n");
            // Move to next lower priority queue
            if(p->priority < 3) {
              printf("%d\n",p->priority);
              p->priority++;
              printf("%d %d\n",p->priority,p->pid);
              switch(p->priority) {
                case 1: p->time_slice = 4; break;
                case 2: p->time_slice = 8; break;
                case 3: p->time_slice = 16; break;
              }
            } else {
              // For lowest priority, reset time slice
              p->time_slice = 16;
            }
          }
          c->proc = 0;
          release(&p->lock);
          break;
        }
        release(&p->lock);
      }
      struct proc* inc;
      if( ticks_used% 480000 == 0) {
        // printf("PID %d\n",p->pid);
        for(inc = proc; inc < &proc[NPROC]; inc++) {
          acquire(&inc->lock);
          inc->priority = 0;
          inc->time_slice = 1;
          release(&inc->lock);
        }
      }
      if(found) break;
    }
    #else
      // Round Robin: Iterate over all processes and run the first RUNNABLE one
      for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE) {
          p->state = RUNNING;
          c->proc = p;
          swtch(&c->context, &p->context);

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
        }
        release(&p->lock);
      }
      

    #endif
  }
}

// void
// tick(void)
// {
//   struct proc *p = myproc();
//   if(p) {
//     acquire(&p->lock);
//     p->total_ticks++;
//     if(p->time_slice > 0) {
//       p->time_slice--;
//     }
//     if(p->time_slice == 0) {
//       // Move to next lower priority queue
//       if(p->priority < 3) {
//         p->priority++;
//         switch(p->priority) {
//           case 1: p->time_slice = 4; break;
//           case 2: p->time_slice = 8; break;
//           case 3: p->time_slice = 16; break;
//         }
//       } else {
//         // For lowest priority, reset time slice
//         p->time_slice = 16;
//       }
//       p->state = RUNNABLE;
//     }
    
//     // Priority boosting
//     if(p->total_ticks % 48 == 0) {
//       p->priority = 0;
//       p->time_slice = 1;
//     }
//     release(&p->lock);
//   }

//   wakeup(&ticks);
// }

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  // Reset time slice but keep priority
  switch(p->priority) {
    case 0: p->time_slice = 1; break;
    case 1: p->time_slice = 4; break;
    case 2: p->time_slice = 8; break;
    case 3: p->time_slice = 16; break;
  }
  sched();
  release(&p->lock);
  // struct proc *p = myproc();
  // acquire(&p->lock);
  // p->state = RUNNABLE;
  // sched();
  // release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p != myproc())
    {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan)
      {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->pid == pid)
    {
      p->killed = 1;
      if (p->state == SLEEPING)
      {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc *p)
{
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if (user_dst)
  {
    return copyout(p->pagetable, dst, src, len);
  }
  else
  {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if (user_src)
  {
    return copyin(p->pagetable, dst, src, len);
  }
  else
  {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [USED] "used",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    // printf("%d %s %s %d %d\n", p->pid, state, p->name, p->priority, p->ticks_in_queue);
    printf("PID: %d Pri: %d State: %s Name: %s Tickets: %d Time: %d", p->pid,p->priority, state, p->name,p->tickets,p->creation_time);
    printf("\n");
  }
}

// waitx
int waitx(uint64 addr, uint *wtime, uint *rtime)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (np = proc; np < &proc[NPROC]; np++)
    {
      if (np->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if (np->state == ZOMBIE)
        {
          // Found one.
          pid = np->pid;
          *rtime = np->rtime;
          *wtime = np->etime - np->ctime - np->rtime;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                   sizeof(np->xstate)) < 0)
          {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || p->killed)
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

void update_time()
{
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == RUNNING)
    {
      p->rtime++;
    }
    release(&p->lock);
  }
}
