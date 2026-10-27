/*
 * 01_race.c — гонка данных: четыре потока увеличивают общий счётчик.
 *
 *   gcc 01_race.c -o 01_race -pthread
 *   ./01_race                    — результат меньше ожидаемого и каждый раз разный
 *
 *   gcc -O1 -g -fsanitize=thread 01_race.c -o 01_race-tsan -pthread
 *   ./01_race-tsan               — отчёт ThreadSanitizer о гонке
 */
#include <pthread.h>
#include <stdio.h>

long count = 0;

void *worker(void *arg) {
    for (int i = 0; i < 1000000; i++)
        count++;                 /* прочитать, прибавить, записать */
    return NULL;
}

int main(void) {
    pthread_t t[4];
    for (int i = 0; i < 4; i++)
        pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < 4; i++)
        pthread_join(t[i], NULL);
    printf("count = %ld (ожидалось 4000000)\n", count);
    return 0;
}
