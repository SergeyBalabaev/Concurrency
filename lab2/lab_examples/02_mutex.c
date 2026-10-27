/*
 * 02_mutex.c — тот же счётчик, но под мьютексом.
 *
 *   gcc 02_mutex.c -o 02_mutex -pthread
 *   ./02_mutex                   — всегда 4000000
 *
 * Под капотом мьютекса — системный вызов futex:
 *   strace -f -e trace=futex ./02_mutex
 */
#include <pthread.h>
#include <stdio.h>

long count = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&m);
        count++;                 /* критическая секция */
        pthread_mutex_unlock(&m);
    }
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
