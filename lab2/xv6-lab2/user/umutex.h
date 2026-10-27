#ifndef UMUTEX_H
#define UMUTEX_H

// Пользовательские мьютекс и условная переменная поверх futex.
// Требует: kernel/types.h, user/user.h (futex, futexstats).

#include "kernel/futex.h"   // FUTEX_WAIT/FUTEX_WAKE/FUTEX_AGAIN, struct futexstat

// Продвинутый уровень: раскомментируйте, когда реализуете мьютекс на
// трёх состояниях. Тогда mutex_lock/mutex_unlock (а значит, и всё, что
// построено поверх них: cond, csem, rwlock, uthread) переключатся на
// него, а mutextest добавит в сравнение варианты из статьи Дреппера.
// #define MUTEX_ADVANCED

// Одно слово на все варианты. Смысл значений зависит от протокола:
//   двухфазный (xchg):  0 свободен, 1 занят;
//   три состояния:      0 свободен, 1 занят, 2 занят + есть ожидающие;
//   Take 1 (Дреппер):   0 свободен, >0 занят (растёт от ожидающих).
struct mutex {
  int val;
};

// Основной интерфейс: им пользуются условные переменные, семафоры,
// rwlock и uthread. Реализация выбирается макросом MUTEX_ADVANCED.
void mutex_init(struct mutex *m);
void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

// Захват "в предположении конкуренции": сразу помечает мьютекс как
// имеющий ожидающих. Нужен условной переменной с requeue (доп. задание),
// у базового мьютекса совпадает с mutex_lock.
void mutex_lock_contended(struct mutex *m);

// --- Базовый уровень: два состояния, перенос sleeplock в userspace ---
void mutex_xchg_lock(struct mutex *m);
void mutex_xchg_unlock(struct mutex *m);

#ifdef MUTEX_ADVANCED
// --- Продвинутый уровень: варианты из "Futexes Are Tricky" ---
void drepper1_lock(struct mutex *m);     // Take 1: инкремент
void drepper1_unlock(struct mutex *m);
void drepper2_lock(struct mutex *m);     // Take 2: три состояния, cmpxchg
void drepper3_lock(struct mutex *m);     // Take 3: три состояния, xchg
void drepper_spin_lock(struct mutex *m); // Take 3 + спин перед сном
void drepper_unlock(struct mutex *m);    // общий для Take 2, Take 3, spin
#endif

// --- Условная переменная на счётчике поколений ---
struct cond {
  unsigned seq;
};

void cond_init(struct cond *cv);
void cond_wait(struct cond *cv, struct mutex *m);
void cond_signal(struct cond *cv);
void cond_broadcast(struct cond *cv);

#ifdef MUTEX_ADVANCED
// Дополнительное задание: broadcast без стада через futex_requeue.
// Ожидающие спят на seq; broadcast будит одного, остальных перевешивает
// в очередь слова мьютекса. Мьютекс запоминается при cond_wait.
struct condrq {
  unsigned seq;
  struct mutex *m;
};

void condrq_init(struct condrq *cv);
void condrq_wait(struct condrq *cv, struct mutex *m);
void condrq_signal(struct condrq *cv);
void condrq_broadcast(struct condrq *cv);
#endif

#endif
