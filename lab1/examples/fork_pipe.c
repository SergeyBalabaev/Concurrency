#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];
    pipe(fd);                              // fd[0] — чтение, fd[1] — запись

    if (fork() == 0) {                     
        close(fd[1]);                      
        char buf[64];
        ssize_t n = read(fd[0], buf, sizeof buf);   
        printf("child get: %.*s\n", (int)n, buf);
        close(fd[0]);
        exit(0);
    }

    close(fd[0]);                         
    write(fd[1], "Hello MPSU!", strlen("Hello MPSU!"));
    close(fd[1]);                          
    wait(NULL);
    return 0;
}