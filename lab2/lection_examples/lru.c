/*
 * lru.c — LRU-кэш: хэш-таблица (ключ → элемент) + двусвязный список.
 * В голове списка самый свежий элемент, в хвосте — самый старый:
 * его и выбрасываем, когда кэш полон.
 *
 *   ./lru mutex    — один мьютекс на весь кэш
 *   ./lru rwbug    — rwlock: get под rdlock («это же чтение»), put под wrlock
 *   ./lru sharded  — SHARDS маленьких кэшей, каждый со своим мьютексом
 *
 * 4 потока 2 секунды делают 95% get и 5% put. В конце печатается скорость
 * и проверяется целостность списка.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THREADS  4
#define CAPACITY 1024      /* сколько элементов помещается в кэш        */
#define KEYS     2048      /* ключей вдвое больше — половина не влезет  */
#define BUCKETS  1024      /* корзин хэш-таблицы                        */
#define SHARDS   16

typedef struct entry {
    int key, value;
    struct entry *prev, *next;   /* соседи в списке LRU      */
    struct entry *hnext;         /* следующий в той же корзине */
} entry;

typedef struct {
    entry *table[BUCKETS];
    entry head;                  /* страж: head.next — самый свежий, head.prev — самый старый */
    int size, capacity;
    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;
} cache;

void cache_init(cache *c, int capacity) {
    memset(c->table, 0, sizeof(c->table));
    c->head.next = c->head.prev = &c->head;
    c->size = 0;
    c->capacity = capacity;
    pthread_mutex_init(&c->mutex, NULL);
    pthread_rwlock_init(&c->rwlock, NULL);
}

/* ===== Обычный однопоточный LRU ======================================== */

void list_remove(entry *e) {
    e->prev->next = e->next;
    e->next->prev = e->prev;
}

void list_push_front(cache *c, entry *e) {
    e->next = c->head.next;
    e->prev = &c->head;
    c->head.next->prev = e;
    c->head.next = e;
}

entry *table_find(cache *c, int key) {
    for (entry *e = c->table[key % BUCKETS]; e != NULL; e = e->hnext)
        if (e->key == key) return e;
    return NULL;
}

void table_remove(cache *c, entry *e) {
    entry **p = &c->table[e->key % BUCKETS];
    while (*p != e) p = &(*p)->hnext;
    *p = e->hnext;
}

/* Найти и сделать самым свежим. Обратите внимание: get МЕНЯЕТ список! */
int lru_get(cache *c, int key, int *value) {
    entry *e = table_find(c, key);
    if (e == NULL) return 0;
    list_remove(e);
    list_push_front(c, e);
    *value = e->value;
    return 1;
}

void lru_put(cache *c, int key, int value) {
    entry *e = table_find(c, key);
    if (e != NULL) {                       /* уже есть — обновляем */
        e->value = value;
        list_remove(e);
        list_push_front(c, e);
        return;
    }
    if (c->size == c->capacity) {          /* полон — выбрасываем самый старый */
        entry *old = c->head.prev;
        list_remove(old);
        table_remove(c, old);
        free(old);
        c->size--;
    }
    e = malloc(sizeof(entry));
    e->key = key;
    e->value = value;
    e->hnext = c->table[key % BUCKETS];
    c->table[key % BUCKETS] = e;
    list_push_front(c, e);
    c->size++;
}

/* ===== 1. mutex: один мьютекс на весь кэш ============================== */

cache one;

int get_mutex(int key, int *value) {
    pthread_mutex_lock(&one.mutex);
    int r = lru_get(&one, key, value);
    pthread_mutex_unlock(&one.mutex);
    return r;
}

void put_mutex(int key, int value) {
    pthread_mutex_lock(&one.mutex);
    lru_put(&one, key, value);
    pthread_mutex_unlock(&one.mutex);
}

/* ===== 2. rwbug: get «только читает» — значит, rdlock? ================= */

int get_rw(int key, int *value) {
    pthread_rwlock_rdlock(&one.rwlock);    /* ОШИБКА: lru_get меняет список */
    int r = lru_get(&one, key, value);
    pthread_rwlock_unlock(&one.rwlock);
    return r;
}

void put_rw(int key, int value) {
    pthread_rwlock_wrlock(&one.rwlock);
    lru_put(&one, key, value);
    pthread_rwlock_unlock(&one.rwlock);
}

/* ===== 3. sharded: много маленьких кэшей =============================== */

cache shards[SHARDS];

int get_sharded(int key, int *value) {
    cache *c = &shards[key % SHARDS];      /* ключ сам выбирает свой кэш */
    pthread_mutex_lock(&c->mutex);
    int r = lru_get(c, key, value);
    pthread_mutex_unlock(&c->mutex);
    return r;
}

void put_sharded(int key, int value) {
    cache *c = &shards[key % SHARDS];
    pthread_mutex_lock(&c->mutex);
    lru_put(c, key, value);
    pthread_mutex_unlock(&c->mutex);
}

/* ===== Нагрузка и проверка ============================================= */

/* Указатели на функции выбранного варианта — их выставляет main. */
int  (*cache_get)(int key, int *value);
void (*cache_put)(int key, int value);

atomic_int  stop;
atomic_long ops;

void *worker(void *arg) {
    unsigned seed = (long)arg + 1;
    long n = 0;
    while (!stop) {
        int key = rand_r(&seed) % KEYS;
        int value;
        if (rand_r(&seed) % 100 < 95)
            cache_get(key, &value);
        else
            cache_put(key, key * 10);
        n++;
    }
    ops += n;
    return NULL;
}

/* Список цел: prev и next согласованы, длина равна size. */
int check(cache *c) {
    int len = 0;
    entry *prev = &c->head;
    for (entry *e = c->head.next; e != &c->head; e = e->next) {
        if (e->prev != prev) return 0;     /* связи разошлись */
        if (++len > c->capacity) return 0; /* список длиннее, чем может быть */
        prev = e;
    }
    return len == c->size;
}

int main(int argc, char **argv) {
    char *mode = argc == 2 ? argv[1] : "";
    int nshards = 1;
    cache *caches = &one;

    if (strcmp(mode, "mutex") == 0) {
        cache_get = get_mutex;   cache_put = put_mutex;
    } else if (strcmp(mode, "rwbug") == 0) {
        cache_get = get_rw;      cache_put = put_rw;
    } else if (strcmp(mode, "sharded") == 0) {
        cache_get = get_sharded; cache_put = put_sharded;
        nshards = SHARDS;
        caches = shards;
    } else {
        printf("использование: %s mutex|rwbug|sharded\n", argv[0]);
        return 1;
    }
    for (int i = 0; i < nshards; i++) cache_init(&caches[i], CAPACITY / nshards);

    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    sleep(2);
    stop = 1;
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);

    printf("%-7s %6ld тыс. операций/с\n", mode, (long)ops / 2000);

    for (int i = 0; i < nshards; i++) {
        if (!check(&caches[i])) {
            printf("НЕКОРРЕКТНО: список испорчен\n");
            return 1;
        }
    }
    printf("корректно\n");
    return 0;
}
