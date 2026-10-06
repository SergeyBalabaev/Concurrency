#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"
#include "user/uthread.h"

static struct {
  int tid;
  void *stack;
} threads[MAX_THREADS];

int
thread_create(void (*fn)(void *), void *arg)
{
  /// ТУТ ВАШ КОД
}

int
thread_join_t(int tid)
{
  /// ТУТ ВАШ КОД
}

void
thread_exit(void)
{
  exit(0);
}