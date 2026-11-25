// promisestat — эксперимент Э5: сколько стоит ожидание promise.
// Сравнивает cpromise и (на продвинутом уровне) fpromise по счётчикам
// futexstats и по времени get на уже готовом значении.
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/upromise.h"
#include "user/uthread.h"
#include "user/timing.h"
#include "user/fstat.h"

#define NW   6        // ожидающих потоков
#define NGET 1000     // get на поток в сценарии "поздние"

static struct cpromise cp;
#ifdef PROMISE_ADVANCED
static struct fpromise fp;
#endif
static int use_f;     // 0 - cpromise, 1 - fpromise

static int
do_get(void)
{
#ifdef PROMISE_ADVANCED
  if(use_f)
    return fpromise_get(&fp);
#endif
  return cpromise_get(&cp);
}

static void
do_set(int v)
{
#ifdef PROMISE_ADVANCED
  if(use_f){
    fpromise_set(&fp, v);
    return;
  }
#endif
  cpromise_set(&cp, v);
}

static void
do_init(void)
{
  cpromise_init(&cp);
#ifdef PROMISE_ADVANCED
  fpromise_init(&fp);
#endif
}

// Потоки создаются ДО первого снимка счётчиков и ждут сигнала go
// активным ожиданием и выходят только после второго снимка: создание,
// выход и join потоков сами ходят в futex (malloc стека, ctid), и эти
// вызовы не должны попасть в замер.
static volatile int go, finished;

static void
early(void *arg)
{
  while(!go)
    ;
  do_get();
  __atomic_fetch_add(&finished, 1, __ATOMIC_SEQ_CST);
  while(go != 2)                     // выход потока тоже ходит в futex (ctid) -
    ;                                // дождёмся второго снимка
  thread_exit();
}

static void
late(void *arg)
{
  while(!go)
    ;
  for(int i = 0; i < NGET; i++)
    do_get();
  __atomic_fetch_add(&finished, 1, __ATOMIC_SEQ_CST);
  while(go != 2)                     // выход потока тоже ходит в futex (ctid) -
    ;                                // дождёмся второго снимка
  thread_exit();
}

static void
wait_finished(void)
{
  while(__atomic_load_n(&finished, __ATOMIC_SEQ_CST) < NW)
    pause(1);
}

static void
run(char *name)
{
  int tids[NW];
  struct futexstat a, b;
  char tag[32];

  // Ранние потребители: все пришли до set и уснули.
  do_init();
  go = finished = 0;
  for(int i = 0; i < NW; i++)
    tids[i] = thread_create(early, 0);
  futexstats(&a);
  go = 1;
  pause(3);                          // все успели уснуть в get
  do_set(1);
  wait_finished();
  futexstats(&b);
  go = 2;
  for(int i = 0; i < NW; i++)
    thread_join_t(tids[i]);
  strcpy(tag, name); strcpy(tag + strlen(tag), " early");
  fstat_print(tag, &a, &b);

  // Поздние потребители: значение уже готово, NW потоков по NGET get.
  go = finished = 0;
  for(int i = 0; i < NW; i++)
    tids[i] = thread_create(late, 0);
  futexstats(&a);
  go = 1;
  wait_finished();
  futexstats(&b);
  go = 2;
  for(int i = 0; i < NW; i++)
    thread_join_t(tids[i]);
  strcpy(tag, name); strcpy(tag + strlen(tag), " late ");
  fstat_print(tag, &a, &b);

  // Цена одного get на готовом значении, один поток.
  uint64 t0 = r_time();
  for(int i = 0; i < 100000; i++)
    do_get();
  uint64 t1 = r_time();
  printf("%s ready get: %lu ns per call\n", name, (t1 - t0) * 100 / 100000);
}

int
main(void)
{
  printf("promisestat: %d waiters; late: %d x %d gets\n", NW, NW, NGET);
  use_f = 0;
  run("cpromise");
#ifdef PROMISE_ADVANCED
  use_f = 1;
  run("fpromise");
#else
  printf("(fpromise: enable PROMISE_ADVANCED in upromise.h)\n");
#endif
  exit(0);
}
