#include "kernel/types.h"
#include "user/user.h"
#include "user/umutex.h"

// Мьютекс и условная переменная поверх futex (БДЗ-2, блоки 3 и 4).
//
// В заготовке БДЗ-3 заполнен базовый уровень - ровно тот код, что
// приведён целиком в методичке БДЗ-2 (разделы 6.2 и 7.2). Если вы
// сделали в БДЗ-2 продвинутый уровень (мьютекс Дреппера), перенесите
// сюда свои функции и включите MUTEX_ADVANCED в umutex.h: всё, что
// построено в БДЗ-3, автоматически переключится на ваш мьютекс.

// =====================================================================
// Базовый уровень: мьютекс на двух состояниях (раздел 6.2)
// =====================================================================

void
mutex_xchg_lock(struct mutex *m)
{
  while (__atomic_exchange_n(&m->val, 1, __ATOMIC_SEQ_CST) != 0)
    futex(&m->val, FUTEX_WAIT, 1);
}

void
mutex_xchg_unlock(struct mutex *m)
{
  __atomic_store_n(&m->val, 0, __ATOMIC_SEQ_CST);
  futex(&m->val, FUTEX_WAKE, 1);
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
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

void
drepper1_unlock(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

// Take 2: три состояния, только сравнение с обменом
void
drepper2_lock(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

// Take 3: три состояния, обмен
void
drepper3_lock(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

// Take 3 + спин перед сном
void
drepper_spin_lock(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

// Общий unlock для Take 2, Take 3 и spin
void
drepper_unlock(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

#endif // MUTEX_ADVANCED

// =====================================================================
// Основной интерфейс
// =====================================================================

void
mutex_init(struct mutex *m)
{
  m->val = 0;
}

#ifdef MUTEX_ADVANCED

// Продвинутый уровень: основной интерфейс - на Take 3.
void
mutex_lock(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

void
mutex_unlock(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

// Нужен только для дополнительного задания FUTEX_REQUEUE (condrq):
// захват, сразу переводящий слово в состояние 2.
void
mutex_lock_contended(struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

#else

// Базовый уровень: основной интерфейс - обёртки над mutex_xchg_*.
void
mutex_lock(struct mutex *m)
{
  mutex_xchg_lock(m);
}

void
mutex_unlock(struct mutex *m)
{
  mutex_xchg_unlock(m);
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
  cv->seq = 0;
}

void
cond_wait(struct cond *cv, struct mutex *m)
{
  unsigned old = cv->seq;         // читаем поколение, ещё держа мьютекс
  mutex_unlock(m);
  futex((int *)&cv->seq, FUTEX_WAIT, (int)old);
  mutex_lock(m);
}

void
cond_signal(struct cond *cv)
{
  __atomic_fetch_add(&cv->seq, 1, __ATOMIC_SEQ_CST);
  futex((int *)&cv->seq, FUTEX_WAKE, 1);
}

void
cond_broadcast(struct cond *cv)
{
  __atomic_fetch_add(&cv->seq, 1, __ATOMIC_SEQ_CST);
  futex((int *)&cv->seq, FUTEX_WAKE, 0x7fffffff);
}

#ifdef MUTEX_ADVANCED

// =====================================================================
// Дополнительное задание: условная переменная с requeue (раздел 7.4)
// =====================================================================

void
condrq_init(struct condrq *cv)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

void
condrq_wait(struct condrq *cv, struct mutex *m)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

void
condrq_signal(struct condrq *cv)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

void
condrq_broadcast(struct condrq *cv)
{
  /// перенесите своё решение из БДЗ-2 (продвинутый уровень)
}

#endif // MUTEX_ADVANCED
