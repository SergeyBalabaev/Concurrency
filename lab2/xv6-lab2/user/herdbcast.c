// herdbcast — стадный эффект cond_broadcast.
//
// NWAIT потоков засыпают на условной переменной; main дожидается,
// пока все реально уснут (контролируем по futexstats, а не по
// таймеру), делает срез статистики и выполняет один broadcast,
// продолжая держать мьютекс.
//
// Ожидаемая картина для обычной cond:
//   woken ~ NWAIT + (NWAIT-1): NWAIT пробуждений от самого broadcast
//     плюс цепочка точечных пробуждений на слове мьютекса;
//   slept ~ NWAIT-1: разбуженные стадом тут же засыпают снова, теперь
//     на мьютексе. Мы разбудили всех, чтобы почти все немедленно уснули.
//
// С MUTEX_ADVANCED тот же сценарий прогоняется на condrq (broadcast
// через futex_requeue, доп. задание): от broadcast просыпается один,
// остальные перевешиваются в очередь мьютекса (requeued = NWAIT-1)
// и просыпаются по одному, без повторных засыпаний.
//
// Точные числа недетерминированы, но порядок величин воспроизводится.

#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/umutex.h"
#include "user/fstat.h"

#define NWAIT 8

static struct mutex m;
static struct cond cv;
#ifdef MUTEX_ADVANCED
static struct condrq cvrq;
#endif
static int use_rq;
static int go;
static volatile int passed;     // сколько ожидающих прошли мимо cond

static void
waiter(void *arg)
{
  mutex_lock(&m);
  while (!go) {
#ifdef MUTEX_ADVANCED
    if (use_rq)
      condrq_wait(&cvrq, &m);
    else
#endif
      cond_wait(&cv, &m);
  }
  mutex_unlock(&m);
  __atomic_fetch_add(&passed, 1, __ATOMIC_SEQ_CST);
  thread_exit();
}

static void
run(char *tag)
{
  int tid[NWAIT];
  struct futexstat a, b;

  mutex_init(&m);
  cond_init(&cv);
#ifdef MUTEX_ADVANCED
  condrq_init(&cvrq);
#endif
  go = 0;
  passed = 0;

  futexstats(&a);
  for (int i = 0; i < NWAIT; i++)
    tid[i] = thread_create(waiter, 0);

  // Ждём, пока все NWAIT реально уснут на условной переменной:
  // опрашиваем прирост wait_slept. Таймер здесь был бы гонкой.
  for (;;) {
    futexstats(&b);
    if (b.wait_slept - a.wait_slept >= NWAIT)
      break;
    pause(1);
  }

  futexstats(&a);          // точка отсчёта: все спят
  mutex_lock(&m);
  go = 1;
#ifdef MUTEX_ADVANCED
  if (use_rq)
    condrq_broadcast(&cvrq);
  else
#endif
    cond_broadcast(&cv);
  pause(2);                // держим мьютекс: пусть стадо упрётся в него
  mutex_unlock(&m);

  // Срез - когда все прошли, но ДО join: ожидание в join тоже ходит
  // через futex и смешалось бы со стадом.
  while (passed < NWAIT)
    pause(1);
  futexstats(&b);
  fstat_print(tag, &a, &b);

  for (int i = 0; i < NWAIT; i++)
    thread_join_t(tid[i]);
}

int
main(void)
{
  printf("herdbcast: %d waiters, one broadcast\n", NWAIT);
  use_rq = 0;
  run("cond  ");
#ifdef MUTEX_ADVANCED
  use_rq = 1;
  run("condrq");
#endif
  exit(0);
}
