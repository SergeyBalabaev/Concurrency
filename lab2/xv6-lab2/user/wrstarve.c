// wrstarve.c — голодание писателя и побудки: обе метрики за один прогон.
//
// Нагрузка: NREADERS читателей крутят цикл {rdlock; pause(1); rdunlock} —
// pause(1) ОБЯЗАТЕЛЕН, без него читатели не перекрываются и nreaders
// касается нуля. Писатель делает PROBES заходов: перед каждым снимает
// rd_acquires, после входа берёт разность — это и есть метрика
// голодания (сколько читателей прошло, пока писатель ждал).
//
// Ожидания. rwlock1: почти все читательские захваты придутся на время
// ожидания писателя в первой пробе (сотни). rwlock2: разность на пробу
// не превышает NREADERS (только успевшие до заявки писателя).

#include "kernel/types.h"
#include "user/user.h"
#include "umutex.h"
#include "urwlock.h"
#include "uthread.h"
#include "fstat.h"

#define NREADERS 8
#define RROUNDS  60
#define PROBES   4

struct rwops {
  void *rw;
  void (*rdlock)(void*);
  void (*rdunlock)(void*);
  void (*wrlock)(void*);
  void (*wrunlock)(void*);
  unsigned *rd_acquires;   // указатель на счётчик внутри замка
};

static void r1_rd(void *p)   { rwlock1_rdlock(p); }
static void r1_rdu(void *p)  { rwlock1_rdunlock(p); }
static void r1_wr(void *p)   { rwlock1_wrlock(p); }
static void r1_wru(void *p)  { rwlock1_wrunlock(p); }
#ifdef RWLOCK_ADVANCED
static void r2_rd(void *p)   { rwlock2_rdlock(p); }
static void r2_rdu(void *p)  { rwlock2_rdunlock(p); }
static void r2_wr(void *p)   { rwlock2_wrlock(p); }
static void r2_wru(void *p)  { rwlock2_wrunlock(p); }
#endif

static struct rwlock1 R1;
#ifdef RWLOCK_ADVANCED
static struct rwlock2 R2;
#endif
static struct rwops ops;

static unsigned deltas[PROBES];
static unsigned total_delta;

static void
reader(void *arg)
{
  for(int r = 0; r < RROUNDS; r++){
    ops.rdlock(ops.rw);
    pause(1);
    ops.rdunlock(ops.rw);
  }
  thread_exit();
}

static void
writer(void *arg)
{
  for(int p = 0; p < PROBES; p++){
    pause(3);                            // даём читателям разогнаться
    unsigned snap = *ops.rd_acquires;    // снимок ДО запроса замка
    ops.wrlock(ops.rw);
    unsigned d = *ops.rd_acquires - snap;
    deltas[p] = d;
    total_delta += d;
    pause(1);                            // подержим, пусть читатели скопятся
    ops.wrunlock(ops.rw);
  }
  thread_exit();
}

static int
run(char *name, int strict)
{
  struct futexstat a, b;
  total_delta = 0;
  for(int p = 0; p < PROBES; p++) deltas[p] = 0;

  futexstats(&a);

  int tids[NREADERS + 1], n = 0;
  for(int i = 0; i < NREADERS; i++) tids[n++] = thread_create(reader, 0);
  tids[n++] = thread_create(writer, 0);
  for(int i = 0; i < n; i++) thread_join_t(tids[i]);

  futexstats(&b);

  printf("%s: starvation per probe:", name);
  for(int p = 0; p < PROBES; p++) printf(" %d", deltas[p]);
  printf("  total=%d\n", total_delta);
  fstat_print(name, &a, &b);

  if(!strict)
    return 0;
  int ok = total_delta <= NREADERS * PROBES;
  printf("%s: %s\n", name, ok ? "OK" : "FAIL");
  return !ok;
}

int
main(void)
{
  int failed = 0;

  rwlock1_init(&R1);
  ops = (struct rwops){ &R1, r1_rd, r1_rdu, r1_wr, r1_wru, &R1.rd_acquires };
  run("rwlock1", 0);   // метрика демонстрационная: печатаем, не проверяем

#ifdef RWLOCK_ADVANCED
  rwlock2_init(&R2);
  ops = (struct rwops){ &R2, r2_rd, r2_rdu, r2_wr, r2_wru, &R2.rd_acquires };
  failed |= run("rwlock2", 1);   // строгий критерий: <= NREADERS на пробу
#endif

  exit(failed);
}