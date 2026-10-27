/*
 * bank.c — переводы между счетами.
 *
 * THREADS потоков делают случайные переводы между ACCOUNTS счетами.
 * Деньги никуда не исчезают и ниоткуда не берутся, поэтому в конце
 * сумма по всем счетам должна остаться прежней, а отрицательных
 * балансов быть не должно.
 *
 * Сборка:   gcc -O2 -g bank.c -o bank -pthread
 * TSan:     gcc -O1 -g -fsanitize=thread bank.c -o bank-tsan -pthread
 */
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define ACCOUNTS  10
#define THREADS   4
#define TRANSFERS 200000      /* переводов на каждый поток */
#define START     1000        /* начальный баланс каждого счёта */

long balance[ACCOUNTS];

/* TODO: синхронизация */
void transfer(int from, int to, long amount) {
    if (balance[from] >= amount) {
        balance[from] -= amount;
        balance[to] += amount;
    }
}

void *worker(void *arg) {
    unsigned seed = (long)arg + 1;
    for (int i = 0; i < TRANSFERS; i++) {
        int from = rand_r(&seed) % ACCOUNTS;
        int to   = rand_r(&seed) % ACCOUNTS;
        if (from == to) continue;
        transfer(from, to, 1 + rand_r(&seed) % 100);
    }
    return NULL;
}

/* Сторож: если программа не уложилась в 10 секунд — значит, зависла. */
void timeout(int sig) {
    (void)sig;
    printf("НЕКОРРЕКТНО: за 10 секунд не закончили — зависли\n");
    fflush(stdout);
    _exit(1);
}

int main(void) {
    signal(SIGALRM, timeout);
    alarm(10);

    for (int i = 0; i < ACCOUNTS; i++) balance[i] = START;

    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);

    long total = 0;
    int negative = 0;
    for (int i = 0; i < ACCOUNTS; i++) {
        total += balance[i];
        if (balance[i] < 0) negative++;
    }
    printf("сумма: %ld (должна быть %d), отрицательных счетов: %d\n",
           total, ACCOUNTS * START, negative);
    if (total == ACCOUNTS * START && negative == 0) {
        printf("корректно\n");
        return 0;
    }
    printf("НЕКОРРЕКТНО\n");
    return 1;
}
