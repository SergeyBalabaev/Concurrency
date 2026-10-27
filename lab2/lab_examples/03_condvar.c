/*
 * 03_condvar.c — условная переменная: один поток ждёт, пока другой
 * подготовит данные.
 *
 *   gcc 03_condvar.c -o 03_condvar -pthread
 *   ./03_condvar
 *
 * Пока main готовит данные (2 секунды), ждущий поток спит и процессор
 * не занимает — проверьте в соседнем терминале: top -H -p $(pgrep 03_condvar)
 */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  c = PTHREAD_COND_INITIALIZER;
int ready = 0;                   /* условие: данные готовы */
int data;                        /* сами данные            */

void *waiter(void *arg) {
    pthread_mutex_lock(&m);
    while (!ready)               /* всегда while, а не if */
        pthread_cond_wait(&c, &m);
    /* здесь ready == 1, и мьютекс снова у нас */
    printf("поток: дождался, data = %d\n", data);
    pthread_mutex_unlock(&m);
    return NULL;
}

int main(void) {
    pthread_t t;
    pthread_create(&t, NULL, waiter, NULL);

    printf("main: готовлю данные...\n");
    sleep(2);

    pthread_mutex_lock(&m);
    data = 42;
    ready = 1;
    pthread_cond_signal(&c);     /* разбудить ждущего */
    pthread_mutex_unlock(&m);
    printf("main: данные готовы, сигнал отправлен\n");

    pthread_join(t, NULL);
    return 0;
}
