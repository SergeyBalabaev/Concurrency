/*
 * philo.c — обедающие философы. N философов за круглым столом, между
 * соседями лежит одна вилка. Чтобы поесть, нужны обе: левая и правая.
 *
 *   ./philo deadlock  — каждый берёт левую вилку, потом правую
 *   ./philo livelock  — взял левую; если правая занята — вежливо кладёт
 *                       левую и пробует снова
 *   ./philo order     — каждый сначала берёт вилку с меньшим номером
 *   ./philo waiter    — официант пускает за стол не больше N-1 философов
 *
 * Каждый философ должен поесть MEALS раз. Раз в секунду main печатает,
 * сколько всего было обедов и неудачных попыток взять вторую вилку.
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define N     5
#define MEALS 200

pthread_mutex_t forks[N];
sem_t waiter;
pthread_barrier_t barrier;

/* Счётчики атомарные: их одновременно меняют философы и читает main. */
atomic_long meals;      /* всего обедов                              */
atomic_long fails;      /* неудачных попыток взять вторую вилку      */
atomic_int  finished;   /* сколько философов уже наелись             */

int left(int id)  { return id; }
int right(int id) { return (id + 1) % N; }

void eat(void)   { meals++; usleep(1000); }
void think(void) { usleep(1000); }

/* 1. Левая, потом правая. Все взяли по левой — и ждут вечно. */
void *philo_deadlock(void *arg) {
    int id = (long)arg;
    for (int i = 0; i < MEALS; i++) {
        pthread_mutex_lock(&forks[left(id)]);
        usleep(1000);                   /* тем временем соседи берут свои левые */
        pthread_mutex_lock(&forks[right(id)]);
        eat();
        pthread_mutex_unlock(&forks[right(id)]);
        pthread_mutex_unlock(&forks[left(id)]);
        think();
    }
    finished++;
    return NULL;
}

/* 2. Вежливые философы: правая занята — кладём левую и пробуем снова.
 *    Барьер заставляет всех действовать строго в ногу: так livelock
 *    воспроизводится каждый раз, а не изредка, как в жизни. */
void *philo_livelock(void *arg) {
    int id = (long)arg;
    int ate = 0;
    while (ate < MEALS) {
        pthread_barrier_wait(&barrier);         /* все тянутся к левой  */
        pthread_mutex_lock(&forks[left(id)]);

        pthread_barrier_wait(&barrier);         /* все тянутся к правой */
        int got = pthread_mutex_trylock(&forks[right(id)]) == 0;

        pthread_barrier_wait(&barrier);         /* все узнали результат */
        if (got) {
            eat();
            ate++;
            pthread_mutex_unlock(&forks[right(id)]);
        } else {
            fails++;
        }
        pthread_mutex_unlock(&forks[left(id)]); /* вежливо кладём левую */
    }
    finished++;
    return NULL;
}

/* 3. Порядок: сначала вилка с меньшим номером. Цикла ожидания нет. */
void *philo_order(void *arg) {
    int id = (long)arg;
    int first  = left(id) < right(id) ? left(id)  : right(id);
    int second = left(id) < right(id) ? right(id) : left(id);
    for (int i = 0; i < MEALS; i++) {
        pthread_mutex_lock(&forks[first]);
        usleep(1000);
        pthread_mutex_lock(&forks[second]);
        eat();
        pthread_mutex_unlock(&forks[second]);
        pthread_mutex_unlock(&forks[first]);
        think();
    }
    finished++;
    return NULL;
}

/* 4. Официант: за столом не больше N-1, значит, кому-то хватит двух вилок. */
void *philo_waiter(void *arg) {
    int id = (long)arg;
    for (int i = 0; i < MEALS; i++) {
        sem_wait(&waiter);
        pthread_mutex_lock(&forks[left(id)]);
        usleep(1000);
        pthread_mutex_lock(&forks[right(id)]);
        eat();
        pthread_mutex_unlock(&forks[right(id)]);
        pthread_mutex_unlock(&forks[left(id)]);
        sem_post(&waiter);
        think();
    }
    finished++;
    return NULL;
}

int main(int argc, char **argv) {
    void *(*philo)(void *) = NULL;      /* какую функцию запустят философы */
    if (argc == 2 && strcmp(argv[1], "deadlock") == 0) philo = philo_deadlock;
    if (argc == 2 && strcmp(argv[1], "livelock") == 0) philo = philo_livelock;
    if (argc == 2 && strcmp(argv[1], "order")    == 0) philo = philo_order;
    if (argc == 2 && strcmp(argv[1], "waiter")   == 0) philo = philo_waiter;
    if (philo == NULL) {
        printf("использование: %s deadlock|livelock|order|waiter\n", argv[0]);
        return 1;
    }

    for (int i = 0; i < N; i++) pthread_mutex_init(&forks[i], NULL);
    sem_init(&waiter, 0, N - 1);
    pthread_barrier_init(&barrier, NULL, N);

    pthread_t t[N];
    for (long i = 0; i < N; i++)
        pthread_create(&t[i], NULL, philo, (void *)i);

    /* Раз в секунду смотрим на прогресс. Три секунды без обедов — зависли. */
    long last = 0;
    int idle = 0;
    while (finished < N) {
        sleep(1);
        long m = meals, f = fails;
        printf("обедов: %ld, неудачных попыток: %ld\n", m, f);
        idle = (m == last) ? idle + 1 : 0;
        last = m;
        if (idle == 3) {
            if (f > 0) printf("попытки идут, а обедов нет: LIVELOCK\n");
            else       printf("никто ничего не делает: DEADLOCK\n");
            exit(1);
        }
    }

    for (int i = 0; i < N; i++) pthread_join(t[i], NULL);
    printf("все поели: обедов %ld\n", (long)meals);
    return 0;
}
