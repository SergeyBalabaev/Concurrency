// prodcons — производитель-потребитель на кольцевом буфере.
//
// Условная переменная используется по прямому назначению: not_empty
// будит потребителей, not_full — производителей. Корректность
// проверяется контрольной суммой: сумма всего произведённого должна
// совпасть с суммой всего потреблённого, а число извлечённых
// элементов — с числом положенных.
//
// Завершение: main дожидается производителей, под мьютексом ставит
// done=1 и делает broadcast на not_empty — потребители, увидев
// done при пустом буфере, выходят. Это ровно та идиома, где signal
// недостаточен: спящих потребителей может быть больше одного.
//
// Ожидаемо в futexstats: slept и woken растут умеренно (потоки спят
// только когда буфер реально пуст/полон), again ненулевой, но
// небольшой — это нормальные проигрыши гонки за сигналом.

#include "kernel/types.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/umutex.h"
#include "user/fstat.h"

#define NBUF   8
#define NPROD  2
#define NCONS  3
#define ITEMS  5000            // элементов на одного производителя

static int buf[NBUF];
static int head, tail, count;

static struct mutex m;
static struct cond not_empty;
static struct cond not_full;

static int done;               // защищается m
static int produced_sum;       // защищается m
static int consumed_sum;       // защищается m
static int consumed_cnt;       // защищается m

static void
producer(void *arg)
{
  int id = (int)(uint64)arg;
  for (int i = 1; i <= ITEMS; i++) {
    int v = id * ITEMS + i;    // уникальные значения у разных производителей
    mutex_lock(&m);
    while (count == NBUF)
      cond_wait(&not_full, &m);
    buf[tail] = v;
    tail = (tail + 1) % NBUF;
    count++;
    produced_sum += v;
    cond_signal(&not_empty);
    mutex_unlock(&m);
  }
  thread_exit();
}

static void
consumer(void *arg)
{
  for (;;) {
    mutex_lock(&m);
    while (count == 0 && !done)
      cond_wait(&not_empty, &m);
    if (count == 0 && done) {
      mutex_unlock(&m);
      break;
    }
    int v = buf[head];
    head = (head + 1) % NBUF;
    count--;
    consumed_sum += v;
    consumed_cnt++;
    cond_signal(&not_full);
    mutex_unlock(&m);
  }
  thread_exit();
}

int
main(void)
{
  int ptid[NPROD], ctid[NCONS];
  struct futexstat s0, s1;

  mutex_init(&m);
  cond_init(&not_empty);
  cond_init(&not_full);

  printf("prodcons: %d prod x %d items, %d cons, buf=%d\n",
         NPROD, ITEMS, NCONS, NBUF);

  futexstats(&s0);
  int t0 = uptime();

  for (int i = 0; i < NCONS; i++)
    ctid[i] = thread_create(consumer, 0);
  for (int i = 0; i < NPROD; i++)
    ptid[i] = thread_create(producer, (void *)(uint64)i);

  for (int i = 0; i < NPROD; i++)
    thread_join_t(ptid[i]);

  mutex_lock(&m);
  done = 1;
  mutex_unlock(&m);
  cond_broadcast(&not_empty);

  for (int i = 0; i < NCONS; i++)
    thread_join_t(ctid[i]);

  int t1 = uptime();
  futexstats(&s1);

  if (consumed_cnt != NPROD * ITEMS || consumed_sum != produced_sum) {
    printf("prodcons: FAIL cnt=%d/%d sum=%d/%d\n",
           consumed_cnt, NPROD * ITEMS, consumed_sum, produced_sum);
    exit(1);
  }
  printf("prodcons: OK ticks=%d\n", t1 - t0);
  fstat_print("prodcons", &s0, &s1);
  exit(0);
}