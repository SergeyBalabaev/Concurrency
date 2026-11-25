//
// futex - очередь ожидания, привязанная к слову памяти пользователя,
// с атомарной (относительно FUTEX_WAKE) проверкой значения.
//
// Ключ футекса - ФИЗИЧЕСКИЙ адрес слова (как shared-futex в Linux):
// для потоков одного процесса он совпадает автоматически (общая
// таблица страниц) и останется корректным для будущей разделяемой
// памяти между процессами.
//
// Два уровня реализации, переключаются макросом FUTEX_ADVANCED:
//
//   Базовый:     один глобальный spinlock + штатные sleep/wakeup,
//                канал сна = физический адрес слова. Просто, но
//                wakeup() будит ВСЕХ на канале - аргумент n
//                операции FUTEX_WAKE невыразим.
//
//   Продвинутый: хеш-таблица бакетов, spinlock на бакет, явная
//                FIFO-очередь ожидающих. Пробуждаем ровно n потоков,
//                переводя их в RUNNABLE вручную, минуя wakeup().
//

#define FUTEX_ADVANCED   // закомментировать для базового уровня

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "futex.h"

// Счётчики для замеров (читаются syscall'ом futexstats). Каждый
// счётчик - отдельное 64-битное слово, увеличиваем атомарно: общая
// блокировка на счётчики сериализовала бы все futex-операции
// системы и обесценила бы бакеты продвинутого уровня.
struct futexstat futexstat;
#define STAT_INC(field) __atomic_fetch_add(&futexstat.field, 1, __ATOMIC_SEQ_CST)

// ---------------------------------------------------------------
// Ключ: uaddr -> физический адрес слова.
//
// Требования к адресу:
//  - выравнивание на 4 байта; приятное следствие - слово не может
//    пересечь границу страницы, один walkaddr описывает его целиком;
//  - страница должна быть отображена с PTE_U (проверяет walkaddr).
//
// Ленивая аллокация: вполне легальный адрес (слово на куче, к
// которому ещё не прикасались) может быть не отображён - тогда
// walkaddr вернёт 0. Это не ошибка пользователя: догружаем страницу
// через vmfault, как это делают copyin/copyout. Важно: догрузка
// происходит ЗДЕСЬ, до захвата спин-блокировки футекса - под
// спин-блокировкой выделять страницы нельзя.
//
// Возврат: физический адрес слова или 0 при ошибке.
static uint64
futex_key(uint64 uaddr)
{
  struct proc *p = myproc();
  uint64 pa;

  if(uaddr & 3)                    // выравнивание на 4
    return 0;

  pa = walkaddr(p->pagetable, uaddr);
  if(pa == 0)
    pa = vmfault(p->pagetable, uaddr, 0);   // лениво выделенная страница?
  if(pa == 0)
    return 0;

  // walkaddr/vmfault возвращают базу страницы - добавим смещение
  return pa + (uaddr & (PGSIZE - 1));
}

// Прочитать слово по физическому адресу. Ядро xv6 отображает всю
// физическую память один в один, поэтому достаточно разыменования -
// в отличие от copyin, здесь не нужны обходы таблиц страниц, и это
// безопасно делать под спин-блокировкой. Страница никуда не денется:
// свопа и миграции страниц в xv6 нет.
static inline uint32
futex_load(uint64 pa)
{
  return *(volatile uint32 *)pa;
}

#ifndef FUTEX_ADVANCED

// ===============================================================
// БАЗОВЫЙ УРОВЕНЬ: глобальный лок + sleep/wakeup
// ===============================================================

struct spinlock futex_lock;

void
futexinit(void)
{
  initlock(&futex_lock, "futex");
}

// Проверка значения и засыпание - под ТЕМ ЖЕ локом, который берёт
// futex_wake. Это закрывает окно потерянного пробуждения: если
// wake случился до нашего acquire - мы увидим уже новое значение
// и вернём FUTEX_AGAIN; если после - wakeup() не сможет пройти,
// пока sleep() не поставит нас спать (sleep отпускает futex_lock
// только после захвата p->lock, который нужен и wakeup'у).
int
futex_wait(uint64 uaddr, uint32 val)
{
  uint64 pa = futex_key(uaddr);
  if(pa == 0)
    return -1;

  acquire(&futex_lock);

  if(futex_load(pa) != val){
    release(&futex_lock);
    STAT_INC(wait_again);
    return FUTEX_AGAIN;
  }

  STAT_INC(wait_slept);
  sleep((void*)pa, &futex_lock);   // канал = физический адрес слова

  release(&futex_lock);
  return 0;
}

// Ограничение базового уровня: wakeup() будит всех спящих на канале,
// поэтому n игнорируется - FUTEX_WAKE(uaddr, 1) невыразим. Сколько
// потоков разбужено, wakeup тоже не сообщает - возвращаем 0.
int
futex_wake(uint64 uaddr, int n)
{
  uint64 pa = futex_key(uaddr);
  if(pa == 0)
    return -1;

  STAT_INC(wake_calls);

  acquire(&futex_lock);
  wakeup((void*)pa);
  release(&futex_lock);
  return 0;
}

