// promisetest — promise/future (БДЗ-3, шаг 5).
// Проверяет основной интерфейс promise_*: на базовом уровне это cpromise,
// с PROMISE_ADVANCED - fpromise.
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/upromise.h"
#include "user/uasync.h"
#include "user/uthread.h"

static int failed;

#define CHECK(cond, msg) do { \
    if(cond) printf("%s: OK\n", msg); \
    else { printf("%s: FAIL\n", msg); failed = 1; } } while(0)

static struct promise pr;

// ---- 2. ожидающие до set ----
static int got[8];

static void
getter(void *arg)
{
  int i = (int)(uint64)arg;
  got[i] = promise_get(&pr);
  thread_exit();
}

// ---- 3. гонка двух set ----
static int set_ret[2];

static void
setter(void *arg)
{
  int i = (int)(uint64)arg;
  set_ret[i] = promise_set(&pr, 100 + i);
  thread_exit();
}

// ---- 4. pool_async ----
static int
square(void *arg)
{
  int x = (int)(uint64)arg;
  return x * x;
}

int
main(void)
{
#ifdef PROMISE_ADVANCED
  printf("promisetest: level = advanced (fpromise)\n");
#else
  printf("promisetest: level = basic (cpromise)\n");
#endif

  // 1. Однопоточная семантика.
  int v = -7;
  promise_init(&pr);
  CHECK(promise_tryget(&pr, &v) == FUTURE_AGAIN && v == -7, "tryget on empty -> FUTURE_AGAIN");
  CHECK(promise_set(&pr, 42) == 0, "first set -> 0");
  CHECK(promise_set(&pr, 13) == -1, "second set -> -1");
  CHECK(promise_tryget(&pr, &v) == 0 && v == 42, "tryget after set -> 42");
  CHECK(promise_get(&pr) == 42 && promise_get(&pr) == 42, "get is repeatable");

  // 2. Шесть потоков ждут до set - все получают значение.
  int tids[6];
  promise_init(&pr);
  for(int i = 0; i < 6; i++)
    tids[i] = thread_create(getter, (void *)(uint64)i);
  pause(2);
  promise_set(&pr, 77);
  for(int i = 0; i < 6; i++)
    thread_join_t(tids[i]);
  int all = 1;
  for(int i = 0; i < 6; i++)
    all &= (got[i] == 77);
  CHECK(all, "6 early getters all got 77");

  // 3. Гонка двух set: ровно один выигрывает, читатели видят его значение.
  int bad = 0;
  for(int r = 0; r < 200; r++){
    int t[4];
    promise_init(&pr);
    t[0] = thread_create(setter, (void *)0);
    t[1] = thread_create(setter, (void *)1);
    t[2] = thread_create(getter, (void *)0);
    t[3] = thread_create(getter, (void *)1);
    for(int i = 0; i < 4; i++)
      thread_join_t(t[i]);
    int wins = (set_ret[0] == 0) + (set_ret[1] == 0);
    int winner = set_ret[0] == 0 ? 100 : 101;
    if(wins != 1 || got[0] != winner || got[1] != winner)
      bad++;
  }
  printf("200 rounds of 2 setters + 2 getters: bad rounds = %d\n", bad);
  CHECK(bad == 0, "exactly one set wins, getters see its value");

  // 4. pool_async: результаты задач пула через promise.
  struct pool p;
  struct promise sq[16];
  pool_init(&p, 3);
  for(int i = 0; i < 16; i++){
    promise_init(&sq[i]);
    pool_async(&p, square, (void *)(uint64)i, &sq[i]);
  }
  int sum = 0;
  for(int i = 0; i < 16; i++)
    sum += promise_get(&sq[i]);         // get - в main, не в задаче пула!
  pool_shutdown(&p);
  printf("sum of squares 0..15 = %d\n", sum);
  CHECK(sum == 1240, "pool_async results");

  printf("promisetest: %s\n", failed ? "FAILED" : "all tests passed");
  exit(failed);
}
