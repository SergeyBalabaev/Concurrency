#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/usem.h"

// sempool — счётная природа семафора: пул из K ресурсов, NTHR потоков.
// Инвариант: внутри одновременно не больше K потоков.

#define K       4
#define NTHR    12       // MAX_THREADS=16: main + 12 рабочих с запасом
#define ROUNDS  25

struct usem pool;
volatile int inside;     // сколько потоков сейчас «в ресурсе»
volatile int inside_max; // максимум за всё время
volatile int violated;   // флаг нарушения инварианта

void
update_max(volatile int *max, int v)
{
  int m = *max;
  while (v > m) {
    if (__atomic_compare_exchange_n(max, &m, v, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
      return;
    // не вышло: в m уже лежит свежее значение *max
  }
}

void
worker(void *arg)
{
  for (int r = 0; r < ROUNDS; r++) {
    sem_wait(&pool);

    int now = __atomic_add_fetch(&inside, 1, __ATOMIC_SEQ_CST);
    update_max(&inside_max, now);
    if (now > K)
      violated = 1;

    pause(1);             // гарантированно отдаём CPU, удерживая слот

    __atomic_fetch_sub(&inside, 1, __ATOMIC_SEQ_CST);
    sem_post(&pool);
  }
  thread_exit();
}

int
main(void)
{
  int t[NTHR];

  sem_init(&pool, K);

  for (int i = 0; i < NTHR; i++)
    t[i] = thread_create(worker, 0);
  for (int i = 0; i < NTHR; i++)
    thread_join_t(t[i]);

  int ok = (!violated && inside_max <= K);
  printf("sempool: K=%d max_inside=%d %s\n", K, inside_max, ok ? "OK" : "FAIL");
  exit(ok ? 0 : 1);
}