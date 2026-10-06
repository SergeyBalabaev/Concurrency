/*
Зачем закрывать второй конец записи
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];
    pipe(fd);

    if (fork() == 0) {                
        close(fd[0]);                  
        write(fd[1], "hello", 5);
        close(fd[1]);                 
        exit(0);
    }

    char buf[64];
    ssize_t n;
    close(fd[1]); 
    while ((n = read(fd[0], buf, sizeof buf)) > 0)
        printf("прочитано: %.*s\n", (int)n, buf);

    printf("read вернул EOF\n");       
    wait(NULL);
    return 0;
}
