#ifndef UTHEN_H
#define UTHEN_H

// Асинхронные фьючи: Subscribe, Then, All (БДЗ-3, дополнительное задание 3).
// Требует: kernel/types.h, user/user.h, user/umutex.h, user/uqueue.h,
//          user/upool.h.
//
// apromise - одноразовый канал int от ОДНОГО продюсера (ap_set) к ОДНОМУ
// консьюмеру (ap_subscribe), как Future/Promise на лекции. Колбэк и
// значение приходят в разделяемое состояние с двух сторон; кто пришёл
// вторым, тот и вызывает колбэк (лекция 7, рисунок 4).

typedef void (*ap_callback)(int value, void *arg);

#define AP_HAS_VALUE    1
#define AP_HAS_CALLBACK 2

struct apromise {
  int state;                   // биты AP_HAS_VALUE | AP_HAS_CALLBACK
  int value;
  ap_callback cb;
  void *cb_arg;
};

void ap_init(struct apromise *p);

// Продюсер: положить значение (ровно один раз).
void ap_set(struct apromise *p, int v);

// Консьюмер: повесить колбэк (ровно один раз). Не блокируется.
// Колбэк вызывается в потоке ap_set, если подписка была раньше,
// или прямо здесь, если значение уже было.
void ap_subscribe(struct apromise *p, ap_callback cb, void *arg);

// Блокирующее ожидание - снаружи, через subscribe (тратит подписку).
int  ap_get(struct apromise *p);

// Запустить fn(arg) в пуле, результат положить в *out.
int  ap_async(struct pool *pl, int (*fn)(void *), void *arg, struct apromise *out);

// Then: когда in будет готов, положить fn(значение) в out.
// Не блокируется; in потрачен (подписка занята).
int  ap_then(struct apromise *in, int (*fn)(int), struct apromise *out);

// All: когда готовы все n входов, разложить значения в values[0..n-1]
// и положить n в out. Не блокируется.
int  ap_all(struct apromise **in, int n, int *values, struct apromise *out);

#endif
