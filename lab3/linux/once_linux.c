// once_linux.c — сверка с Linux: pthread_once (аналог call_once из БДЗ-3).
//   gcc once_linux.c -o once_linux -pthread
//   strace -f -e trace=futex ./once_linux
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NT 8
#define TBL 1024

static pthread_once_t once = PTHREAD_ONCE_INIT;
static int table[TBL];
static int init_calls;

static void build_table(void) {
    init_calls++;
    for (int i = 0; i < TBL; i++) table[i] = i * i;
    usleep(200000);                       // долгая инициализация: 0.2 с
}

static void *user_of_table(void *arg) {
    pthread_once(&once, build_table);     // ранние: ждут на futex
    return NULL;
}

int main(void) {
    pthread_t t[NT];
    for (int i = 0; i < NT; i++) pthread_create(&t[i], NULL, user_of_table, NULL);
    for (int i = 0; i < NT; i++) pthread_join(t[i], NULL);
    fprintf(stderr, "--- late calls start ---\n");
    for (int i = 0; i < 1000000; i++)
        pthread_once(&once, build_table);  // поздние: быстрый путь
    fprintf(stderr, "--- late calls end ---\n");
    printf("init_calls=%d, once_control=%d\n", init_calls, (int)once);
    return 0;
}
