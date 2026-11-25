// runtests — прогнать тесты и подвести итог.
//
//   runtests          - регрессия БДЗ-1/2 и обязательная часть БДЗ-3
//   runtests all      - то же плюс дополнительные задания БДЗ-3
//
// Тест считается пройденным, если завершился с кодом 0. Эксперименты
// (poolbench, promisestat, pooldeadlock 2) в основном печатают замеры -
// их числа смотрите глазами; pooldeadlock 2 сюда не входит: он
// нарочно завершается с кодом 1.

#include "kernel/types.h"
#include "user/user.h"

struct test {
  char *argv[3];
};

static struct test tests[] = {
  // ЛР1: потоки
  { { "threadtest", 0 } },
  { { "racetest", 0 } },
  { { "benchtest", 0 } },
  // Блок 1-2: futex и join
  { { "futextest", 0 } },          // argv[1] = "basic" подставляется ниже
  { { "herdtest", "old", 0 } },
  { { "herdtest", "futex", 0 } },
  // Блок 3: мьютекс
  { { "mutextest", 0 } },
  // Блок 4: условная переменная
  { { "prodcons", 0 } },
  { { "herdbcast", 0 } },
  // БДЗ-3, шаг 1: память
  { { "malloctest", "sbrk", 0 } },
  { { "malloctest", "lazy", 0 } },
  { { "malloctest", 0 } },
  // БДЗ-3, шаги 2-5
  { { "queuetest", 0 } },
  { { "pooltest", 0 } },
  { { "wgtest", 0 } },
  { { "promisetest", 0 } },
  { { "pooldeadlock", "1", 0 } },
  // БДЗ-3, дополнительные задания (только runtests all)
  { { "oncetest", 0 } },
  { { "brokenpromise", 0 } },
  { { "thentest", 0 } },
};

#define NTESTS (sizeof(tests) / sizeof(tests[0]))
#define NBONUS 3                  // последние три - дополнительные задания

int
main(int argc, char *argv[])
{
  int failed = 0;
  int all = (argc > 1 && strcmp(argv[1], "all") == 0);
  int ntests = all ? NTESTS : NTESTS - NBONUS;

  for (int i = 0; i < ntests; i++) {
    char **av = tests[i].argv;
    printf("\n===== %s %s =====\n", av[0], av[1] ? av[1] : "");

    int pid = fork();
    if (pid < 0) {
      printf("runtests: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      exec(av[0], av);
      // сюда попадаем, только если exec не нашёл программу
      printf("runtests: exec %s failed (нет в UPROGS?)\n", av[0]);
      exit(127);
    }

    int status;
    wait(&status);
    if (status == 0) {
      printf("===== %s: OK =====\n", av[0]);
    } else {
      printf("===== %s: FAIL (status %d) =====\n", av[0], status);
      failed++;
    }
  }

  printf("\nruntests: %d/%d passed\n", ntests - failed, ntests);
  exit(failed ? 1 : 0);
}