// requeue на базовом уровне невыразим: у sleep/wakeup нет очередей.
int
futex_requeue(uint64 uaddr, int nwake, uint64 uaddr2, int nmove, uint32 cmpval)
{
  return -1;
}

#else  // FUTEX_ADVANCED

// ===============================================================
// ПРОДВИНУТЫЙ УРОВЕНЬ: хеш-таблица очередей
// ===============================================================
//
// Как в Linux: массив бакетов, каждый - спин-блокировка плюс список
// ожидающих. В один бакет попадают РАЗНЫЕ футексы (коллизии хеша),
// поэтому при пробуждении элементы фильтруются точным сравнением
// ключа. Элемент очереди не выделяется динамически: поля futex_key
// и futex_next встроены прямо в struct proc - поток может ждать
// максимум на одном футексе, а время жизни записи proc заведомо
// покрывает время ожидания (тот же приём, что futex_q на стеке
// ядра в Linux).
//
// Порядок захвата блокировок: bucket->lock, затем p->lock -
// одинаковый и в wait, и в wake, иначе deadlock.

#define NFUTEXBUCKET 16          // степень двойки

struct futex_bucket {
  struct spinlock lock;
  struct proc *head;             // FIFO: будим в порядке постановки
  struct proc *tail;
};

static struct futex_bucket futextab[NFUTEXBUCKET];

static struct futex_bucket*
futex_bucket(uint64 pa)
{
  // младшие 2 бита нулевые (выравнивание) - отбрасываем;
  // умножение на нечётную константу перемешивает старшие биты
  // в младшие (хеш Кнута), чтобы соседние слова не толпились
  // в соседних бакетах
  return &futextab[((pa >> 2) * 2654435761UL) % NFUTEXBUCKET];
}

void
futexinit(void)
{
  for(int i = 0; i < NFUTEXBUCKET; i++){
    initlock(&futextab[i].lock, "futex");
    futextab[i].head = futextab[i].tail = 0;
  }
}

// Поставить p в хвост очереди бакета. Вызывается под b->lock.
static void
futex_enqueue(struct futex_bucket *b, struct proc *p, uint64 pa)
{
  p->futex_key = pa;
  p->futex_next = 0;
  if(b->tail)
    b->tail->futex_next = p;
  else
    b->head = p;
  b->tail = p;
}

// Вынуть элемент *link из очереди; prev - предыдущий элемент
// (0, если *link - голова). Вызывается под b->lock. После вызова
// *link указывает на следующий элемент - удобно для обхода.
static struct proc*
futex_unlink(struct futex_bucket *b, struct proc **link, struct proc *prev)
{
  struct proc *p = *link;
  *link = p->futex_next;
  if(b->tail == p)
    b->tail = prev;
  p->futex_next = 0;
  return p;
}

// Убрать p из очереди бакета, если он там есть (поиск по списку).
// Нужен только ожидающему, которого разбудили мимо futex_wake
// (kill); после обычного пробуждения p уже вынут и это no-op.
// Вызывается под b->lock.
static void
futex_unqueue(struct futex_bucket *b, struct proc *p)
{
  struct proc **link = &b->head, *prev = 0;

  for(; *link; prev = *link, link = &(*link)->futex_next){
    if(*link == p){
      futex_unlink(b, link, prev);
      return;
    }
  }
}

// Перевести вынутый из очереди поток в RUNNABLE. Возвращает 1, если
// поток действительно спал. Вызывается под b->lock.
static int
futex_wakeone(struct proc *p)
{
  int woke = 0;

  acquire(&p->lock);
  if(p->state == SLEEPING){
    // то самое "адресное" пробуждение: RUNNABLE вручную,
    // без обхода всей таблицы процессов, как в wakeup()
    p->state = RUNNABLE;
    woke = 1;
    STAT_INC(woken);
  }
  // state != SLEEPING: поток уже разбужен kkill'ом и ждёт
  // b->lock, чтобы вынуть себя из очереди; мы сделали это
  // за него, будить второй раз нечего
  release(&p->lock);
  return woke;
}

