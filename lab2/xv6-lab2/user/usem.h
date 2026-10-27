#ifndef USEM_H
#define USEM_H

// Семафоры поверх futex. Требует: kernel/types.h, user/user.h.

#include "user/umutex.h"   // mutex, cond

// Продвинутый уровень: раскомментируйте, когда реализуете fsem.
// Тогда struct usem и функции sem_* переключатся на него.
// #define SEM_ADVANCED

//
// Базовый уровень: семафор из мьютекса и условной переменной.
//
struct csem {
  struct mutex m;
  struct cond  cv;
  unsigned     value;
};

void csem_init(struct csem *s, unsigned value);
void csem_wait(struct csem *s);
int  csem_trywait(struct csem *s);   // 0 — успех, -1 — счётчик пуст
void csem_post(struct csem *s);

#ifdef SEM_ADVANCED
//
// Продвинутый уровень: семафор непосредственно над futex.
// value и nwaiters — отдельные слова (так устроен sem_t в glibc).
//
struct fsem {
  unsigned value;      // ключ futex: FUTEX_WAIT/FUTEX_WAKE работают по этому адресу
  unsigned nwaiters;   // число потоков в медленном пути fsem_wait
};

void fsem_init(struct fsem *s, unsigned value);
void fsem_wait(struct fsem *s);
int  fsem_trywait(struct fsem *s);
void fsem_post(struct fsem *s);

// Наивный post: безусловный FUTEX_WAKE на каждом инкременте.
// Корректен, но разоблачается счётчиком wake_calls (тест naivepost).
void fsem_post_naive(struct fsem *s);
#endif

//
// Основной интерфейс: им пользуются тесты и барьер.
//
struct usem {
#ifdef SEM_ADVANCED
  struct fsem impl;
#else
  struct csem impl;
#endif
};

void sem_init(struct usem *s, unsigned value);
void sem_wait(struct usem *s);
int  sem_trywait(struct usem *s);
void sem_post(struct usem *s);

#endif
