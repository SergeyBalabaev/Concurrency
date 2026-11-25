#ifndef UQUEUE_H
#define UQUEUE_H

// Ограниченная блокирующая очередь задач (БДЗ-3, шаг 2).
// Требует: kernel/types.h, user/user.h, user/umutex.h.
//
// Элемент очереди - задача {fn, arg}, хранится в кольцевом буфере по
// значению: ни put, ни take не выделяют память.

#define QUEUE_MAX 64

struct task {
  void (*fn)(void *);
  void *arg;
};

struct bqueue {
  struct mutex m;              // защищает все поля ниже
  struct cond not_empty;       // ждут take: "появился элемент или закрыли"
  struct cond not_full;        // ждут put:  "появилось место или закрыли"
  struct task buf[QUEUE_MAX];
  int cap;                     // ёмкость, 1..QUEUE_MAX
  int head;                    // индекс самого старого элемента
  int count;                   // сколько элементов в буфере
  int closed;                  // 1 - после bq_close
};

// cap <= 0 или > QUEUE_MAX означает QUEUE_MAX.
void bq_init(struct bqueue *q, int cap);

// Положить задачу; если очередь полна - ждать места.
// Возврат: 0 - положили, -1 - очередь закрыта (задача не принята).
int  bq_put(struct bqueue *q, struct task t);

// Взять задачу; если очередь пуста - ждать.
// Возврат: 0 - взяли (в *t), -1 - очередь закрыта И пуста: пора выходить.
int  bq_take(struct bqueue *q, struct task *t);

// Закрыть очередь: новые put отвергаются, take отдают оставшееся,
// а на пустой очереди возвращают -1 вместо сна. Будит всех ждущих.
void bq_close(struct bqueue *q);

#endif
