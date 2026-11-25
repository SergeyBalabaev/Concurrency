#ifndef FUTEX_H
#define FUTEX_H

// Операции системного вызова futex(uaddr, op, val).
//
// FUTEX_WAIT: заснуть на слове *uaddr, если оно всё ещё равно val.
//   val    - значение, которое вызывающий видел перед входом в ядро.
//   Возврат: 0            - спали и были разбужены (или прерваны);
//            FUTEX_AGAIN  - *uaddr != val, мир уже изменился, сна не было;
//            -1           - плохой адрес (невыровнен / вне памяти процесса).
//   ВНИМАНИЕ: возврат 0 НЕ гарантирует, что событие произошло.
//   FUTEX_WAIT всегда вызывается в цикле с повторной проверкой условия.
//
// FUTEX_WAKE: разбудить не более val потоков, спящих на слове uaddr.
//   Возврат: число реально разбуженных (может быть 0), либо -1.
//
// Дополнительное задание (продвинутый уровень), отдельный системный вызов
//   futex_requeue(uaddr, nwake, uaddr2, nmove, cmpval):
//   если *uaddr == cmpval - разбудить до nwake ожидающих на uaddr, а ещё
//   до nmove перевесить, не будя, в очередь слова uaddr2. Аналог
//   FUTEX_CMP_REQUEUE в Linux. Возврат: разбуженные + перевешенные,
//   FUTEX_AGAIN при несовпадении значения, -1 при плохом адресе.

#define FUTEX_WAIT  0
#define FUTEX_WAKE  1

// Аналог линуксовского EAGAIN (в xv6 нет errno).
#define FUTEX_AGAIN (-2)

// Счётчики для замеров; читаются syscall'ом futexstats(&s).
// В ядре увеличиваются атомарно (__atomic_fetch_add), без блокировок.
struct futexstat {
  uint64 wait_slept;    // FUTEX_WAIT: реально уснули
  uint64 wait_again;    // FUTEX_WAIT: вернулись сразу (значение изменилось)
  uint64 woken;         // потоков разбужено через FUTEX_WAKE (и requeue)
  uint64 join_sleeps;   // засыпаний в sys_thread_join на канале &proc
  uint64 wake_calls;    // FUTEX_WAKE: всего вызовов (в т.ч. никого не разбудивших)
  uint64 requeued;      // потоков перевешено futex_requeue (доп. задание)
};

#endif
