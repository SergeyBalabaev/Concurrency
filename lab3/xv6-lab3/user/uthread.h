#ifndef UTHREAD_H
#define UTHREAD_H

#define MAX_THREADS 16

int  thread_create(void (*fn)(void *), void *arg);   // -> tid или -1
int  thread_join_t(int tid);                          // ждать + free стека
void thread_exit(void);                               // завершить поток

#endif