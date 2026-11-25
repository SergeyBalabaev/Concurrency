// mutextest — корректность и цена вариантов мьютекса.
//
// Тест A (solo): один поток, M_SOLO циклов lock/unlock, конкуренции нет.
//   Смотрим wake_calls: сколько раз unlock сходил в ядро впустую.
//   Ожидаемо: xchg и drepper1 — M_SOLO, варианты на трёх состояниях — 0.
//
// Тест B (contended): NTHREAD потоков по M_EACH инкрементов общего
//   счётчика под мьютексом; итог обязан быть ровно NTHREAD * M_EACH.
//   Прогоняется с короткой и с длинной критической секцией.
//   Смотрим соотношение again/slept: у drepper1 лавина FUTEX_AGAIN,
//   у остальных почти каждый уход в ядро кончается сном.
//
// Базовый уровень сравнивает только xchg. С MUTEX_ADVANCED (umutex.h)
// добавляются варианты из статьи Дреппера.

#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/umutex.h"
#include "user/fstat.h"

#define M_SOLO   5000
#define NTHREAD  6         // не больше MAX_THREADS - 1
#define M_EACH   10000
#define LONG_CS  500       // итераций пустой работы в длинной секции

struct impl {
  char *name;
  void (*lock)(struct mutex *);
  void (*unlock)(struct mutex *);
};

static struct impl impls[] = {
  { "xchg    ", mutex_xchg_lock,   mutex_xchg_unlock },
#ifdef MUTEX_ADVANCED
  { "drepper1", drepper1_lock,     drepper1_unlock   },
  { "drepper2", drepper2_lock,     drepper_unlock    },
  { "drepper3", drepper3_lock,     drepper_unlock    },
  { "spin    ", drepper_spin_lock, drepper_unlock    },
#endif
};

#define NIMPL (sizeof(impls) / sizeof(impls[0]))

static struct mutex m;
static struct impl *cur;

// ---------- тест A: без конкуренции ----------

static void
solo(struct impl *im)
{
  struct futexstat a, b;
  char tag[32];

  mutex_init(&m);
  futexstats(&a);
  int t0 = uptime();
  for (int i = 0; i < M_SOLO; i++) {
    im->lock(&m);
    im->unlock(&m);
  }
  int t1 = uptime();
  futexstats(&b);

  strcpy(tag, im->name);
  strcpy(tag + strlen(tag), " solo ");
  printf("ticks=%d  ", t1 - t0);
  fstat_print(tag, &a, &b);
}

// ---------- тест B: конкуренция ----------

static int counter;
static int cs_work;               // длина критической секции
static volatile int go;           // стартовый флаг: все потоки начинают разом

static void
worker(void *arg)
{
  volatile int x = 0;
  // Ждём, пока main создаст всех. Без этого первый поток успевал бы
  // выполнить все M_EACH итераций, пока создаётся следующий (clone
  // дороже сотни итераций), потоки не перекрывались бы, и тест не
  // поймал бы даже отсутствие мьютекса.
  while (!go)
    ;
  for (int i = 0; i < M_EACH; i++) {
    cur->lock(&m);
    counter++;
    for (int j = 0; j < cs_work; j++)
      x++;
    cur->unlock(&m);
  }
  thread_exit();
}

static int
contended(struct impl *im, int work, char *len)
{
  struct futexstat a, b;
  int tid[NTHREAD];
  char tag[32];

  mutex_init(&m);
  cur = im;
  counter = 0;
  cs_work = work;
  go = 0;

  for (int i = 0; i < NTHREAD; i++)
    tid[i] = thread_create(worker, 0);
  futexstats(&a);
  int t0 = uptime();
  go = 1;
  for (int i = 0; i < NTHREAD; i++)
    thread_join_t(tid[i]);
  int t1 = uptime();
  futexstats(&b);

  strcpy(tag, im->name);
  strcpy(tag + strlen(tag), len);
  printf("ticks=%d  ", t1 - t0);
  fstat_print(tag, &a, &b);

  if (counter != NTHREAD * M_EACH) {
    printf("  FAIL: counter=%d expected=%d\n", counter, NTHREAD * M_EACH);
    return 1;
  }
  return 0;
}

int
main(void)
{
  int failed = 0;

  printf("mutextest: solo M=%d, contended %d x %d\n",
         M_SOLO, NTHREAD, M_EACH);

  printf("--- uncontended ---\n");
  for (int i = 0; i < NIMPL; i++)
    solo(&impls[i]);

  printf("--- contended, short cs ---\n");
  for (int i = 0; i < NIMPL; i++)
    failed |= contended(&impls[i], 0, " short");

  printf("--- contended, long cs ---\n");
  for (int i = 0; i < NIMPL; i++)
    failed |= contended(&impls[i], LONG_CS, " long ");

  printf(failed ? "mutextest: FAIL\n" : "mutextest: OK\n");
  exit(failed);
}
