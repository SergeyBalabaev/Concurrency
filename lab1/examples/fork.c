#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("before fork\n");
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        sleep(10);
        printf("pid return %d, child pid: %d, my parent %d\n", pid, getpid(), getppid());
    } else {
        printf("parent: my child has pid %d\n", pid);
    }
    printf("after fork\n");
    return 0;
}