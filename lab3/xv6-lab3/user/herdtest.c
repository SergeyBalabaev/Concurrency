//
// herdtest - замер thundering herd: во что обходится ожидание
// завершения потоков на общем канале (&proc, thread_join из ЛР1)
// по сравнению с futex-ожиданием на индивидуальном слове ctid.
//
//   herdtest old    - N ожидающих внутри sys_thread_join
//   herdtest futex  - N ожидающих в futex_wait на своих словах
//
// Сценарий: N рабочих потоков завершаются по одному (пауза i+1
// тиков); N потоков-ожидающих, каждый ждёт своего рабочего.
// На общем канале каждый exit будит ВСЕХ ещё спящих ожидающих:
// суммарно ~ N + (N-1) + ... + 1 = N(N+1)/2 засыпаний.
// На futex каждый exit будит ровно одного: ~ N.
//
// Работаем с clone/futex напрямую, мимо uthread: в режиме old нам
// нужен "голый" thread_join, а таблица uthread без доп. задания
// не защищена от параллельных join'ов из N потоков.
//
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/futex.h"
#include "user/user.h"
#include "user/fstat.h"

#define N 8

int use_futex;

volatile int worker_alive[N];    // ctid-слова рабочих
volatile int joiner_alive[N];    // ctid-слова ожидающих
int worker_tid[N];

// ждать нуля в слове-ctid, затем убрать останки потока
void
join_via_futex(volatile int *alive, int tid)
{
  int v;
  while((v = *alive) != 0)
    futex(alive, FUTEX_WAIT, v);
  thread_join(tid);
}

void
worker(void *arg)
{
  int i = (int)(uint64)arg;
  pause(2 * (i + 1));            // завершаемся по очереди
  exit(0);
}

void
joiner(void *arg)
{
  int i = (int)(uint64)arg;
  if(use_futex)
    join_via_futex(&worker_alive[i], worker_tid[i]);
  else
    thread_join(worker_tid[i]);  // общий канал - стадо
  exit(0);
}

int
main(int argc, char *argv[])
{
  struct futexstat s0, s1;

  use_futex = (argc > 1 && argv[1][0] == 'f');
  printf("herdtest: %d workers, %d joiners, mode = %s\n",
         N, N, use_futex ? "futex" : "old (shared channel)");

  futexstats(&s0);

  for(int i = 0; i < N; i++){
    worker_alive[i] = 1;
    worker_tid[i] = clone(worker, (void*)(uint64)i,
                          malloc(PGSIZE), (int*)&worker_alive[i]);
  }
  int joiner_tid[N];
  for(int i = 0; i < N; i++){
    joiner_alive[i] = 1;
    joiner_tid[i] = clone(joiner, (void*)(uint64)i,
                          malloc(PGSIZE), (int*)&joiner_alive[i]);
  }

  // дождаться самих ожидающих - тем же способом, что и они
  for(int i = 0; i < N; i++){
    if(use_futex)
      join_via_futex(&joiner_alive[i], joiner_tid[i]);
    else
      thread_join(joiner_tid[i]);
  }

  futexstats(&s1);

  printf("  sleeps in thread_join (herd):  %d\n",
         (int)(s1.join_sleeps - s0.join_sleeps));
  fstat_print("  futex", &s0, &s1);
  // стеки не освобождаем - процесс сейчас завершится
  exit(0);
}