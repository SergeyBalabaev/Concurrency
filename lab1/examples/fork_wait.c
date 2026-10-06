#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid == 0) {
        sleep(5);
        exit(42);                     // потомок завершается кодом 42
    }
    int status;
    waitpid(pid, &status, 0);          // родитель собирает его
    if (WIFEXITED(status))
        printf("child exit with code %d\n", WEXITSTATUS(status));
    return 0;
}