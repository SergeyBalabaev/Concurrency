#ifndef UONCE_H
#define UONCE_H

// call_once - fpromise без значения (БДЗ-3, дополнительное задание 1).
// Требует: kernel/types.h, user/user.h.

#define ONCE_INIT { 0 }

struct once {
  int state;                   // те же P_EMPTY / P_SETTING / P_READY
};

// Ровно один из вызвавших выполнит fn; остальные дождутся её завершения.
// После возврата все эффекты fn видны вызвавшему потоку.
void call_once(struct once *o, void (*fn)(void));

#endif
