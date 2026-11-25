//
// futextest - функциональные тесты системного вызова futex.
//
// Проверяем контракт, а не реализацию: тесты 1-5 обязаны проходить
// на обоих уровнях; тест 6 (адресное пробуждение WAKE(1)) - только
// на продвинутом, на базовом он честно покажет "разбужены все".
// Тест 7 - futex_requeue (дополнительное задание).
//
//   futextest          - все тесты (продвинутый уровень)
//   futextest basic    - без тестов 6-7 (базовый уровень)
//
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/futex.h"
#include "user/user.h"
#include "user/uthread.h"

volatile int word;               // слово-футекс для большинства тестов

volatile int entered, released;  // счётчики для тестов WAKE(1) и requeue
volatile int word2;              // второе слово для теста requeue

int failed;

void
check(int ok, const char *name)
{
  printf("%s: %s\n", name, ok ? "OK" : "FAIL");
  if(!ok)
    failed = 1;
}

// --- тест 4: ожидающий и пробуждающий -------------------------
void
sleeper(void *arg)
{
  // ждём, пока main не переведёт word из 0 в 1
  int v;
  while((v = word) == 0)
    futex(&word, FUTEX_WAIT, v);
  thread_exit();
}

// --- тест 6: K ожидающих, будим по одному ----------------------
void
one_of_many(void *arg)
{
  __atomic_fetch_add(&entered, 1, __ATOMIC_SEQ_CST);
  int v;
  while((v = word) == 0)
    futex(&word, FUTEX_WAIT, v);
  __atomic_fetch_add(&released, 1, __ATOMIC_SEQ_CST);
  thread_exit();
}

// --- тест 7: ждём на word, нас перевешивают на word2 -----------
void
requeued_waiter(void *arg)
{
  __atomic_fetch_add(&entered, 1, __ATOMIC_SEQ_CST);
  int v;
  while((v = word) == 0)
    futex(&word, FUTEX_WAIT, v);
  __atomic_fetch_add(&released, 1, __ATOMIC_SEQ_CST);
  thread_exit();
}

int
main(int argc, char *argv[])
{
  int r;
  int basic = (argc > 1 && strcmp(argv[1], "basic") == 0);

  // 1. Невыровненный адрес -> ошибка
  r = futex((int*)((char*)&word + 2), FUTEX_WAIT, 0);
  check(r == -1, "unaligned addr rejected");

  // 2. Значение уже изменилось -> FUTEX_AGAIN, без сна
  word = 7;
  r = futex(&word, FUTEX_WAIT, 6);
  check(r == FUTEX_AGAIN, "value mismatch -> FUTEX_AGAIN");

  // 3. WAKE без ожидающих -> 0 разбуженных
  r = futex(&word, FUTEX_WAKE, 100);
  check(r == 0, "wake with no waiters -> 0");

  // 3a. Слово на лениво выделенной странице: sbrk без касания.
  // walkaddr вернёт 0, ядро обязано догрузить страницу, прочитать
  // из неё 0 и вернуть FUTEX_AGAIN (мы передаём заведомо не ноль).
  volatile int *lazy = (volatile int*)sbrk(PGSIZE);
  r = futex(lazy, FUTEX_WAIT, 12345);
  check(r == FUTEX_AGAIN, "lazily allocated page");

  // 4. Настоящее ожидание: поток спит, пока main не разбудит
  word = 0;
  int tid = thread_create(sleeper, 0);
  pause(2);                      // дать потоку уснуть
  word = 1;                      // сначала изменить слово...
  r = futex(&word, FUTEX_WAKE, 1);   // ...потом будить
  check(r >= 0, "wake sleeping thread");
  check(thread_join_t(tid) == tid, "sleeper joined");

  // 5. Порядок "изменить, потом разбудить" в тесте 4 обязателен;
  // а здесь - гонка наоборот: будим ДО того, как поток уснул.
  // Потерянного пробуждения быть не должно: поток обязан увидеть
  // новое значение при атомарной проверке в ядре.
  word = 0;
  tid = thread_create(sleeper, 0);
  word = 1;
  futex(&word, FUTEX_WAKE, 1);   // скорее всего, уйдёт в пустоту
  check(thread_join_t(tid) == tid, "no lost wakeup");

  if(basic){
    printf(failed ? "futextest: FAILED\n" : "futextest: basic tests passed\n");
    exit(failed);
  }

  // 6. Адресное пробуждение: K ожидающих, WAKE(1) освобождает
  // ровно одного (продвинутый уровень; базовый разбудит всех -
  // это ожидаемое расхождение, см. методичку)
  #define K 4
  int tids[K];
  word = 0;
  entered = released = 0;
  for(int i = 0; i < K; i++)
    tids[i] = thread_create(one_of_many, 0);
  while(entered < K)
    pause(1);
  pause(3);                      // дать всем уснуть в futex

  word = 1;                      // разрешить выход...
  futex(&word, FUTEX_WAKE, 1);   // ...но разбудить только одного
  pause(3);
  printf("woken after WAKE(1): %d of %d (advanced: 1, basic: %d)\n",
         released, K, K);
  check(released == 1, "WAKE(1) wakes exactly one");

  futex(&word, FUTEX_WAKE, K);   // выпустить остальных
  for(int i = 0; i < K; i++)
    thread_join_t(tids[i]);
  check(released == K, "WAKE(n) released the rest");

  // 7. requeue: K ожидающих на word; разбудить 1, остальных перевесить
  // на word2. Они должны остаться спать, пока не разбудят word2.
  word = 0;
  word2 = 0;
  entered = released = 0;
  for(int i = 0; i < K; i++)
    tids[i] = thread_create(requeued_waiter, 0);
  while(entered < K)
    pause(1);
  pause(3);

  r = futex_requeue(&word, 1, &word2, K, 1);       // word != 1
  check(r == FUTEX_AGAIN, "requeue: cmpval mismatch -> FUTEX_AGAIN");

  word = 1;                       // разрешить выход...
  r = futex_requeue(&word, 1, &word2, K, 1);
  pause(3);
  check(r == K && released == 1, "requeue: one woken, rest moved");

  r = futex(&word, FUTEX_WAKE, K);  // на word больше никто не спит
  pause(3);
  check(r == 0 && released == 1, "requeue: moved threads left word");

  r = futex(&word2, FUTEX_WAKE, K); // а на word2 - спят
  for(int i = 0; i < K; i++)
    thread_join_t(tids[i]);
  check(r == K - 1 && released == K, "requeue: woken via word2");

  printf(failed ? "futextest: FAILED\n" : "futextest: all tests passed\n");
  exit(failed);
}