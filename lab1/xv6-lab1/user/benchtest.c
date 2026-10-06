#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/timing.h"

#define N 100

void empty_thread(void *arg) { thread_exit(); }

int
main(void)
{
  uint64 t0, t1;

  // --- clone + join ---
  t0 = r_time();
  for(int i = 0; i < N; i++){
    int tid = thread_create(empty_thread, 0);
    thread_join_t(tid);
  }
  t1 = r_time();
  printf("clone+join: %lu us total, %lu us each\n",
         (t1-t0)/TICKS_PER_US, (t1-t0)/TICKS_PER_US/N);

  // --- fork + wait ---
  t0 = r_time();
  for(int i = 0; i < N; i++){
    int pid = fork();
    if(pid == 0)
      exit(0);
    wait(0);
  }
  t1 = r_time();
  printf("fork+wait:  %lu us total, %lu us each\n",
         (t1-t0)/TICKS_PER_US, (t1-t0)/TICKS_PER_US/N);

  exit(0);
}