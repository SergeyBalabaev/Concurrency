#ifndef UFUTEX_H
#define UFUTEX_H

// Тонкие обёртки над системным вызовом futex из БДЗ-2 - в тех же
// именах, что в конспекте лекции 7. Требует: kernel/types.h, user/user.h.

#include "kernel/futex.h"

#define WAKE_ALL 0x7fffffff

// Уснуть, только если *addr всё ещё равно val. Возврат: 0 - проснулись,
// FUTEX_AGAIN - значение уже другое (в ядро сходили, но не уснули).
static inline int
futex_wait(volatile int *addr, int val)
{
  return futex(addr, FUTEX_WAIT, val);
}

// Разбудить не больше n ожидающих на addr (WAKE_ALL - всех).
static inline int
futex_wake(volatile int *addr, int n)
{
  return futex(addr, FUTEX_WAKE, n);
}

#endif
