#ifndef UPOOL_H
#define UPOOL_H

// Пул потоков (БДЗ-3, шаг 3).
// Требует: kernel/types.h, user/user.h, user/umutex.h, user/uqueue.h.

#define POOL_MAX 8

struct pool {
  struct bqueue q;             // вся синхронизация пула - в очереди
  int tids[POOL_MAX];
  int nworkers;
};

// Запустить nworkers рабочих (1..POOL_MAX). Возврат: 0 или -1.
int  pool_init(struct pool *p, int nworkers);

// Запланировать fn(arg). Возврат из submit означает только "задача
// в очереди", а не "задача выполнена". -1 - пул уже остановлен.
int  pool_submit(struct pool *p, void (*fn)(void *), void *arg);

// Рабочая функция потока пула: берёт задачи из очереди и выполняет,
// пока очередь не закрыта и не пуста. Вызывается только из pool_init.
void pool_worker(void *arg);

// Остановить пул: новые задачи не принимаются, уже принятые будут
// выполнены, затем все рабочие завершатся и будут присоединены.
// Вызывать из того потока, который делал pool_init.
void pool_shutdown(struct pool *p);

#endif
