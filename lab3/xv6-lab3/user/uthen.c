#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/ufutex.h"
#include "user/uthen.h"

void
ap_init(struct apromise *p)
{
  p->state = 0;
  p->value = 0;
  p->cb = 0;
  p->cb_arg = 0;
}

// Рандеву. Каждая сторона сначала пишет своё (value или cb), потом
// атомарно поднимает свой бит и смотрит, был ли уже бит другой стороны.
// acq_rel: release публикует наше поле, acquire забирает чужое.
void
ap_set(struct apromise *p, int v)
{
  /// ТУТ ВАШ КОД
}

void
ap_subscribe(struct apromise *p, ap_callback cb, void *arg)
{
  /// ТУТ ВАШ КОД
}

// ---- Get через Subscribe: слово ожидания живёт на стеке ap_get ----
struct waiter {
  int ready;
  int value;
};

void
wake_waiter(int v, void *arg)
{
  struct waiter *w = arg;
  w->value = v;
  __atomic_store_n(&w->ready, 1, __ATOMIC_RELEASE);
  futex_wake(&w->ready, 1);
}

int
ap_get(struct apromise *p)
{
  struct waiter w = { 0, 0 };
  ap_subscribe(p, wake_waiter, &w);
  while(__atomic_load_n(&w.ready, __ATOMIC_ACQUIRE) == 0)
    futex_wait(&w.ready, 0);
  // Осторожно: колбэк ещё может выполнять futex_wake(&w.ready) после
  // того, как мы увидели ready = 1 и вернулись. Это безопасно только
  // потому, что FUTEX_WAKE по адресу не читает память. Сравните с
  // лекцией: в C++-версии notify делают под мьютексом.
  return w.value;
}

// ---- Async ----
struct async_call {
  int (*fn)(void *);
  void *arg;
  struct apromise *out;
};

void
ap_async_trampoline(void *a)
{
  struct async_call *c = a;
  int r = c->fn(c->arg);
  struct apromise *out = c->out;
  free(c);
  ap_set(out, r);
}

int
ap_async(struct pool *pl, int (*fn)(void *), void *arg, struct apromise *out)
{
  struct async_call *c = malloc(sizeof(*c));
  if(c == 0)
    return -1;
  c->fn = fn;
  c->arg = arg;
  c->out = out;
  if(pool_submit(pl, ap_async_trampoline, c) < 0){
    free(c);
    return -1;
  }
  return 0;
}

// ---- Then ----
struct then_state {
  int (*fn)(int);
  struct apromise *out;
};

void
then_cb(int v, void *arg)
{
  /// ТУТ ВАШ КОД
}

int
ap_then(struct apromise *in, int (*fn)(int), struct apromise *out)
{
  /// ТУТ ВАШ КОД
  return -1;
}

// ---- All ----
struct all_state {
  int left;                    // сколько входов ещё не ответило
  int n;
  int *values;                 // у каждого входа своя ячейка
  struct apromise *out;
};

struct all_slot {
  struct all_state *st;
  int i;
};

void
all_cb(int v, void *arg)
{
  /// ТУТ ВАШ КОД
}

int
ap_all(struct apromise **in, int n, int *values, struct apromise *out)
{
  /// ТУТ ВАШ КОД
  return -1;
}
