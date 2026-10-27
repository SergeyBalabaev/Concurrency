#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/usem.h"
#include "user/fstat.h"

// naivepost — зачем fsem нужен счётчик nwaiters. Одна и та же нагрузка
// на fsem_post (будит, только если есть ожидающие) и на fsem_post_naive
// (FUTEX_WAKE на каждом post). Сравниваем wake_calls.
// Только продвинутый уровень (SEM_ADVANCED).

#ifdef SEM_ADVANCED

#define M     5000
#define NTHR  8
#define CROUNDS 500

struct fsem s;
int use_naive;

void
post(struct fsem *sm)
{
  if (use_naive)
    fsem_post_naive(sm);
  else
    fsem_post(sm);
}

// Соло: один поток, начальное значение 1, конкуренции нет —
// каждый wait проходит быстрым путём, ни один post никого не будит.
void
solo(char *tag)
{
  struct futexstat a, b;

  fsem_init(&s, 1);
  futexstats(&a);
  for (int i = 0; i < M; i++) {
    fsem_wait(&s);
    post(&s);
  }
  futexstats(&b);
  printf("solo      M=%d     ", M);
  fstat_print(tag, &a, &b);
}

// Конкуренция: пул на 2 слота, 8 потоков — здесь у обеих версий
// wake_calls > 0, но у наивной он равен числу post, у правильной — меньше.
void
cworker(void *arg)
{
  for (int r = 0; r < CROUNDS; r++) {
    fsem_wait(&s);
    for (volatile int d = 0; d < 100; d++)
      ;
    post(&s);
  }
  thread_exit();
}

void
contended(char *tag)
{
  struct futexstat a, b;
  int t[NTHR];

  fsem_init(&s, 2);
  futexstats(&a);
  for (int i = 0; i < NTHR; i++)
    t[i] = thread_create(cworker, 0);
  for (int i = 0; i < NTHR; i++)
    thread_join_t(t[i]);
  futexstats(&b);
  printf("contended posts=%d  ", NTHR * CROUNDS);
  fstat_print(tag, &a, &b);
}

int
main(void)
{
  use_naive = 0; solo("nwaiters");
  use_naive = 1; solo("naive   ");
  use_naive = 0; contended("nwaiters");
  use_naive = 1; contended("naive   ");
  exit(0);
}

#else

int
main(void)
{
  printf("naivepost: skipped (needs SEM_ADVANCED)\n");
  exit(0);
}

#endif
