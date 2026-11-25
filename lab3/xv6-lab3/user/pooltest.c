// pooltest — пул потоков (БДЗ-3, шаг 3).
#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/timing.h"

static int failed;

#define CHECK(cond, msg) do { \
    if(cond) printf("%s: OK\n", msg); \
    else { printf("%s: FAIL\n", msg); failed = 1; } } while(0)

static int counter;

static void
inc(void *arg)
{
  __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);
}

// Задача "потяжелее": 20 мс крутится на процессоре и запоминает, кто
// её выполнил. Длинная - нарочно: простаивающее ядро xv6 спит в wfi до
// таймерного прерывания (до 100 мс), и короткие задачи успел бы
// разобрать один рабочий, пока остальные ждут, когда их ядро проснётся.
#define NSLOW 24
static int who[NSLOW];

static void
slow(void *arg)
{
  int i = (int)(uint64)arg;
  uint64 t0 = r_time();
  while(r_time() - t0 < 20 * TICKS_PER_MS)
    ;
  who[i] = gettid();
}

int
main(void)
{
  struct pool p;

  // 1. Все задачи выполнены (ожидание - через shutdown).
  pool_init(&p, 3);
  for(int i = 0; i < 1000; i++)
    pool_submit(&p, inc, 0);
  pool_shutdown(&p);                   // дожидается и очереди, и рабочих
  printf("1000 tasks: counter = %d\n", counter);
  CHECK(counter == 1000, "all submitted tasks executed");

  // 2. После shutdown задачи не принимаются.
  CHECK(pool_submit(&p, inc, 0) == -1, "submit after shutdown -> -1");

  // 3. Задачи действительно расходятся по разным рабочим.
  pool_init(&p, 3);
  for(int i = 0; i < NSLOW; i++)
    pool_submit(&p, slow, (void *)(uint64)i);
  pool_shutdown(&p);
  int distinct = 0;
  for(int i = 0; i < NSLOW; i++){
    int fresh = 1;
    for(int j = 0; j < i; j++)
      if(who[j] == who[i])
        fresh = 0;
    distinct += fresh;
  }
  printf("%d slow tasks ran on %d distinct workers (pool of 3):", NSLOW, distinct);
  for(int i = 0; i < NSLOW; i++)
    printf(" %d", who[i]);
  printf("\n");
  CHECK(distinct >= 2 && distinct <= 3, "work is spread over workers");

  // 4. Пул можно поднимать и останавливать много раз (нет утечки потоков).
  for(int r = 0; r < 20; r++){
    pool_init(&p, 4);
    pool_submit(&p, inc, 0);
    pool_shutdown(&p);
  }
  CHECK(counter == 1020, "20 init/shutdown cycles");

  printf("pooltest: %s\n", failed ? "FAILED" : "all tests passed");
  exit(failed);
}
