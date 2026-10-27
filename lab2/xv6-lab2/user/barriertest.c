#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/umutex.h"
#include "user/usem.h"
#include "user/fstat.h"

#define NTHR 8
#define ROUNDS 50

//
// Дополнительное задание: двухфазный многоразовый барьер поверх
// struct usem (раздел 8.6, Downey §3.7). Поля структуры можно менять,
// интерфейс barrier_init/barrier_wait - нет: им пользуется тест ниже.
//
struct barrier {
  struct mutex m;
  int count;
  int n;
  struct usem turnstile;
  struct usem turnstile2;
};

void
barrier_init(struct barrier *b, int n)
{
  /// ТУТ ВАШ КОД
}

void
barrier_wait(struct barrier *b)
{
  /// ТУТ ВАШ КОД
}

//
// Тест: N потоков, R раундов. В раунде каждый увеличивает свой счётчик,
// барьер, поток 0 проверяет, что ВСЕ счётчики равны номеру раунда,
// второй барьер — чтобы никто не начал следующий раунд до конца проверки.
//
struct barrier bar;
volatile int counter[NTHR];
volatile int failed;

void
worker(void *arg)
{
  int id = (int)(uint64)arg;

  for (int r = 1; r <= ROUNDS; r++) {
    counter[id]++;

    barrier_wait(&bar);

    if (id == 0) {
      for (int i = 0; i < NTHR; i++)
        if (counter[i] != r) {
          printf("round %d: counter[%d]=%d, expected %d\n",
                 r, i, counter[i], r);
          failed = 1;
        }
    }

    barrier_wait(&bar);
  }
  thread_exit();
}

int
main(void)
{
  struct futexstat a, b;
  int t[NTHR];

  barrier_init(&bar, NTHR);
  futexstats(&a);

  for (int i = 0; i < NTHR; i++)
    t[i] = thread_create(worker, (void *)(uint64)i);
  for (int i = 0; i < NTHR; i++)
    thread_join_t(t[i]);

  futexstats(&b);

  printf("barriertest: %d threads, %d rounds, %s\n",
         NTHR, ROUNDS, failed ? "FAIL" : "OK");
  fstat_print("barriertest", &a, &b);
  exit(failed ? 1 : 0);
}