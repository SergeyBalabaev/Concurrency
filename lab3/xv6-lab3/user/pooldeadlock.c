// pooldeadlock — эксперимент Э3: задача пула, ждущая другую задачу пула.
//
//   pooldeadlock NPARENTS [NWORKERS]
//
// Каждая задача-родитель отправляет в тот же пул задачу-ребёнка и ждёт
// её результата через promise_get - то есть блокирует рабочий поток.
// Пока родителей меньше, чем рабочих, всё работает. Когда родители
// заняли всех рабочих, детям негде выполниться: дедлок.
//
// Чтобы не перезагружать xv6, main работает сторожем: если за 2 секунды
// родители не закончили, он объявляет дедлок и сам выставляет детские
// promise, освобождая рабочих.
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/upromise.h"

#define MAXP 8

static struct pool p;
static struct promise child_res[MAXP];
static int parents_done;
static int children_ran;

static void
child(void *arg)
{
  int i = (int)(uint64)arg;
  __atomic_fetch_add(&children_ran, 1, __ATOMIC_SEQ_CST);
  promise_set(&child_res[i], i * 10);   // после спасения вернёт -1 - не страшно
}

static void
parent(void *arg)
{
  int i = (int)(uint64)arg;
  pool_submit(&p, child, arg);
  promise_get(&child_res[i]);           // ПЛОХО: блокируем рабочий поток
  __atomic_fetch_add(&parents_done, 1, __ATOMIC_SEQ_CST);
}

int
main(int argc, char *argv[])
{
  int np = argc > 1 ? atoi(argv[1]) : 2;
  int nw = argc > 2 ? atoi(argv[2]) : 2;
  if(np < 1 || np > MAXP || nw < 1 || nw > POOL_MAX){
    printf("usage: pooldeadlock NPARENTS(1..%d) [NWORKERS]\n", MAXP);
    exit(1);
  }

  printf("pool of %d workers, %d parent tasks, each waits for its child\n", nw, np);
  pool_init(&p, nw);
  for(int i = 0; i < np; i++)
    promise_init(&child_res[i]);
  for(int i = 0; i < np; i++)
    pool_submit(&p, parent, (void *)(uint64)i);

  int waited = 0;
  while(__atomic_load_n(&parents_done, __ATOMIC_SEQ_CST) < np && waited < 20){
    pause(1);
    waited++;
  }
  int done = __atomic_load_n(&parents_done, __ATOMIC_SEQ_CST);
  if(done == np){
    printf("OK: all parents finished, children ran: %d\n", children_ran);
  } else {
    printf("DEADLOCK: after 2 s parents done %d of %d, children ran %d\n",
           done, np, __atomic_load_n(&children_ran, __ATOMIC_SEQ_CST));
    printf("  every worker is blocked in promise_get, children wait in the queue\n");
    printf("  rescuing: main sets the children's promises\n");
    for(int i = 0; i < np; i++)
      promise_set(&child_res[i], -1);
  }
  pool_shutdown(&p);
  printf("pooldeadlock: finished\n");
  exit(done == np ? 0 : 1);
}
