#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}



uint64
sys_getsyscount(void)
{
    int syscall_num;
    // Get the syscall number bitmask from user space
    argint(0, &syscall_num);
    // printf("number %d\n",syscall_num);
    struct proc *p = myproc();
    
    int total_count = 0; // Initialize total count for the selected syscall
    // Iterate over each syscall count in the syscall_counts array
    printf("num %d\n",syscall_num);
    // for (int i = 0; i < NELEM(p->syscall_counts); i++) {
      int i=0;
    while(i<NELEM(p->syscall_counts)){
        // Check if the ith syscall is included in the mask
        if (syscall_num & (1 << i)) {  
            total_count += p->syscall_counts[i]; // Add to the total count
            break;
        }
        i++;
    }
    
    printf("toatal %d\n",total_count);
    for(int j=0;j<NELEM(p->syscall_counts);j++){
      printf("Syscall %d: %d times\n" ,j,p->syscall_counts[j]);
    }
    // Return the total count of syscalls specified by the mask
    return total_count;
}



uint64 sys_sigalarm(void)
{
  int ticks;
  uint64 handler;

  argint(0, &ticks);
  argaddr(1, &handler);

  struct proc *p = myproc();
  acquire(&p->lock);
  p->ticks = ticks;
  p->handler = handler;
  p->cur_ticks = 0;
  p->alarm_on = 0;
  release(&p->lock);

  return 0;
}



uint64 sys_sigreturn(void)
{
  struct proc *p = myproc();
  
  if(p->alarm_tf == 0)
    return -1;  // Return error if no saved trapframe exists
  
  acquire(&p->lock);
  // Restore all registers from saved trapframe
  memmove(p->trapframe, p->alarm_tf, sizeof(struct trapframe));
  
  // Reset alarm state
  p->alarm_on = 0;
  p->cur_ticks = 0;
  
  // Save the value of a0 before releasing the lock
  uint64 ret = p->trapframe->a0;
  
  release(&p->lock);
  
  return ret;
}

uint64
sys_settickets(void)
{
  int number;
  argint(0, &number);
  return gettickets(number);
}