// urwlock.c — блокировки чтения-записи поверх mutex и cond (umutex)
//
// Блок 6. Интерфейс - в user/urwlock.h, его не меняйте.
// Ваша задача - написать тела функций.

#include "kernel/types.h"
#include "user/user.h"
#include "umutex.h"
#include "urwlock.h"

// ---------------------------------------------------------------- rwlock1
// Базовый уровень: reader-preference, одна условная переменная (раздел 9.2).
// Не забудьте увеличивать rd_acquires при каждом читательском захвате -
// по нему тест wrstarve измеряет голодание.

void
rwlock1_init(struct rwlock1 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock1_rdlock(struct rwlock1 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock1_rdunlock(struct rwlock1 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock1_wrlock(struct rwlock1 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock1_wrunlock(struct rwlock1 *rw)
{
  /// ТУТ ВАШ КОД
}

#ifdef RWLOCK_ADVANCED

// ---------------------------------------------------------------- rwlock2
// Продвинутый уровень: writer-preference, две условные переменные (раздел 9.5).

void
rwlock2_init(struct rwlock2 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock2_rdlock(struct rwlock2 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock2_rdunlock(struct rwlock2 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock2_wrlock(struct rwlock2 *rw)
{
  /// ТУТ ВАШ КОД
}

void
rwlock2_wrunlock(struct rwlock2 *rw)
{
  /// ТУТ ВАШ КОД
}

#endif // RWLOCK_ADVANCED
