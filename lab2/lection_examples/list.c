/*
 * list.c — потокобезопасное множество на упорядоченном связном списке.
 *
 *   ./list coarse [READ%]  — один мьютекс на весь список
 *   ./list rw     [READ%]  — один rwlock: contains читает, add/remove пишут
 *   ./list hoh    [READ%]  — hand-over-hand: мьютекс в каждом узле
 *   ./list hohbug [READ%]  — hand-over-hand с ошибкой в remove
 *   ./list opt    [READ%]  — оптимистичный: идём без блокировок
 *
 * 4 потока 2 секунды выполняют случайные contains/add/remove
 * (READ% — доля contains, по умолчанию 50). В конце печатается скорость
 * и проверяется, что размер списка = начальный + удачные add − удачные remove.
 *
 * В начале и в конце списка стоят стражи с ключами INT_MIN и INT_MAX,
 * поэтому curr никогда не бывает NULL.
 */
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THREADS 4
#define KEYS    1024        /* ключи 0..1023, в начале в списке чётные */

typedef struct node {
    int key;
    struct node *_Atomic next;  /* атомарный: opt читает его без блокировок */
    pthread_mutex_t lock;       /* нужен только hoh и opt                   */
} node;

node *head;

node *new_node(int key, node *next) {
    node *n = malloc(sizeof(node));
    n->key = key;
    n->next = next;
    pthread_mutex_init(&n->lock, NULL);
    return n;
}

/* ===== Обычный однопоточный список ===================================== */

int seq_add(int key) {
    node *pred = head, *curr = head->next;
    while (curr->key < key) { pred = curr; curr = curr->next; }
    if (curr->key == key) return 0;            /* уже есть */
    pred->next = new_node(key, curr);
    return 1;
}

int seq_remove(int key) {
    node *pred = head, *curr = head->next;
    while (curr->key < key) { pred = curr; curr = curr->next; }
    if (curr->key != key) return 0;            /* нет такого */
    pred->next = curr->next;
    free(curr);
    return 1;
}

int seq_contains(int key) {
    node *curr = head->next;
    while (curr->key < key) curr = curr->next;
    return curr->key == key;
}

/* ===== 1. coarse: один мьютекс на весь список ========================== */

pthread_mutex_t big_lock = PTHREAD_MUTEX_INITIALIZER;

int coarse_add(int key) {
    pthread_mutex_lock(&big_lock);
    int r = seq_add(key);
    pthread_mutex_unlock(&big_lock);
    return r;
}

int coarse_remove(int key) {
    pthread_mutex_lock(&big_lock);
    int r = seq_remove(key);
    pthread_mutex_unlock(&big_lock);
    return r;
}

int coarse_contains(int key) {
    pthread_mutex_lock(&big_lock);
    int r = seq_contains(key);
    pthread_mutex_unlock(&big_lock);
    return r;
}

/* ===== 2. rw: читатели не мешают друг другу ============================ */

pthread_rwlock_t big_rw = PTHREAD_RWLOCK_INITIALIZER;

int rw_add(int key) {
    pthread_rwlock_wrlock(&big_rw);
    int r = seq_add(key);
    pthread_rwlock_unlock(&big_rw);
    return r;
}

int rw_remove(int key) {
    pthread_rwlock_wrlock(&big_rw);
    int r = seq_remove(key);
    pthread_rwlock_unlock(&big_rw);
    return r;
}

int rw_contains(int key) {
    pthread_rwlock_rdlock(&big_rw);
    int r = seq_contains(key);
    pthread_rwlock_unlock(&big_rw);
    return r;
}

/* ===== 3. hoh: hand-over-hand ========================================== */

/* Идём по списку, держа ДВА узла: захватили следующий — отпустили
 * предыдущий. Возвращает pred и curr заблокированными.
 * Все захватывают узлы в одном порядке — по списку, поэтому deadlock невозможен. */
void hoh_find(int key, node **pred_out, node **curr_out) {
    node *pred = head;
    pthread_mutex_lock(&pred->lock);
    node *curr = pred->next;
    pthread_mutex_lock(&curr->lock);
    while (curr->key < key) {
        pthread_mutex_unlock(&pred->lock);
        pred = curr;
        curr = curr->next;
        pthread_mutex_lock(&curr->lock);
    }
    *pred_out = pred;
    *curr_out = curr;
}

int hoh_add(int key) {
    node *pred, *curr;
    hoh_find(key, &pred, &curr);
    int r = 0;
    if (curr->key != key) {
        pred->next = new_node(key, curr);
        r = 1;
    }
    pthread_mutex_unlock(&curr->lock);
    pthread_mutex_unlock(&pred->lock);
    return r;
}

int hoh_remove(int key) {
    node *pred, *curr;
    hoh_find(key, &pred, &curr);
    int r = 0;
    if (curr->key == key) {
        pred->next = curr->next;
        r = 1;
    }
    pthread_mutex_unlock(&curr->lock);
    pthread_mutex_unlock(&pred->lock);
    if (r) free(curr);     /* к curr можно прийти только через pred, а его держали мы */
    return r;
}

int hoh_contains(int key) {
    node *pred, *curr;
    hoh_find(key, &pred, &curr);
    int r = curr->key == key;
    pthread_mutex_unlock(&curr->lock);
    pthread_mutex_unlock(&pred->lock);
    return r;
}

/* ===== 4. hohbug: add и contains — как в hoh, ошибка только в remove == */

