/*
 * life.c — игра «Жизнь» Конвея в несколько потоков.
 *
 * Поле N×N, клетка живая (1) или мёртвая (0). На каждом шаге клетка
 * смотрит на 8 соседей: живая выживает при 2 или 3 живых соседях,
 * мёртвая оживает ровно при 3. Край поля замкнут в тор.
 *
 * Хранятся два поля: grid[0] и grid[1]. На шаге step потоки читают
 * grid[step % 2] и пишут grid[(step + 1) % 2]. Каждый поток считает
 * свою полосу строк.
 *
 * main сначала считает результат в одном потоке (эталон), потом —
 * в THREADS потоков, и сравнивает.
 *
 * Сборка:   gcc -O2 -g life.c -o life -pthread
 * TSan:     gcc -O1 -g -fsanitize=thread life.c -o life-tsan -pthread
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N       256
#define STEPS   300
#define THREADS 4

unsigned char grid[2][N][N];
unsigned char start[N][N];       /* начальное поле */
unsigned char expected[N][N];    /* эталон: результат однопоточного счёта */

/* Посчитать строки [lo, hi) нового поля по старому. */
void step_rows(int step, int lo, int hi) {
    unsigned char (*cur)[N] = grid[step % 2];
    unsigned char (*nxt)[N] = grid[(step + 1) % 2];
    for (int r = lo; r < hi; r++) {
        for (int c = 0; c < N; c++) {
            int alive = 0;
            for (int dr = -1; dr <= 1; dr++)
                for (int dc = -1; dc <= 1; dc++)
                    if (dr || dc)
                        alive += cur[(r + dr + N) % N][(c + dc + N) % N];
            nxt[r][c] = cur[r][c] ? (alive == 2 || alive == 3) : (alive == 3);
        }
    }
}

/* TODO: синхронизация */
void *worker(void *arg) {
    int id = (long)arg;
    int lo = id * N / THREADS, hi = (id + 1) * N / THREADS;
    for (int step = 0; step < STEPS; step++)
        step_rows(step, lo, hi);
    return NULL;
}

int main(void) {
    srand(42);
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            start[r][c] = rand() % 4 == 0;

    /* 1. Эталон в одном потоке */
    memcpy(grid[0], start, sizeof(start));
    for (int step = 0; step < STEPS; step++)
        step_rows(step, 0, N);
    memcpy(expected, grid[STEPS % 2], sizeof(expected));

    /* 2. То же в THREADS потоков */
    memcpy(grid[0], start, sizeof(start));
    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);

    int diff = 0;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (grid[STEPS % 2][r][c] != expected[r][c]) diff++;
    printf("клеток, не совпавших с эталоном: %d\n", diff);
    if (diff == 0) {
        printf("корректно\n");
        return 0;
    }
    printf("НЕКОРРЕКТНО\n");
    return 1;
}
