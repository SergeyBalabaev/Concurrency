#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/uqueue.h"
#include "user/upool.h"
#include "user/upromise.h"
#include "user/uasync.h"

// Задача пула принимает один void*, а нам нужно передать три значения:
// функцию, её аргумент и promise. Упаковываем их в структуру в куче:
// стек вызывающего может закончиться раньше, чем задача начнётся.
// Освобождает структуру сама задача - malloc/free из потоков после
// шага 1 безопасны.
struct async_call {
  int (*fn)(void *);
  void *arg;
  struct promise *out;
};

void
async_trampoline(void *a)
{
  /// ТУТ ВАШ КОД
}

int
pool_async(struct pool *p, int (*fn)(void *), void *arg, struct promise *out)
{
  /// ТУТ ВАШ КОД
  return -1;
}
