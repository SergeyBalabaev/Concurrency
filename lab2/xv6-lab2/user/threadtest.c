#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"

void
worker(void *arg)
{
  int id = (int)(uint64)arg;
  for(int i = 0; i < 10; i++){
    printf("thread %d (tid %d): line %d\n", id, gettid(), i);
  }
  thread_exit();
}

int
main(void)
{
  int tids[4];

  printf("main: tid %d\n", gettid());

  for(int i = 0; i < 4; i++)
    tids[i] = thread_create(worker, (void*)(uint64)i);

  for(int i = 0; i < 4; i++)
    thread_join_t(tids[i]);

  printf("main: all joined\n");
  exit(0);
}