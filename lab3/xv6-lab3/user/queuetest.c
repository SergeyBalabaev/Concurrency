// queuetest — блокирующая очередь (БДЗ-3, шаг 2).
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/uthread.h"
#include "user/fstat.h"

static int failed;

#define CHECK(cond, msg) do { \
    if(cond) printf("%s: OK\n", msg); \
    else { printf("%s: FAIL\n", msg); failed = 1; } } while(0)

static struct task
mk(int v)
{
  struct task t = { 0, (void *)(uint64)v };
  return t;
}

static int
val(struct task t)
{
  return (int)(uint64)t.arg;
}

// ---- 1. FIFO и close в одном потоке ----
static void
test_fifo_close(void)
{
  struct bqueue q;
  struct task t;
  int ok = 1;

  bq_init(&q, 8);
  for(int i = 1; i <= 5; i++)
    bq_put(&q, mk(i));
  for(int i = 1; i <= 3; i++)
    if(bq_take(&q, &t) != 0 || val(t) != i)
      ok = 0;
  CHECK(ok, "fifo order");

  bq_close(&q);
  CHECK(bq_put(&q, mk(99)) == -1, "put after close -> -1");
  ok = bq_take(&q, &t) == 0 && val(t) == 4 &&
       bq_take(&q, &t) == 0 && val(t) == 5;
  CHECK(ok, "take after close drains the rest");
  CHECK(bq_take(&q, &t) == -1, "take on closed empty -> -1");
}

// ---- 2. close будит спящих потребителей ----
static struct bqueue sq;
static int sleepers_out;

static void
sleeper(void *arg)
{
  struct task t;
  if(bq_take(&sq, &t) == -1)
    __atomic_fetch_add(&sleepers_out, 1, __ATOMIC_SEQ_CST);
  thread_exit();
}

static void
test_close_wakes(void)
{
  int tids[3];
  bq_init(&sq, 4);
  for(int i = 0; i < 3; i++)
    tids[i] = thread_create(sleeper, 0);
  pause(2);                            // дать всем уснуть в take
  bq_close(&sq);
  for(int i = 0; i < 3; i++)
    thread_join_t(tids[i]);
  CHECK(sleepers_out == 3, "close wakes all sleeping consumers");
}

// ---- 3. несколько производителей и потребителей ----
#define NPROD 3
#define NCONS 3
#define PER   2000
#define TOTAL (NPROD * PER)

static struct bqueue mq;
static char seen[TOTAL];
static int dups, taken;

static void
producer(void *arg)
{
  int id = (int)(uint64)arg;
  for(int i = 0; i < PER; i++)
    bq_put(&mq, mk(id * PER + i));
  thread_exit();
}

static void
consumer(void *arg)
{
  struct task t;
  while(bq_take(&mq, &t) == 0){
    int v = val(t);
    if(__atomic_exchange_n(&seen[v], 1, __ATOMIC_RELAXED))
      __atomic_fetch_add(&dups, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&taken, 1, __ATOMIC_RELAXED);
  }
  thread_exit();
}

static void
test_mpmc(void)
{
  int pt[NPROD], ct[NCONS];
  struct futexstat a, b;

  bq_init(&mq, 8);
  futexstats(&a);
  for(int i = 0; i < NCONS; i++)
    ct[i] = thread_create(consumer, 0);
  for(int i = 0; i < NPROD; i++)
    pt[i] = thread_create(producer, (void *)(uint64)i);
  for(int i = 0; i < NPROD; i++)
    thread_join_t(pt[i]);
  bq_close(&mq);                       // производители закончили
  for(int i = 0; i < NCONS; i++)
    thread_join_t(ct[i]);
  futexstats(&b);

  int missing = 0;
  for(int i = 0; i < TOTAL; i++)
    if(!seen[i])
      missing++;
  printf("mpmc: %d prod x %d, %d cons, cap 8: taken=%d missing=%d dups=%d\n",
         NPROD, PER, NCONS, taken, missing, dups);
  CHECK(taken == TOTAL && missing == 0 && dups == 0, "mpmc: every item exactly once");
  fstat_print("mpmc", &a, &b);
}

int
main(void)
{
  test_fifo_close();
  test_close_wakes();
  test_mpmc();
  printf("queuetest: %s\n", failed ? "FAILED" : "all tests passed");
  exit(failed);
}
