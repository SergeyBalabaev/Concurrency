#include <pthread.h>
#include <stdio.h>


void *worker(void *arg) {
    printf("поток работает\n");
    return NULL;
}

int main(void) {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    pthread_join(t, NULL);               // ждём, результат не нужен → NULL
    printf("main: поток завершился\n");
    return 0;
}