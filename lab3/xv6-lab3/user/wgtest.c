// wgtest — WaitGroup (БДЗ-3, шаг 4).
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/uwaitgroup.h"
#include "user/uthread.h"
#include "user/fstat.h"

static int failed;

#define CHECK(cond, msg) do { \
    if(cond) printf("%s: OK\n", msg); \
    else { printf("%s: FAIL\n", msg); failed = 1; } } while(0)

static struct waitgroup wg;
static int done_flag[16];

static void
job(void *arg)
{
  int i = (int)(uint64)arg;
  volatile int x = 0;
  for(int k = 0; k < 100000 * (i % 4 + 1); k++)
    x += k;
  done_flag[i] = 1;                    // записано до done...
  wg_done(&wg);                        // ...значит, видно после wait
}

static int released;

static void
waiter(void *arg)
{
  wg_wait(&wg);
  __atomic_fetch_add(&released, 1, __ATOMIC_SEQ_CST);
  thread_exit();
}

int
main(void)
{
  struct pool p;
  struct futexstat a, b;

  wg_init(&wg);
  wg_wait(&wg);
  printf("wait on zero counter returns at once: OK\n");

  pool_init(&p, 3);

  // 1. Все задачи завершены к возврату из wait.
  wg_add(&wg, 16);                     // ДО submit
  for(int i = 0; i < 16; i++)
    pool_submit(&p, job, (void *)(uint64)i);
  wg_wait(&wg);
  int all = 1;
  for(int i = 0; i < 16; i++)
    all &= done_flag[i];
  CHECK(all, "16 tasks: all done after wait");

  // 2. Несколько ожидающих отпускаются одним событием.
  int tids[3];
  wg_add(&wg, 8);
  for(int i = 0; i < 3; i++)
    tids[i] = thread_create(waiter, 0);
  futexstats(&a);
  for(int i = 0; i < 8; i++)
    pool_submit(&p, job, (void *)(uint64)i);
  for(int i = 0; i < 3; i++)
    thread_join_t(tids[i]);
  wg_wait(&wg);
  futexstats(&b);
  CHECK(released == 3, "3 extra waiters released");
  fstat_print("waiters", &a, &b);

  // 3. WaitGroup многоразовый: 50 раундов по 4 задачи.
  int ok = 1;
  for(int r = 0; r < 50; r++){
    for(int i = 0; i < 4; i++)
      done_flag[i] = 0;
    wg_add(&wg, 4);
    for(int i = 0; i < 4; i++)
      pool_submit(&p, job, (void *)(uint64)i);
    wg_wait(&wg);
    for(int i = 0; i < 4; i++)
      ok &= done_flag[i];
  }
  CHECK(ok, "50 rounds of reuse");

  pool_shutdown(&p);
  printf("wgtest: %s\n", failed ? "FAILED" : "all tests passed");
  exit(failed);
}
