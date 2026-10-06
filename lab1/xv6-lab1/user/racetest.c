#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/timing.h"

#define N_ITER 100000000

volatile int counter = 0;

void
worker(void *arg)
{
  //printf("start tid %d t=%lu\n", gettid(), r_time());
  for(int i = 0; i < N_ITER; i++)
    counter++;
  thread_exit();
}

void
worker_atomic(void *arg)
{
  for(int i = 0; i < N_ITER; i++)
    __sync_fetch_and_add(&counter, 1);   // починка
  thread_exit();
}

int
main(int argc, char *argv[])
{
  void (*fn)(void*) = worker;
  if(argc > 1 && argv[1][0] == 'a')
    fn = worker_atomic;             // racetest a - атомарная версия

  counter = 0;
  int t1 = thread_create(fn, 0);
  int t2 = thread_create(fn, 0);
  thread_join_t(t1);
  thread_join_t(t2);

  printf("expected %d, got %d, lost %d\n",
         2*N_ITER, counter, 2*N_ITER - counter);
  exit(0);
}