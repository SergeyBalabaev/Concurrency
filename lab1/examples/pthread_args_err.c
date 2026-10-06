/*

Пример ошибки с передачей в pthread_create переменных

Лечение:
    for (long i = 0; i < 4; i++)
        pthread_create(&t[i], NULL, worker, (void *)i);   // значение, а не адрес
        ...
        long id = (long)arg;          
*/

#include <pthread.h>
#include <stdio.h>

void *worker(void *arg) {
    long id = *(long*)arg;                     // читаем через переданный адрес
    printf("поток стартовал с id = %ld\n", id);
    return NULL;
}

int main(void) {
    pthread_t t[4];
    for (long i = 0; i < 4; i++)
        pthread_create(&t[i], NULL, worker, &i);   // значение, а не адрес

//    for (long i = 0; i < 4; i++)
//        pthread_create(&t[i], NULL, worker, &i);   // ОШИБКА: адрес общей i
    for (int i = 0; i < 4; i++)
        pthread_join(t[i], NULL);
    return 0;
}
