#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"

// Блокирующая очередь на мьютексе и двух условных переменных
// (лекция 7, раздел "Блокирующая очередь"; БДЗ-2, prodcons).

void
bq_init(struct bqueue *q, int cap)
{
  /// ТУТ ВАШ КОД
}

int
bq_put(struct bqueue *q, struct task t)
{
  /// ТУТ ВАШ КОД
  return -1;
}

int
bq_take(struct bqueue *q, struct task *t)
{
  /// ТУТ ВАШ КОД
  return -1;
}

void
bq_close(struct bqueue *q)
{
  /// ТУТ ВАШ КОД
}
