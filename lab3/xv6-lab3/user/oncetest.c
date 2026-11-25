// oncetest — call_once (БДЗ-3, дополнительное задание 1).
// Восемь потоков одновременно просят "ленивую" таблицу; строит её
// ровно один, остальные ждут и видят таблицу целиком.
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/futex.h"
#include "user/uonce.h"
#include "user/uthread.h"
#include "user/fstat.h"

#define NT 8
#define TBL 1024

static struct once init_once = ONCE_INIT;
static int table[TBL];
static int init_calls;
static int bad_views;
static volatile int go;

static void
build_table(void)
{
  __atomic_fetch_add(&init_calls, 1, __ATOMIC_RELAXED);
  for(int i = 0; i < TBL; i++){
    table[i] = i * i;
    if(i == TBL / 2)
      pause(2);                  // долгая инициализация: остальные успеют прийти
  }
}

static void
user_of_table(void *arg)
{
  while(!go)
    ;
  call_once(&init_once, build_table);
  for(int i = 0; i < TBL; i++)
    if(table[i] != i * i){
      __atomic_fetch_add(&bad_views, 1, __ATOMIC_RELAXED);
      break;
    }
  thread_exit();
}

int
main(void)
{
  int tids[NT];
  struct futexstat a, b;

  for(int i = 0; i < NT; i++)
    tids[i] = thread_create(user_of_table, 0);
  futexstats(&a);
  go = 1;
  for(int i = 0; i < NT; i++)
    thread_join_t(tids[i]);
  futexstats(&b);

  printf("oncetest: %d threads, init_calls=%d, incomplete views=%d\n",
         NT, init_calls, bad_views);
  fstat_print("once", &a, &b);

  // Повторный вызов после завершения - быстрый путь, без ядра.
  futexstats(&a);
  for(int i = 0; i < 1000; i++)
    call_once(&init_once, build_table);
  futexstats(&b);
  fstat_print("once x1000 after done", &a, &b);

  int ok = init_calls == 1 && bad_views == 0;
  printf("oncetest: %s\n", ok ? "OK" : "FAIL");
  exit(ok ? 0 : 1);
}
