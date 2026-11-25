// thentest — Subscribe, Then, All (БДЗ-3, дополнительное задание 3).
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/uthen.h"
#include "user/timing.h"

static int failed;

#define CHECK(cond, msg) do { \
    if(cond) printf("%s: OK\n", msg); \
    else { printf("%s: FAIL\n", msg); failed = 1; } } while(0)

static int answer(void *arg)  { return 42; }
static int twice(int x)       { return x * 2; }
static int plus_one(int x)    { return x + 1; }

static int
slow_answer(void *arg)
{
  uint64 t0 = r_time();
  while(r_time() - t0 < 300 * TICKS_PER_MS)   // 0.3 с
    ;
  return 42;
}

static int cb_tid;

static void
remember_tid(int v, void *arg)
{
  cb_tid = gettid();
}

static int
square(void *arg)
{
  int x = (int)(uint64)arg;
  return x * x;
}

int
main(void)
{
  struct pool p;
  pool_init(&p, 3);
  int main_tid = gettid();

  // 1. Цепочка из лекции: 42 -> *2 -> +1 = 85. Код только планирует работу.
  struct apromise a, b, c;
  ap_init(&a); ap_init(&b); ap_init(&c);
  ap_async(&p, answer, 0, &a);
  ap_then(&a, twice, &b);
  ap_then(&b, plus_one, &c);
  int v = ap_get(&c);
  printf("async(42).then(*2).then(+1) = %d\n", v);
  CHECK(v == 85, "then chain");

  // 2. Где выполняется колбэк: подписались раньше - в рабочем потоке.
  ap_init(&a);
  ap_async(&p, slow_answer, 0, &a);
  ap_subscribe(&a, remember_tid, 0);           // значения ещё нет
  pause(5);
  printf("subscribe before set: callback ran in tid %d (main is %d)\n", cb_tid, main_tid);
  CHECK(cb_tid != main_tid, "callback in producer's thread");

  //    Подписались позже - прямо в main, внутри subscribe.
  ap_init(&a);
  ap_async(&p, answer, 0, &a);
  pause(3);                                    // значение уже готово
  ap_subscribe(&a, remember_tid, 0);
  printf("subscribe after set:  callback ran in tid %d (main is %d)\n", cb_tid, main_tid);
  CHECK(cb_tid == main_tid, "callback in subscriber's thread");

  // 3. All из 8 асинхронных задач.
  struct apromise in[8], all;
  struct apromise *ptrs[8];
  int vals[8];
  ap_init(&all);
  for(int i = 0; i < 8; i++){
    ap_init(&in[i]);
    ptrs[i] = &in[i];
    ap_async(&p, square, (void *)(uint64)(i + 1), &in[i]);
  }
  ap_all(ptrs, 8, vals, &all);
  int n = ap_get(&all);
  int sum = 0;
  for(int i = 0; i < n; i++)
    sum += vals[i];
  printf("all of 8: n=%d, sum of squares 1..8 = %d\n", n, sum);
  CHECK(n == 8 && sum == 204, "all");

  pool_shutdown(&p);
  printf("thentest: %s\n", failed ? "FAILED" : "all tests passed");
  exit(failed);
}
