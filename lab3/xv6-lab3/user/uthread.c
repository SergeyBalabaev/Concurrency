#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/futex.h"
#include "user/user.h"
#include "user/uthread.h"

// Библиотека потоков из БДЗ-1 с join через futex из БДЗ-2 (блок 2,
// раздел 4.9 методички БДЗ-2 - код оттуда).
//
// Слово alive - это ctid-слово: thread_create пишет в него 1 ДО clone,
// ядро при выходе потока пишет 0 и делает FUTEX_WAKE. thread_join_t
// ждёт нуля на futex, затем системным вызовом thread_join убирает
// запись ZOMBIE и освобождает стек.
//
// Ограничение (как и раньше): таблицу threads[] меняет только тот поток,
// который создаёт и присоединяет потоки. В БДЗ-3 это всегда main.

static struct {
  int tid;
  void *stack;
  volatile int alive;
} threads[MAX_THREADS];

int
thread_create(void (*fn)(void *), void *arg)
{
  int i;
  for(i = 0; i < MAX_THREADS; i++)
    if(threads[i].tid == 0)
      break;
  if(i == MAX_THREADS)
    return -1;

  void *stack = malloc(PGSIZE);
  if(stack == 0)
    return -1;

  threads[i].alive = 1;                         // ДО clone, см. БДЗ-2, 4.9
  int tid = clone(fn, arg, stack, (int *)&threads[i].alive);
  if(tid < 0){
    threads[i].alive = 0;
    free(stack);
    return -1;
  }
  threads[i].tid = tid;
  threads[i].stack = stack;
  return tid;
}

int
thread_join_t(int tid)
{
  int i;
  if(tid <= 0)
    return -1;
  for(i = 0; i < MAX_THREADS; i++)
    if(threads[i].tid == tid)
      break;
  if(i == MAX_THREADS)
    return -1;

  int v;
  while((v = threads[i].alive) != 0)
    futex((int *)&threads[i].alive, FUTEX_WAIT, v);

  int r = thread_join(tid);      // убрать останки: запись proc, trapframe
  if(r < 0)
    return r;

  free(threads[i].stack);
  threads[i].stack = 0;
  threads[i].tid = 0;
  return r;
}

void
thread_exit(void)
{
  exit(0);
}
