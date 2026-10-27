#include "kernel/types.h"
#include "kernel/futex.h"    // FUTEX_WAIT, FUTEX_WAKE, FUTEX_AGAIN
#include "user/user.h"
#include "user/usem.h"

// Блок 5: семафоры. Интерфейс - в user/usem.h, его не меняйте.
// Ваша задача - написать тела функций. Заглушки, возвращающие -1,
// поставлены только для того, чтобы заготовка собиралась.

//
// ---------- Базовый уровень: мьютекс + условная переменная (раздел 8.4) ----------
//

void
csem_init(struct csem *s, unsigned value)
{
  /// ТУТ ВАШ КОД
}

void
csem_wait(struct csem *s)
{
  /// ТУТ ВАШ КОД
}

// 0 - забрали единицу, -1 - счётчик пуст
int
csem_trywait(struct csem *s)
{
  /// ТУТ ВАШ КОД
  return -1;
}

void
csem_post(struct csem *s)
{
  /// ТУТ ВАШ КОД
}

#ifdef SEM_ADVANCED

//
// ---------- Продвинутый уровень: семафор прямо на futex (раздел 8.5) ----------
//

void
fsem_init(struct fsem *s, unsigned value)
{
  /// ТУТ ВАШ КОД
}

void
fsem_wait(struct fsem *s)
{
  /// ТУТ ВАШ КОД
}

// 0 - забрали единицу, -1 - счётчик пуст
int
fsem_trywait(struct fsem *s)
{
  /// ТУТ ВАШ КОД
  return -1;
}

void
fsem_post(struct fsem *s)
{
  /// ТУТ ВАШ КОД
}

// Наивный post: FUTEX_WAKE на каждом инкременте (для теста naivepost)
void
fsem_post_naive(struct fsem *s)
{
  /// ТУТ ВАШ КОД
}

#endif // SEM_ADVANCED

//
// ---------- Основной интерфейс: обёртки над csem или fsem ----------
//

#ifdef SEM_ADVANCED

void
sem_init(struct usem *s, unsigned v)
{
  /// ТУТ ВАШ КОД
}

void
sem_wait(struct usem *s)
{
  /// ТУТ ВАШ КОД
}

int
sem_trywait(struct usem *s)
{
  /// ТУТ ВАШ КОД
  return -1;
}

void
sem_post(struct usem *s)
{
  /// ТУТ ВАШ КОД
}

#else

void
sem_init(struct usem *s, unsigned v)
{
  /// ТУТ ВАШ КОД
}

void
sem_wait(struct usem *s)
{
  /// ТУТ ВАШ КОД
}

int
sem_trywait(struct usem *s)
{
  /// ТУТ ВАШ КОД
  return -1;
}

void
sem_post(struct usem *s)
{
  /// ТУТ ВАШ КОД
}

#endif
