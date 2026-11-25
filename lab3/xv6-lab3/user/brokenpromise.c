// brokenpromise — дополнительное задание 2 (эксперимент Э6):
// почему fpromise нужно ТРИ состояния, а не два.
//
// Обе версии ниже обходятся двумя состояниями (EMPTY/READY) и обе
// неправильны. Окно гонки в них раскрыто вручную вызовом pause(),
// чтобы ошибка воспроизводилась детерминированно, а не раз в месяц.
#include "kernel/types.h"
#include "user/user.h"
#include "user/ufutex.h"
#include "user/uthread.h"

enum { B_EMPTY = 0, B_READY = 2 };

struct bpromise {
  int state;
  int value;
};

static int
bp_get(struct bpromise *p)
{
  int s;
  while((s = __atomic_load_n(&p->state, __ATOMIC_ACQUIRE)) != B_READY)
    futex_wait(&p->state, s);
  return p->value;
}

// ---- Вариант A: сначала объявить готовность, потом записать value ----
static int
bp_set_ready_first(struct bpromise *p, int v)
{
  int expected = B_EMPTY;
  if(!__atomic_compare_exchange_n(&p->state, &expected, B_READY, 0,
                                  __ATOMIC_RELEASE, __ATOMIC_RELAXED))
    return -1;
  futex_wake(&p->state, WAKE_ALL);
  pause(2);                                  // окно: READY уже виден, value ещё нет
  p->value = v;
  return 0;
}

// ---- Вариант B: сначала записать value, потом CAS в READY ----
static int
bp_set_value_first(struct bpromise *p, int v, int slow)
{
  if(slow)
    pause(3);                                // окно: этот set опоздал к value
  p->value = v;
  int expected = B_EMPTY;
  if(!__atomic_compare_exchange_n(&p->state, &expected, B_READY, 0,
                                  __ATOMIC_RELEASE, __ATOMIC_RELAXED))
    return -1;                               // проиграл, но value уже затёр!
  futex_wake(&p->state, WAKE_ALL);
  return 0;
}

static struct bpromise bp;
static int seen_a;

static void
getter_a(void *arg)
{
  seen_a = bp_get(&bp);
  thread_exit();
}

static int ret_fast, ret_slow;

static void
setter_slow(void *arg)
{
  ret_slow = bp_set_value_first(&bp, 2, 1);
  thread_exit();
}

int
main(void)
{
  int broken = 0;

  // A: ожидающий просыпается от READY и читает value до записи.
  bp.state = B_EMPTY;
  bp.value = 0;
  int t = thread_create(getter_a, 0);
  pause(1);
  bp_set_ready_first(&bp, 42);
  thread_join_t(t);
  printf("A (READY before value): getter got %d, expected 42 -> %s\n",
         seen_a, seen_a == 42 ? "ok" : "BROKEN");
  broken += seen_a != 42;

  // B: два set'а; быстрый выигрывает CAS, медленный потом затирает value.
  bp.state = B_EMPTY;
  bp.value = 0;
  t = thread_create(setter_slow, 0);
  ret_fast = bp_set_value_first(&bp, 1, 0);
  int first_read = bp_get(&bp);
  thread_join_t(t);
  int second_read = bp_get(&bp);
  printf("B (value before CAS): set(1) -> %d, set(2) -> %d; reads: %d then %d -> %s\n",
         ret_fast, ret_slow, first_read, second_read,
         first_read == second_read ? "ok" : "BROKEN");
  broken += first_read != second_read;

  printf("brokenpromise: %d of 2 two-state variants broken (expected 2)\n", broken);
  exit(broken == 2 ? 0 : 1);
}
