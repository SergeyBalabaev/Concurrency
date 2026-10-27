// rwtest.c — корректность rwlock1 (и rwlock2 на продвинутом уровне):
// неделимость записи, инварианты.
//
// Писатели переписывают массив cells поэлементно, с sleep(1) посередине,
// нарочно распахивая окно для порванного чтения. Читатель под rdlock обязан
// видеть все ячейки равными. Дополнительно счётчики inside_r/inside_w
// проверяют сами инварианты rwlock: читатель не должен застать писателя,
// писатель — вообще никого. Обе версии обязаны проходить; предложите
// студентам убрать rdlock и убедиться, что тест ловит разрывы.

#include "kernel/types.h"
#include "user/user.h"
#include "umutex.h"
#include "urwlock.h"
#include "uthread.h"
#include "fstat.h"

#define NCELLS   16
#define NREADERS 6
#define NWRITERS 3
#define RROUNDS  40       // каждый раунд читателя - плюс пауза в тик
#define WROUNDS  30

// Обёртки приводят конкретный тип к void*: один код теста на обе версии.
struct rwops {
  void *rw;
  void (*rdlock)(void*);
  void (*rdunlock)(void*);
  void (*wrlock)(void*);
  void (*wrunlock)(void*);
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

static int cells[NCELLS];
static int wseq;                 // последовательный номер записи (под wrlock)
static int inside_r, inside_w;   // сколько сейчас внутри (инструментовка)
static int torn, violations;     // счётчики нарушений
static volatile int go;          // стартовый флаг: все потоки начинают разом

static void
reader(void *arg)
{
  while(!go)
    ;
  for(int r = 0; r < RROUNDS; r++){
    ops.rdlock(ops.rw);
    __atomic_fetch_add(&inside_r, 1, __ATOMIC_SEQ_CST);
    if(inside_w != 0)
      __atomic_fetch_add(&violations, 1, __ATOMIC_SEQ_CST);
    int v = cells[0];
    for(int i = 1; i < NCELLS; i++)
      if(cells[i] != v){
        __atomic_fetch_add(&torn, 1, __ATOMIC_SEQ_CST);
        break;
      }
    __atomic_fetch_add(&inside_r, -1, __ATOMIC_SEQ_CST);
    ops.rdunlock(ops.rw);
    // Пауза вне секции: читатели не заканчивают работу раньше, чем
    // начнут писатели, и при этом дают писателям войти (у rwlock1
    // непрерывный поток читателей заморил бы писателей голодом).
    pause(1);
  }
  thread_exit();
}

static void
writer(void *arg)
{
  while(!go)
    ;
  for(int r = 0; r < WROUNDS; r++){
    ops.wrlock(ops.rw);
    inside_w = 1;                        // под исключительным захватом
    if(inside_r != 0)
      __atomic_fetch_add(&violations, 1, __ATOMIC_SEQ_CST);
    int v = ++wseq;
    for(int i = 0; i < NCELLS/2; i++)
      cells[i] = v;
    pause(1);                            // распахиваем окно разрыва
    for(int i = NCELLS/2; i < NCELLS; i++)
      cells[i] = v;
    inside_w = 0;
    ops.wrunlock(ops.rw);
  }
  thread_exit();
}

static int
run(char *name)
{
  for(int i = 0; i < NCELLS; i++) cells[i] = 0;
  wseq = 0; inside_r = 0; inside_w = 0; torn = 0; violations = 0;
  go = 0;

  struct futexstat a, b;
  int tids[NREADERS + NWRITERS], n = 0;
  for(int i = 0; i < NREADERS; i++) tids[n++] = thread_create(reader, 0);
  for(int i = 0; i < NWRITERS; i++) tids[n++] = thread_create(writer, 0);
  futexstats(&a);
  go = 1;
  for(int i = 0; i < n; i++) thread_join_t(tids[i]);
  futexstats(&b);

  int ok = (torn == 0 && violations == 0 && wseq == NWRITERS*WROUNDS);
  for(int i = 0; i < NCELLS; i++)
    if(cells[i] != wseq) ok = 0;
  printf("%s: torn=%d violations=%d writes=%d %s\n",
         name, torn, violations, wseq, ok ? "OK" : "FAIL");
  // Побудки: здесь несколько писателей, и при уходе писателя в общей
  // очереди rwlock1 спят и читатели, и писатели - broadcast будит всех.
  fstat_print(name, &a, &b);
  return !ok;
}

int
main(void)
{
  int failed = 0;

  rwlock1_init(&R1);
  ops = (struct rwops){ &R1, r1_rd, r1_rdu, r1_wr, r1_wru };
  failed |= run("rwlock1");

#ifdef RWLOCK_ADVANCED
  rwlock2_init(&R2);
  ops = (struct rwops){ &R2, r2_rd, r2_rdu, r2_wr, r2_wru };
  failed |= run("rwlock2");
#endif

  exit(failed);
}