#ifndef UWAITGROUP_H
#define UWAITGROUP_H

// WaitGroup - ожидание группы задач (БДЗ-3, шаг 4).
// Требует: kernel/types.h, user/user.h, user/umutex.h.

struct waitgroup {
  struct mutex m;
  struct cond zero;            // событие "счётчик стал нулём"
  int count;                   // незавершённые задачи
};

void wg_init(struct waitgroup *wg);
void wg_add(struct waitgroup *wg, int n);   // ДО запуска задач!
void wg_done(struct waitgroup *wg);         // последнее действие задачи
void wg_wait(struct waitgroup *wg);         // ждать, пока count != 0

#endif
