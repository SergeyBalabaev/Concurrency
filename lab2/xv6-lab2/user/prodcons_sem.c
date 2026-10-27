#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/umutex.h"
#include "user/usem.h"
#include "user/fstat.h"

// prodcons_sem — производитель-потребитель на двух счётных семафорах
// (Downey, §4.1). empty считает свободные ячейки, full — заполненные;
// мьютекс по-прежнему нужен: индексы делят несколько производителей
// и несколько потребителей. Завершение — «ядовитые пилюли».

#define NBUF   8
#define NPROD  2
#define NCONS  3
#define NITEMS 5000
#define POISON (-1)

int buf[NBUF];
int head, tail;                 // индексы растут монотонно, доступ по % NBUF

struct usem empty;              // свободные ячейки, старт NBUF
struct usem full;               // заполненные ячейки, старт 0
struct mutex m;                 // защищает buf/head/tail

int produced_sum;               // копится атомарно
int consumed_sum;

void
put(int v)
{
  sem_wait(&empty);
  mutex_lock(&m);
  buf[head % NBUF] = v;
  head++;
  mutex_unlock(&m);
  sem_post(&full);
}

void
producer(void *arg)
{
  int id = (int)(uint64)arg;
  int sum = 0;
  for (int i = 0; i < NITEMS; i++) {
    int v = id * NITEMS + i + 1;      // +1, чтобы не путаться с нулями
    put(v);
    sum += v;
  }
  __atomic_fetch_add(&produced_sum, sum, __ATOMIC_SEQ_CST);
  thread_exit();
}

void
consumer(void *arg)
{
  int sum = 0;
  for (;;) {
    sem_wait(&full);
    mutex_lock(&m);
    int v = buf[tail % NBUF];
    tail++;
    mutex_unlock(&m);
    sem_post(&empty);
    if (v == POISON)
      break;
    sum += v;
  }
  __atomic_fetch_add(&consumed_sum, sum, __ATOMIC_SEQ_CST);
  thread_exit();
}

int
main(void)
{
  struct futexstat st0, st1;
  int prod[NPROD], cons[NCONS];

  sem_init(&empty, NBUF);
  sem_init(&full, 0);
  mutex_init(&m);

  futexstats(&st0);

  for (int i = 0; i < NCONS; i++)
    cons[i] = thread_create(consumer, 0);
  for (int i = 0; i < NPROD; i++)
    prod[i] = thread_create(producer, (void *)(uint64)i);

  for (int i = 0; i < NPROD; i++)
    thread_join_t(prod[i]);

  // Производители закончили: проталкиваем по пилюле на потребителя
  // через тот же семафорный протокол.
  for (int i = 0; i < NCONS; i++)
    put(POISON);

  for (int i = 0; i < NCONS; i++)
    thread_join_t(cons[i]);

  futexstats(&st1);

  int ok = (produced_sum == consumed_sum);
  printf("prodcons_sem: produced=%d consumed=%d %s\n",
         produced_sum, consumed_sum, ok ? "OK" : "FAIL");
  fstat_print("prodcons_sem", &st0, &st1);
  exit(ok ? 0 : 1);
}