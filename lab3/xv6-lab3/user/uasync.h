#ifndef UASYNC_H
#define UASYNC_H

// pool_async: запустить функцию в пуле и получить promise её результата
// (лекция 7, AsyncVia). Требует: kernel/types.h, user/user.h,
// user/umutex.h, user/uqueue.h, user/upool.h, user/upromise.h.
//
// Пул ничего не знает о результатах: задача сама кладёт результат в
// promise. *out должен быть инициализирован (promise_init) и жить, пока
// его кто-то ждёт. Возврат: 0 или -1 (пул остановлен / нет памяти).
int pool_async(struct pool *p, int (*fn)(void *), void *arg, struct promise *out);

#endif
