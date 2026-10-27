#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/futex.h"
#include "user/user.h"
#include "user/uthread.h"

// Библиотека потоков из лабораторной работы 1.
//
// Ядро в этой заготовке уже умеет механизм ctid: clone принимает
// четвёртым аргументом адрес слова, в которое ядро при выходе потока
// запишет 0 и сделает FUTEX_WAKE (раздел 4.9). Пока ниже передаётся 0 -
// "не использовать", и join работает по-старому, через системный
// вызов thread_join на общем канале.
//
// Блок 2: переведите join на futex (раздел 10.3, шаг 2):
//   - добавьте в таблицу поле volatile int alive;
//   - в thread_create записывайте alive = 1 ДО clone и передавайте
//     его адрес четвёртым аргументом;
//   - в thread_join_t сначала ждите обнуления alive в цикле с
//     FUTEX_WAIT, затем вызывайте thread_join(tid) для уборки ZOMBIE,
//     затем освобождайте стек.

static struct {
  int tid;
  void *stack;
} threads[MAX_THREADS];

int
thread_create(void (*fn)(void *), void *arg)
{
  int i;
  for(i = 0; i < MAX_THREADS; i++)
    if(threads[i].tid == 0)
      break;
  if(i == MAX_THREADS)
    return -1;

  void *stack = malloc(PGSIZE);
  if(stack == 0)
    return -1;

  int tid = clone(fn, arg, stack, 0);   // Блок 2: здесь будет &alive
  if(tid < 0){
    free(stack);
    return -1;
  }
  threads[i].tid = tid;
  threads[i].stack = stack;
  return tid;
}

int
thread_join_t(int tid)
{
  int i;
  if(tid <= 0)
    return -1;
  for(i = 0; i < MAX_THREADS; i++)
    if(threads[i].tid == tid)
      break;
  if(i == MAX_THREADS)
    return -1;

  // Блок 2: перед thread_join дождитесь обнуления alive через futex.
  int r = thread_join(tid);
  if(r < 0)
    return r;

  free(threads[i].stack);
  threads[i].stack = 0;
  threads[i].tid = 0;
  return r;
}

void
thread_exit(void)
{
  exit(0);
}
