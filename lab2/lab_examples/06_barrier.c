/*
 * 06_barrier.c — барьер: работа идёт этапами, и следующий этап
 * начинается, только когда предыдущий закончили все.
 *
 * Три потока проходят три этапа, каждый работает случайное время.
 *
 *   gcc 06_barrier.c -o 06_barrier -pthread
 *   ./06_barrier
 *
 * Закомментируйте pthread_barrier_wait — этапы перемешаются.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THREADS 3
#define STAGES  3

pthread_barrier_t barrier;

void *worker(void *arg) {
    long id = (long)arg;
    unsigned seed = id + 1;
    for (int stage = 1; stage <= STAGES; stage++) {
        usleep(rand_r(&seed) % 500000);      /* работаем до 0,5 с */
        printf("поток %ld: закончил этап %d\n", id, stage);
        pthread_barrier_wait(&barrier);      /* ждём остальных */
    }
    return NULL;
}

int main(void) {
    pthread_barrier_init(&barrier, NULL, THREADS);
    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    pthread_barrier_destroy(&barrier);
    return 0;
}
