/*
 * buffer.c — ограниченный буфер: производители и потребители.
 *
 * PRODUCERS потоков кладут числа в общий буфер на SIZE ячеек,
 * CONSUMERS потоков их забирают. Каждое число производится один раз,
 * поэтому каждое должно быть и потреблено ровно один раз.
 *
 * Сборка:   gcc -O2 -g buffer.c -o buffer -pthread
 * TSan:     gcc -O1 -g -fsanitize=thread buffer.c -o buffer-tsan -pthread
 */
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#define SIZE      8           /* ёмкость буфера */
#define PRODUCERS 3
#define CONSUMERS 2
#define ITEMS     60000       /* чисел от каждого производителя */
#define TOTAL     (PRODUCERS * ITEMS)

int buf[SIZE];
int head = 0;                 /* откуда забирать */
int tail = 0;                 /* куда класть     */
int count = 0;                /* сколько лежит   */

int seen[TOTAL];              /* сколько раз потреблено каждое число */

/* TODO: синхронизация. Положить x в буфер; если он полон — ждать. */
void put(int x) {
    buf[tail] = x;
    tail = (tail + 1) % SIZE;
    count++;
}

/* TODO: синхронизация. Забрать число из буфера; если он пуст — ждать. */
int get(void) {
    int x = buf[head];
    head = (head + 1) % SIZE;
    count--;
    return x;
}

void *producer(void *arg) {
    int id = (long)arg;
    for (int i = 0; i < ITEMS; i++)
        put(id * ITEMS + i);  
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 0; i < TOTAL / CONSUMERS; i++)
        seen[get()]++;
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

    pthread_t p[PRODUCERS], c[CONSUMERS];
    for (long i = 0; i < PRODUCERS; i++) pthread_create(&p[i], NULL, producer, (void *)i);
    for (long i = 0; i < CONSUMERS; i++) pthread_create(&c[i], NULL, consumer, (void *)i);
    for (int i = 0; i < PRODUCERS; i++) pthread_join(p[i], NULL);
    for (int i = 0; i < CONSUMERS; i++) pthread_join(c[i], NULL);

    int lost = 0, twice = 0;
    for (int i = 0; i < TOTAL; i++) {
        if (seen[i] == 0) lost++;
        if (seen[i] > 1)  twice++;
    }
    printf("потеряно: %d, потреблено дважды: %d\n", lost, twice);
    if (lost == 0 && twice == 0) {
        printf("корректно\n");
        return 0;
    }
    printf("НЕКОРРЕКТНО\n");
    return 1;
}
