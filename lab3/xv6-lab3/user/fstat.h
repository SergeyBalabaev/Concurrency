#ifndef FSTAT_H
#define FSTAT_H

// fstat.h — общий инструмент замеров для тестов синхронизации.
//
// Каждый замер устроен одинаково: снимок счётчиков futexstats до
// сценария, снимок после, печать разности. Абсолютные значения
// счётчиков смысла не имеют (они копятся с загрузки системы) —
// смотрим только на дельту.
//
//   struct futexstat a, b;
//   futexstats(&a);
//   ... сценарий ...
//   futexstats(&b);
//   fstat_print("mutex solo", &a, &b);
//
// Требует: kernel/types.h, user/user.h, kernel/futex.h.

static inline void
fstat_print(char *tag, struct futexstat *a, struct futexstat *b)
{
  printf("%s: slept=%d again=%d woken=%d wake_calls=%d",
         tag,
         (int)(b->wait_slept - a->wait_slept),
         (int)(b->wait_again - a->wait_again),
         (int)(b->woken      - a->woken),
         (int)(b->wake_calls - a->wake_calls));
  if (b->requeued != a->requeued)
    printf(" requeued=%d", (int)(b->requeued - a->requeued));
  if (b->join_sleeps != a->join_sleeps)
    printf(" join_sleeps=%d", (int)(b->join_sleeps - a->join_sleeps));
  printf("\n");
}

#endif
