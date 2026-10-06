#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid == 0) {                       // потомок станет другой программой
        char *args[] = { "ls", "-l", NULL };
        execvp("ls", args);
        perror("execvp");                 // сюда — только если запуск не удался
        exit(127);
    }
    wait(NULL);                           // родитель дожидается потомка
    printf("parent: child exit\n");
    return 0;
}