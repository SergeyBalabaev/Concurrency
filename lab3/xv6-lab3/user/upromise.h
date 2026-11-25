#ifndef UPROMISE_H
#define UPROMISE_H

// Promise/future для одного int (БДЗ-3, шаг 5).
// Требует: kernel/types.h, user/user.h, user/umutex.h.
//
// Promise одноразовый на запись (повторный set возвращает -1: "первый
// выигрывает") и многоразовый на чтение: get могут вызвать сколько
// угодно потоков, и каждый получит значение (как shared_future в C++).

// Продвинутый уровень: раскомментируйте, когда реализуете fpromise.
// Тогда основной интерфейс promise_* переключится на него, а
// promisetest и promisestat добавят его в сравнение.
// #define PROMISE_ADVANCED

#define FUTURE_AGAIN 1          // tryget: значения ещё нет

// --- Базовый уровень: мьютекс + условная переменная, два состояния ---
struct cpromise {
  struct mutex m;
  struct cond ready_cv;        // событие "значение готово"
  int ready;                   // 0 - пусто, 1 - готово
  int value;
};

void cpromise_init(struct cpromise *p);
int  cpromise_set(struct cpromise *p, int v);        // 0 или -1
int  cpromise_get(struct cpromise *p);               // ждёт значение
int  cpromise_tryget(struct cpromise *p, int *v);    // 0 или FUTURE_AGAIN

#ifdef PROMISE_ADVANCED
// --- Продвинутый уровень: одно слово состояния на futex, три состояния ---
#define P_EMPTY   0            // значения нет
#define P_SETTING 1            // право записи куплено, готовность не объявлена
#define P_READY   2            // значение записано и опубликовано

struct fpromise {
  int state;
  int value;
};

void fpromise_init(struct fpromise *p);
int  fpromise_set(struct fpromise *p, int v);
int  fpromise_get(struct fpromise *p);
int  fpromise_tryget(struct fpromise *p, int *v);
#endif

// --- Основной интерфейс: им пользуются тесты и pool_async ---
#ifdef PROMISE_ADVANCED
struct promise { struct fpromise f; };
#else
struct promise { struct cpromise c; };
#endif

void promise_init(struct promise *p);
int  promise_set(struct promise *p, int v);
int  promise_get(struct promise *p);
int  promise_tryget(struct promise *p, int *v);

#endif
