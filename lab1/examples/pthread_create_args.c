#include <pthread.h>
#include <stdio.h>


typedef struct {
    int   id;
    long *data;
    int   lo, hi;
} Task;

void *worker(void *arg) {
    Task *t = arg;                       // распаковали структуру
    long sum = 0;
    for (int i = t->lo; i < t->hi; i++)
        sum += t->data[i];
    printf("поток %d: сумма куска = %ld\n", t->id, sum);
    return NULL;
}

int main(void) {
    long data[100] = {1, 2, 4, 3};
    pthread_t t[4];
    Task tasks[4];                       // живёт всё время работы потоков
    for (int i = 0; i < 4; i++) {
        tasks[i] = (Task){ .id = i, .data = data,
                           .lo = i * 25, .hi = (i + 1) * 25 };
        pthread_create(&t[i], NULL, worker, &tasks[i]);
    }
    for (int i = 0; i < 4; i++)
        pthread_join(t[i], NULL);
    return 0;
}