// malloctest — память в многопоточном процессе (БДЗ-3, шаг 1).
//
//   malloctest sbrk   - потоки одновременно растят кучу через sbrk
//   malloctest lazy   - поток делает sbrklazy, main трогает эту память
//   malloctest        - потоки одновременно вызывают malloc/free
//
// Без правок шага 1 первый сценарий роняет ядро (panic: mappages: remap),
// второй убивает main ошибкой страницы, третий портит кучу.

#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"
#include "user/uthread.h"

#define NT 4

// ---------- сценарий sbrk ----------
#define NPAGES 32
static char *pages[NT][NPAGES];

void
sbrk_worker(void *arg)
{
  int id = (int)(uint64)arg;
  for(int i = 0; i < NPAGES; i++){
    char *p = sbrk(PGSIZE);
    if(p == (char *)-1){
      printf("thread %d: sbrk failed\n", id);
      thread_exit();
    }
    memset(p, 'A' + id, PGSIZE);         // страница - только наша
    pages[id][i] = p;
  }
  thread_exit();
}

int
test_sbrk(void)
{
  int tids[NT];
  for(int i = 0; i < NT; i++)
    tids[i] = thread_create(sbrk_worker, (void *)(uint64)i);
  for(int i = 0; i < NT; i++)
    thread_join_t(tids[i]);

  // Проверка: каждая страница по-прежнему заполнена буквой своего потока.
  // Если два потока получили один и тот же адрес, один затёр другого.
  int bad = 0;
  for(int t = 0; t < NT; t++)
    for(int i = 0; i < NPAGES; i++)
      for(int k = 0; k < PGSIZE; k += 512)
        if(pages[t][i][k] != 'A' + t){
          bad++;
          break;
        }
  printf("sbrk: %d threads x %d pages, overlapped pages: %d\n", NT, NPAGES, bad);
  return bad == 0;
}

// ---------- сценарий lazy ----------
static char *volatile lazy_mem;

void
lazy_worker(void *arg)
{
  lazy_mem = sbrklazy(4 * PGSIZE);     // память обещана, но не выделена
  thread_exit();
}

int
test_lazy(void)
{
  int tid = thread_create(lazy_worker, 0);
  thread_join_t(tid);
  printf("lazy: thread got %p, main touches it...\n", lazy_mem);
  for(int i = 0; i < 4 * PGSIZE; i += PGSIZE)
    lazy_mem[i] = 1;                   // ошибка страницы обрабатывается в main
  printf("lazy: main survived\n");
  return 1;
}

// ---------- сценарий malloc ----------
#define ROUNDS 5000
#define LIVE 8
static int corrupt;
static int finished;                   // дошли до конца (не убиты ошибкой страницы)

void
malloc_worker(void *arg)
{
  int id = (int)(uint64)arg;
  char *live[LIVE] = {0};
  int size[LIVE] = {0};
  unsigned seed = 12345 + id;

  for(int r = 0; r < ROUNDS; r++){
    int j = r % LIVE;
    if(live[j]){
      for(int k = 0; k < size[j]; k++)
        if(live[j][k] != (char)(id + j)){
          __atomic_fetch_add(&corrupt, 1, __ATOMIC_RELAXED);
          break;
        }
      free(live[j]);
    }
    seed = seed * 1103515245 + 12345;
    size[j] = 16 + (seed >> 16) % 3000;
    live[j] = malloc(size[j]);
    if(live[j] == 0){
      printf("thread %d: malloc failed\n", id);
      thread_exit();
    }
    memset(live[j], id + j, size[j]);
  }
  for(int j = 0; j < LIVE; j++)
    if(live[j])
      free(live[j]);
  __atomic_fetch_add(&finished, 1, __ATOMIC_SEQ_CST);
  thread_exit();
}

int
test_malloc(void)
{
  int tids[NT];
  for(int i = 0; i < NT; i++)
    tids[i] = thread_create(malloc_worker, (void *)(uint64)i);
  for(int i = 0; i < NT; i++)
    thread_join_t(tids[i]);
  printf("malloc: %d threads x %d rounds, finished %d, corrupted blocks: %d\n",
         NT, ROUNDS, finished, corrupt);
  return corrupt == 0 && finished == NT;
}

int
main(int argc, char *argv[])
{
  int ok;
  if(argc > 1 && strcmp(argv[1], "sbrk") == 0)
    ok = test_sbrk();
  else if(argc > 1 && strcmp(argv[1], "lazy") == 0)
    ok = test_lazy();
  else
    ok = test_malloc();
  printf("malloctest: %s\n", ok ? "OK" : "FAIL");
  exit(ok ? 0 : 1);
}
