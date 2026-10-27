/*
 * deadlock.c — минимальная взаимная блокировка.
 *
 * Два потока переводят деньги между счетами a и b.
 * Каждый счёт защищён своим мьютексом: a — мьютексом A, b — мьютексом B.
 *
 *   ./deadlock good   — оба потока берут мьютексы в порядке A, B
 *   ./deadlock bad    — второй поток берёт их в порядке B, A → зависание
 *   ./deadlock seq    — порядок плохой, но потоки работают по очереди:
 *                       программа не виснет, а TSan всё равно находит ошибку
 *                       (make tsan; ./deadlock-tsan seq)
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define ITERS 1000000

pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;
long a = 0, b = 0;

/* Перевод из a в b: берём A, потом B. */
void *transfer_ab(void *name) {
    for (int i = 0; i < ITERS; i++) {
        pthread_mutex_lock(&A);
        pthread_mutex_lock(&B);
        a--;
        b++;
        pthread_mutex_unlock(&B);
        pthread_mutex_unlock(&A);
        if (i % 100000 == 0) printf("%s: %d\n", (char *)name, i);
    }
    return NULL;
}

/* Перевод из b в a. ПРАВИЛЬНО: тот же порядок — A, потом B. */
void *transfer_ba_good(void *name) {
    for (int i = 0; i < ITERS; i++) {
        pthread_mutex_lock(&A);
        pthread_mutex_lock(&B);
        b--;
        a++;
        pthread_mutex_unlock(&B);
        pthread_mutex_unlock(&A);
        if (i % 100000 == 0) printf("%s: %d\n", (char *)name, i);
    }
    return NULL;
}

/* Перевод из b в a. НЕПРАВИЛЬНО: обратный порядок — B, потом A. */
void *transfer_ba_bad(void *name) {
    for (int i = 0; i < ITERS; i++) {
        pthread_mutex_lock(&B);
        pthread_mutex_lock(&A);
        b--;
        a++;
        pthread_mutex_unlock(&A);
        pthread_mutex_unlock(&B);
        if (i % 100000 == 0) printf("%s: %d\n", (char *)name, i);
    }
    return NULL;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);   /* печатать сразу, без буфера */
    pthread_t t1, t2;

    if (argc == 2 && strcmp(argv[1], "good") == 0) {
        pthread_create(&t1, NULL, transfer_ab, "поток 1");
        pthread_create(&t2, NULL, transfer_ba_good, "поток 2");
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
    } else if (argc == 2 && strcmp(argv[1], "bad") == 0) {
        pthread_create(&t1, NULL, transfer_ab, "поток 1");
        pthread_create(&t2, NULL, transfer_ba_bad, "поток 2");
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
    } else if (argc == 2 && strcmp(argv[1], "seq") == 0) {
        pthread_create(&t1, NULL, transfer_ab, "поток 1");
        pthread_join(t1, NULL);         /* второй стартует после первого */
        pthread_create(&t2, NULL, transfer_ba_bad, "поток 2");
        pthread_join(t2, NULL);
    } else {
        printf("использование: %s good|bad|seq\n", argv[0]);
        return 1;
    }

    printf("готово: a = %ld, b = %ld\n", a, b);
    return 0;
}
