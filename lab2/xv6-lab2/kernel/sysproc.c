#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "futex.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
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
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
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
  return kkill(pid);
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
sys_clone(void)
{
  uint64 fn, arg, stack, ctid;
  argaddr(0, &fn);
  argaddr(1, &arg);
  argaddr(2, &stack);
  argaddr(3, &ctid);
  return clone(fn, arg, stack, ctid);
}

uint64
sys_thread_join(void)
{
  int tid;
  argint(0, &tid);
  return thread_join(tid);
}

uint64
sys_gettid(void)
{
  return myproc()->pid;
}


// futex(uaddr, op, val)
//   op = FUTEX_WAIT: val - ожидаемое значение слова
//   op = FUTEX_WAKE: val - сколько потоков разбудить (максимум)
uint64
sys_futex(void)
{
  uint64 uaddr;
  int op, val;

  argaddr(0, &uaddr);
  argint(1, &op);
  argint(2, &val);

  switch(op){
  case FUTEX_WAIT:
    return futex_wait(uaddr, (uint32)val);
  case FUTEX_WAKE:
    if(val < 0)
      return -1;
    return futex_wake(uaddr, val);
  default:
    return -1;
  }
}

// futexstats(struct futexstat *s): скопировать счётчики пользователю
uint64
sys_futexstats(void)
{
  extern struct futexstat futexstat;
  struct futexstat snap;
  uint64 uaddr;

  argaddr(0, &uaddr);

  // Счётчики увеличиваются атомарно и без общей блокировки, поэтому
  // снимок не строго согласован между полями. Для замеров это не
  // важно: мы всегда берём разность двух снимков вокруг сценария.
  snap = futexstat;

  struct proc *p = myproc();
  if(copyout(p->pagetable, uaddr, (char*)&snap, sizeof(snap)) < 0)
    return -1;
  return 0;
}
// futex_requeue(uaddr, nwake, uaddr2, nmove, cmpval) - доп. задание
uint64
sys_futex_requeue(void)
{
  uint64 uaddr, uaddr2;
  int nwake, nmove, cmpval;

  argaddr(0, &uaddr);
  argint(1, &nwake);
  argaddr(2, &uaddr2);
  argint(3, &nmove);
  argint(4, &cmpval);
  if(nwake < 0 || nmove < 0)
    return -1;
  return futex_requeue(uaddr, nwake, uaddr2, nmove, (uint32)cmpval);
}
