/*
 * 05_rwlock.c — rwlock: читатели не мешают друг другу.
 *
 * Пять читателей постоянно читают настройки (два поля, которые всегда
 * должны совпадать), один писатель изредка их меняет. Программа считает,
 * сколько читателей было внутри одновременно.
 *
 *   gcc 05_rwlock.c -o 05_rwlock -pthread
 *   ./05_rwlock
 *
 * Замените rdlock на wrlock в reader() — читатели одновременно быть
 * перестанут.
 *
 * Уберите паузу «отдыхаем» в reader() — программа зависнет: читатели
 * сменяют друг друга без перерыва, и писатель не дождётся момента,
 * когда внутри никого нет (голодание писателя).
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

#define READERS 5

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
int version = 0, version_copy = 0;  /* инвариант: всегда равны */

atomic_int inside, max_inside;       /* читателей внутри сейчас / максимум */
atomic_int stop;

void *reader(void *arg) {
    long errors = 0;
    while (!stop) {
        pthread_rwlock_rdlock(&lock);
        int now = ++inside;
        if (now > max_inside) max_inside = now;
        if (version != version_copy) errors++;
        usleep(100);                 /* читаем долго */
        inside--;
        pthread_rwlock_unlock(&lock);
        usleep(100);                 /* отдыхаем */
    }
    if (errors) printf("читатель %ld: видел несогласованные данные %ld раз\n",
                       (long)arg, errors);
    return NULL;
}

void *writer(void *arg) {
    for (int i = 0; i < 20; i++) {
        usleep(50000);
        pthread_rwlock_wrlock(&lock);
        version++;
        usleep(100);                 /* пишем долго: инвариант временно нарушен */
        version_copy++;
        pthread_rwlock_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    pthread_t r[READERS], w;
    for (long i = 0; i < READERS; i++) pthread_create(&r[i], NULL, reader, (void *)i);
    pthread_create(&w, NULL, writer, NULL);

    pthread_join(w, NULL);
    stop = 1;
    for (int i = 0; i < READERS; i++) pthread_join(r[i], NULL);

    printf("версия настроек: %d\n", version);
    printf("одновременно читали до %d читателей\n", (int)max_inside);
    return 0;
}
