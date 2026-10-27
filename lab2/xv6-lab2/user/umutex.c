#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"

// Блоки 3 и 4: мьютекс и условная переменная поверх futex.
// Интерфейс - в user/umutex.h, его не меняйте: им пользуются тесты,
// семафоры, rwlock. Ваша задача - написать тела функций.

// =====================================================================
// Базовый уровень: мьютекс на двух состояниях (раздел 6.2)
// =====================================================================

void
mutex_xchg_lock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

void
mutex_xchg_unlock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

#ifdef MUTEX_ADVANCED

// =====================================================================
// Продвинутый уровень: варианты из статьи U. Drepper, "Futexes Are
// Tricky" (раздел 6.3). Номера совпадают с разделами статьи.
// =====================================================================

// Take 1: инкремент
void
drepper1_lock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

void
drepper1_unlock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

// Take 2: три состояния, только сравнение с обменом
void
drepper2_lock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

// Take 3: три состояния, обмен
void
drepper3_lock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

// Take 3 + спин перед сном
void
drepper_spin_lock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

// Общий unlock для Take 2, Take 3 и spin
void
drepper_unlock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

#endif // MUTEX_ADVANCED

// =====================================================================
// Основной интерфейс
// =====================================================================

void
mutex_init(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

#ifdef MUTEX_ADVANCED

// Продвинутый уровень: основной интерфейс - на Take 3.
void
mutex_lock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

void
mutex_unlock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

// Нужен только для дополнительного задания FUTEX_REQUEUE (condrq):
// захват, сразу переводящий слово в состояние 2.
void
mutex_lock_contended(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

#else

// Базовый уровень: основной интерфейс - обёртки над mutex_xchg_*.
void
mutex_lock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

void
mutex_unlock(struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

void
mutex_lock_contended(struct mutex *m)
{
  mutex_lock(m);   // на базовом уровне совпадает с mutex_lock
}

#endif

// =====================================================================
// Условная переменная на счётчике поколений (раздел 7.2)
// =====================================================================

void
cond_init(struct cond *cv)
{
  /// ТУТ ВАШ КОД
}

void
cond_wait(struct cond *cv, struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

void
cond_signal(struct cond *cv)
{
  /// ТУТ ВАШ КОД
}

void
cond_broadcast(struct cond *cv)
{
  /// ТУТ ВАШ КОД
}

#ifdef MUTEX_ADVANCED

// =====================================================================
// Дополнительное задание: условная переменная с requeue (раздел 7.4)
// =====================================================================

void
condrq_init(struct condrq *cv)
{
  /// ТУТ ВАШ КОД
}

void
condrq_wait(struct condrq *cv, struct mutex *m)
{
  /// ТУТ ВАШ КОД
}

void
condrq_signal(struct condrq *cv)
{
  /// ТУТ ВАШ КОД
}

void
condrq_broadcast(struct condrq *cv)
{
  /// ТУТ ВАШ КОД
}

#endif // MUTEX_ADVANCED
