// runtests — прогнать все тесты лабораторной и подвести итог.
//
//   runtests          - продвинутый уровень (все тесты)
//   runtests basic    - базовый уровень: futextest без тестов 6-7
//
// Тест считается пройденным, если завершился с кодом 0. herdtest
// и wrstarve в основном печатают замеры - их числа смотрите глазами.

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
  // Блок 5: семафор
  { { "prodcons_sem", 0 } },
  { { "sempool", 0 } },
  { { "naivepost", 0 } },
  { { "barriertest", 0 } },
  // Блок 6: rwlock
  { { "rwtest", 0 } },
  { { "wrstarve", 0 } },
};

#define NTESTS (sizeof(tests) / sizeof(tests[0]))

int
main(int argc, char *argv[])
{
  int failed = 0;
  int basic = (argc > 1 && strcmp(argv[1], "basic") == 0);

  if (basic)
    tests[3].argv[1] = "basic";

  for (int i = 0; i < NTESTS; i++) {
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

  printf("\nruntests: %d/%d passed\n", (int)NTESTS - failed, (int)NTESTS);
  exit(failed ? 1 : 0);
}
