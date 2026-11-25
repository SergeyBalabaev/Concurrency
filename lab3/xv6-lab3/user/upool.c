#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/uthread.h"

// Пул потоков поверх блокирующей очереди (лекция 7, "Пул как
// планировщик"). Пул не знает ни мьютекса, ни кондвара - только
// очередь с гарантией "take заблокируется, пока не появится элемент".

void
pool_worker(void *arg)
{
  /// ТУТ ВАШ КОД
}

int
pool_init(struct pool *p, int nworkers)
{
  /// ТУТ ВАШ КОД
  return -1;
}

int
pool_submit(struct pool *p, void (*fn)(void *), void *arg)
{
  /// ТУТ ВАШ КОД
  return -1;
}

void
pool_shutdown(struct pool *p)
{
  /// ТУТ ВАШ КОД
}
