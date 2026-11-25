// poolbench — эксперимент Э1: поток на задачу против пула.
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/uwaitgroup.h"
#include "user/uthread.h"
#include "user/timing.h"

#define N 480                         // кратно 8

static int counter;
static struct waitgroup wg;

static void
tiny(void *arg)                         // задача - для пула
{
  __atomic_fetch_add(&counter, 1, __ATOMIC_RELAXED);
  wg_done(&wg);
}

static void
tiny_thread(void *arg)                  // та же работа - для отдельного потока
{
  __atomic_fetch_add(&counter, 1, __ATOMIC_RELAXED);
  thread_exit();
}

int
main(int argc, char *argv[])
{
  int nw = argc > 1 ? atoi(argv[1]) : 3;
  uint64 t0, t1;

  // 1. Поток на задачу: создать, дождаться, и так N раз.
  counter = 0;
  t0 = r_time();
  for(int i = 0; i < N; i++)
    thread_join_t(thread_create(tiny_thread, 0));
  t1 = r_time();
  uint64 per_thread = (t1 - t0) / TICKS_PER_US / N;
  printf("thread per task : %d tasks, %lu us/task (counter=%d)\n", N, per_thread, counter);

  // 2. Поток на задачу, но пачками по 8 одновременно.
  counter = 0;
  t0 = r_time();
  for(int i = 0; i < N; i += 8){
    int tids[8];
    for(int k = 0; k < 8; k++)
      tids[k] = thread_create(tiny_thread, 0);
    for(int k = 0; k < 8; k++)
      thread_join_t(tids[k]);
  }
  t1 = r_time();
  uint64 per_batch = (t1 - t0) / TICKS_PER_US / N;
  printf("thread per task, 8 at once: %lu us/task (counter=%d)\n", per_batch, counter);

  // 3. Пул: потоки созданы один раз, задачи идут через очередь.
  struct pool p;
  counter = 0;
  wg_init(&wg);
  t0 = r_time();
  pool_init(&p, nw);
  wg_add(&wg, N);
  for(int i = 0; i < N; i++)
    pool_submit(&p, tiny, 0);
  wg_wait(&wg);
  pool_shutdown(&p);
  t1 = r_time();
  uint64 per_pool = (t1 - t0) / TICKS_PER_US / N;
  printf("pool of %d      : %lu us/task (counter=%d), incl. pool start/stop\n",
         nw, per_pool, counter);
  if(per_pool > 0)
    printf("thread per task / pool = %lux\n", per_thread / per_pool);
  exit(0);
}
