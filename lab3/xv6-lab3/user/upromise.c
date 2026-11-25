#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"
#include "user/ufutex.h"
#include "user/upromise.h"

// =====================================================================
// Базовый уровень: cpromise на мьютексе и условной переменной
// (лекция 7, "Две реализации"). Устроен как WaitGroup: флаг, значение,
// мьютекс, кондвар. Двух состояний хватает, потому что проверка "не
// установлено ли уже" и сама установка склеены мьютексом.
// =====================================================================

void
cpromise_init(struct cpromise *p)
{
  /// ТУТ ВАШ КОД
}

int
cpromise_set(struct cpromise *p, int v)
{
  /// ТУТ ВАШ КОД
  return -1;
}

int
cpromise_get(struct cpromise *p)
{
  /// ТУТ ВАШ КОД
  return 0;
}

int
cpromise_tryget(struct cpromise *p, int *v)
{
  /// ТУТ ВАШ КОД
  return FUTURE_AGAIN;
}

#ifdef PROMISE_ADVANCED

// =====================================================================
// Продвинутый уровень: fpromise - одно слово состояния, ожидание на
// futex, автомат EMPTY -> SETTING -> READY (лекция 7, рисунок 8).
// =====================================================================

void
fpromise_init(struct fpromise *p)
{
  /// ТУТ ВАШ КОД
}

int
fpromise_set(struct fpromise *p, int v)
{
  /// ТУТ ВАШ КОД
  return -1;
}

int
fpromise_get(struct fpromise *p)
{
  /// ТУТ ВАШ КОД
  return 0;
}

int
fpromise_tryget(struct fpromise *p, int *v)
{
  /// ТУТ ВАШ КОД
  return FUTURE_AGAIN;
}

// Основной интерфейс - на fpromise.
void promise_init(struct promise *p)            { fpromise_init(&p->f); }
int  promise_set(struct promise *p, int v)      { return fpromise_set(&p->f, v); }
int  promise_get(struct promise *p)             { return fpromise_get(&p->f); }
int  promise_tryget(struct promise *p, int *v)  { return fpromise_tryget(&p->f, v); }

#else

// Основной интерфейс - на cpromise.
void promise_init(struct promise *p)            { cpromise_init(&p->c); }
int  promise_set(struct promise *p, int v)      { return cpromise_set(&p->c, v); }
int  promise_get(struct promise *p)             { return cpromise_get(&p->c); }
int  promise_tryget(struct promise *p, int *v)  { return cpromise_tryget(&p->c, v); }

#endif
