// urwlock.h — блокировка чтения-записи поверх mutex и cond (umutex)

#ifndef URWLOCK_H
#define URWLOCK_H

#include "umutex.h"

// Продвинутый уровень: раскомментируйте, когда реализуете rwlock2.
// #define RWLOCK_ADVANCED

// rwlock1: наивная версия. Политика reader-preference (читатели проходят,
// пока писатель не внутри), одна условная переменная, broadcast на каждом
// освобождении (последним читателем или писателем).
// Дефекты: голодание писателя + стадо разбуженных.
struct rwlock1 {
  struct mutex lk;        // защищает все поля ниже
  struct cond   c;         // общая очередь: спят и читатели, и писатели
  int nreaders;            // читателей внутри
  int writer;              // 1, если внутри писатель
  unsigned rd_acquires;    // всего успешных читательских захватов (метрика)
};

void rwlock1_init(struct rwlock1 *rw);
void rwlock1_rdlock(struct rwlock1 *rw);
void rwlock1_rdunlock(struct rwlock1 *rw);
void rwlock1_wrlock(struct rwlock1 *rw);
void rwlock1_wrunlock(struct rwlock1 *rw);

#ifdef RWLOCK_ADVANCED
// rwlock2: писательский приоритет (writer-preference), две условные
// переменные, адресная побудка. Чинит оба дефекта rwlock1.
// Продвинутый уровень: студенты пишут сами (Downey, §4.2).
struct rwlock2 {
  struct mutex lk;
  struct cond   canread;   // здесь спят только читатели
  struct cond   canwrite;  // здесь спят только писатели
  int nreaders;
  int writer;
  int nwriters_waiting;    // писателей "у двери" — ключ к смене политики
  unsigned rd_acquires;
};

void rwlock2_init(struct rwlock2 *rw);
void rwlock2_rdlock(struct rwlock2 *rw);
void rwlock2_rdunlock(struct rwlock2 *rw);
void rwlock2_wrlock(struct rwlock2 *rw);
void rwlock2_wrunlock(struct rwlock2 *rw);
#endif

#endif