/* Держим только pred: «pred->next я меняю под его блокировкой, этого хватит». */
int bug_remove(int key) {
    node *pred = head;
    pthread_mutex_lock(&pred->lock);
    node *curr = pred->next;
    while (curr->key < key) {
        pthread_mutex_unlock(&pred->lock);
        pthread_mutex_lock(&curr->lock);
        pred = curr;
        curr = curr->next;
    }
    int r = 0;
    if (curr->key == key) {
        node *succ = curr->next;  /* curr не заблокирован: сосед может в этот  */
        usleep(100);              /* момент удалить succ (пауза расширяет окно) */
        pred->next = succ;        /* …и мы вернём удалённый succ обратно        */
        r = 1;
    }
    pthread_mutex_unlock(&pred->lock);
    return r;                     /* curr не освобождаем: на нём может кто-то стоять */
}

/* ===== 5. opt: оптимистичный =========================================== */

/* pred всё ещё в списке и всё ещё указывает на curr? */
int validate(node *pred, node *curr) {
    for (node *n = head; n->key <= pred->key; n = n->next)
        if (n == pred) return pred->next == curr;
    return 0;
}

/* Идём БЕЗ блокировок, потом блокируем pred и curr и проверяем,
 * что за это время ничего не поменялось. Поменялось — начинаем заново. */
void opt_find(int key, node **pred_out, node **curr_out) {
    for (;;) {
        node *pred = head, *curr = head->next;
        while (curr->key < key) { pred = curr; curr = curr->next; }
        pthread_mutex_lock(&pred->lock);
        pthread_mutex_lock(&curr->lock);
        if (validate(pred, curr)) {
            *pred_out = pred;
            *curr_out = curr;
            return;
        }
        pthread_mutex_unlock(&curr->lock);
        pthread_mutex_unlock(&pred->lock);
    }
}

int opt_add(int key) {
    node *pred, *curr;
    opt_find(key, &pred, &curr);
    int r = 0;
    if (curr->key != key) {
        pred->next = new_node(key, curr);
        r = 1;
    }
    pthread_mutex_unlock(&curr->lock);
    pthread_mutex_unlock(&pred->lock);
    return r;
}

int opt_remove(int key) {
    node *pred, *curr;
    opt_find(key, &pred, &curr);
    int r = 0;
    if (curr->key == key) {
        pred->next = curr->next;
        r = 1;
    }
    pthread_mutex_unlock(&curr->lock);
    pthread_mutex_unlock(&pred->lock);
    return r;     /* ВОПРОС: почему здесь нельзя сделать free(curr)? */
}

int opt_contains(int key) {
    node *pred, *curr;
    opt_find(key, &pred, &curr);
    int r = curr->key == key;
    pthread_mutex_unlock(&curr->lock);
    pthread_mutex_unlock(&pred->lock);
    return r;
}

/* ===== Нагрузка и проверка ============================================= */

/* Указатели на функции выбранного варианта — их выставляет main. */
int (*set_add)(int);
int (*set_remove)(int);
int (*set_contains)(int);

int read_pct = 50;
atomic_int  stop;
atomic_long ops, added, removed;

void *worker(void *arg) {
    unsigned seed = (long)arg + 1;
    long n = 0, a = 0, r = 0;
    while (!stop) {
        int key = rand_r(&seed) % KEYS;
        int op  = rand_r(&seed) % 100;
        if (op < read_pct)  set_contains(key);
        else if (op % 2)    a += set_add(key);
        else                r += set_remove(key);
        n++;
    }
    ops += n;
    added += a;
    removed += r;
    return NULL;
}

int main(int argc, char **argv) {
    char *mode = argc >= 2 ? argv[1] : "";
    if (argc >= 3) read_pct = atoi(argv[2]);

    if (strcmp(mode, "coarse") == 0) {
        set_add = coarse_add; set_remove = coarse_remove; set_contains = coarse_contains;
    } else if (strcmp(mode, "rw") == 0) {
        set_add = rw_add;     set_remove = rw_remove;     set_contains = rw_contains;
    } else if (strcmp(mode, "hoh") == 0) {
        set_add = hoh_add;    set_remove = hoh_remove;    set_contains = hoh_contains;
    } else if (strcmp(mode, "hohbug") == 0) {
        set_add = hoh_add;    set_remove = bug_remove;    set_contains = hoh_contains;
    } else if (strcmp(mode, "opt") == 0) {
        set_add = opt_add;    set_remove = opt_remove;    set_contains = opt_contains;
    } else {
        printf("использование: %s coarse|rw|hoh|hohbug|opt [READ%%]\n", argv[0]);
        return 1;
    }

    /* Начальный список: стражи и все чётные ключи. */
    head = new_node(INT_MIN, new_node(INT_MAX, NULL));
    for (int k = 0; k < KEYS; k += 2) seq_add(k);
    long start_size = KEYS / 2;

    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    sleep(2);
    stop = 1;
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);

    printf("%-6s чтений %3d%%: %6ld тыс. операций/с\n",
           mode, read_pct, (long)ops / 2000);

    /* Проверка: ключи по возрастанию и размер сходится со счётчиками. */
    long size = 0;
    int sorted = 1;
    for (node *n = head->next; n->key != INT_MAX; n = n->next) {
        if (n->next->key <= n->key) sorted = 0;
        size++;
    }
    long expected = start_size + added - removed;
    if (sorted && size == expected) {
        printf("корректно: в списке %ld ключей\n", size);
        return 0;
    }
    printf("НЕКОРРЕКТНО: в списке %ld ключей, а должно быть %ld%s\n",
           size, expected, sorted ? "" : "; порядок нарушен");
    return 1;
}
