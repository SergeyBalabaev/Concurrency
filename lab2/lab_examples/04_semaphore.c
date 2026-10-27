/*
 * 04_semaphore.c — семафор в двух ролях.
 *
 * 1. Счётчик ресурсов: шесть потоков хотят соединиться с базой данных,
 *    а свободных соединений всего три. Семафор с начальным значением 3
 *    пропускает не больше трёх потоков одновременно.
 * 2. Сигнал о событии: семафор с начальным значением 0. main ждёт в
 *    sem_wait, пока каждый поток не сообщит о завершении через sem_post.
 *
 *   gcc 04_semaphore.c -o 04_semaphore -pthread
 *   ./04_semaphore
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

#define THREADS     6
#define CONNECTIONS 3

sem_t connections;               /* роль 1: свободные соединения    */
sem_t done;                      /* роль 2: «поток закончил работу» */
atomic_int busy;                 /* сколько соединений занято сейчас */

void *client(void *arg) {
    long id = (long)arg;

    sem_wait(&connections);      /* взять соединение (или ждать) */
    int now = ++busy;
    printf("поток %ld: подключился, занято соединений: %d\n", id, now);
    sleep(1);                    /* работаем с базой */
    busy--;
    printf("поток %ld: отключился\n", id);
    sem_post(&connections);      /* вернуть соединение */

    sem_post(&done);             /* сообщить main: я закончил */
    return NULL;
}

int main(void) {
    sem_init(&connections, 0, CONNECTIONS);
    sem_init(&done, 0, 0);

    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) {
        pthread_create(&t[i], NULL, client, (void *)i);
        pthread_detach(t[i]);    /* join не нужен: о завершении узнаем через done */
    }

    for (int i = 0; i < THREADS; i++)
        sem_wait(&done);         /* дождаться всех */
    printf("main: все потоки закончили\n");
    return 0;
}
