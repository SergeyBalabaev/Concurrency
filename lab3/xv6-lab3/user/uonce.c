#include "kernel/types.h"
#include "user/user.h"
#include "user/ufutex.h"
#include "user/uonce.h"

// Лекция 7, "call_once: future без значения". Код почти дословно
// повторяет fpromise; release-запись READY публикует все эффекты fn.

enum { O_EMPTY = 0, O_RUNNING = 1, O_DONE = 2 };

void
call_once(struct once *o, void (*fn)(void))
{
  /// ТУТ ВАШ КОД
}