int
futex_wait(uint64 uaddr, uint32 val)
{
  struct proc *p = myproc();
  struct futex_bucket *b;
  uint64 pa, key;

  pa = futex_key(uaddr);
  if(pa == 0)
    return -1;

  b = futex_bucket(pa);
  acquire(&b->lock);

  // Атомарная (относительно futex_wake по этому бакету) проверка.
  if(futex_load(pa) != val){
    release(&b->lock);
    STAT_INC(wait_again);
    return FUTEX_AGAIN;
  }

  // Встать в хвост очереди - ЕЩЁ ДО того, как отпустим b->lock.
  // Любой wake, пришедший после нашего release, нас уже увидит.
  futex_enqueue(b, p, pa);
  STAT_INC(wait_slept);

  // Дальше - развёрнутый sleep() (см. proc.c): мы не можем позвать
  // sleep(chan, &b->lock), потому что просыпаться будем не через
  // wakeup(chan), а адресно. Протокол тот же:
  //   1) захватить p->lock;
  //   2) только потом отпустить b->lock.
  // Пробуждающий обязан держать b->lock, чтобы вынуть нас из
  // очереди, и p->lock, чтобы сменить состояние, - значит, между
  // нашими шагами 1-2 и до sched() он вклиниться не может:
  // окно потерянного пробуждения закрыто.
  acquire(&p->lock);
  release(&b->lock);

  p->chan = (void*)pa;           // для procdump; матчинг идёт по очереди
  p->state = SLEEPING;

  sched();

  p->chan = 0;
  release(&p->lock);

  // Нас могли разбудить МИМО очереди: kkill() переводит SLEEPING
  // в RUNNABLE, не зная ни о каких футексах. Если мы всё ещё в
  // очереди - вынем себя сами; после futex_wake это no-op.
  //
  // Цикл нужен только из-за futex_requeue: пока мы спали, нас могли
  // перевесить в очередь другого слова, то есть в другой бакет.
  // Ключ меняется только под блокировкой бакета, поэтому сверяем
  // его повторно уже под ней.
  for(;;){
    key = __atomic_load_n(&p->futex_key, __ATOMIC_RELAXED);
    b = futex_bucket(key);
    acquire(&b->lock);
    if(p->futex_key == key){
      futex_unqueue(b, p);
      release(&b->lock);
      break;
    }
    release(&b->lock);
  }

  return 0;
}

int
futex_wake(uint64 uaddr, int n)
{
  struct futex_bucket *b;
  struct proc **link, *prev, *pp;
  uint64 pa;
  int nwoken = 0;

  pa = futex_key(uaddr);
  if(pa == 0)
    return -1;

  STAT_INC(wake_calls);

  b = futex_bucket(pa);
  acquire(&b->lock);

  // Пока мы держим b->lock, ни один поток из очереди не может
  // завершить futex_wait (его последний шаг - unqueue под b->lock),
  // а значит, не может ни выйти, ни быть переиспользован его слот
  // proc. Поэтому разыменовывать элементы очереди безопасно.
  //
  // Обход через указатель на ссылку (link): подходящий элемент
  // вынимаем на месте, за один проход, без повторного поиска.
  link = &b->head;
  prev = 0;
  while(*link && nwoken < n){
    pp = *link;
    if(pp->futex_key != pa){     // чужой футекс из того же бакета
      prev = pp;
      link = &pp->futex_next;
      continue;
    }
    futex_unlink(b, link, prev); // *link теперь - следующий
    nwoken += futex_wakeone(pp);
  }

  release(&b->lock);
  return nwoken;
}

// ---------------------------------------------------------------
// Дополнительное задание: futex_requeue (аналог FUTEX_CMP_REQUEUE).
//
// Разбудить до nwake потоков, ждущих на uaddr, а ещё до nmove -
// перевесить в очередь слова uaddr2, не будя. Применение - broadcast
// условной переменной: будим одного, остальных переносим в очередь
// мьютекса, и они просыпаются по одному при каждом unlock.
//
// cmpval закрывает ту же гонку, что и val в FUTEX_WAIT: если слово
// uaddr уже изменилось (пришёл новый сигнал), вызывающий должен
// перечитать состояние и повторить.
int
futex_requeue(uint64 uaddr, int nwake, uint64 uaddr2, int nmove, uint32 cmpval)
{
  struct futex_bucket *b1, *b2;
  struct proc **link, *prev, *pp;
  uint64 pa1, pa2;
  int nwoken = 0, nmoved = 0;

  // Оба ключа - до захвата блокировок (vmfault под спинлоком нельзя).
  pa1 = futex_key(uaddr);
  pa2 = futex_key(uaddr2);
  if(pa1 == 0 || pa2 == 0)
    return -1;

  b1 = futex_bucket(pa1);
  b2 = futex_bucket(pa2);

  // Две блокировки одного класса - берём в порядке адресов, иначе
  // два встречных requeue (A->B и B->A) взаимно заблокируются.
  if(b1 == b2){
    acquire(&b1->lock);
  } else if(b1 < b2){
    acquire(&b1->lock);
    acquire(&b2->lock);
  } else {
    acquire(&b2->lock);
    acquire(&b1->lock);
  }

  if(futex_load(pa1) != cmpval){
    nwoken = FUTEX_AGAIN;
    goto out;
  }

  link = &b1->head;
  prev = 0;
  while(*link && (nwoken < nwake || nmoved < nmove)){
    pp = *link;
    if(pp->futex_key != pa1){
      prev = pp;
      link = &pp->futex_next;
      continue;
    }
    futex_unlink(b1, link, prev);
    if(nwoken < nwake){
      nwoken += futex_wakeone(pp);
    } else {
      // Перевесить, не будя: поток продолжает спать, но теперь его
      // разбудит futex_wake(uaddr2). Ключ меняем под обеими
      // блокировками - см. цикл самоизвлечения в futex_wait.
      futex_enqueue(b2, pp, pa2);
      nmoved++;
      STAT_INC(requeued);
    }
  }

out:
  release(&b1->lock);
  if(b2 != b1)
    release(&b2->lock);
  return nwoken < 0 ? nwoken : nwoken + nmoved;
}

#endif // FUTEX_ADVANCED
