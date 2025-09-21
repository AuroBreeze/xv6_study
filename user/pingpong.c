#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc != 1){
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }

    int p1[2], p2[2];
    pipe(p1); // parent -> child
    pipe(p2); // child -> parent

    int pid = fork();
    if(pid < 0){
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if(pid == 0){
        // ===== 子进程 =====
        close(p1[1]); // 子进程不写 p1
        close(p2[0]); // 子进程不读 p2

        char buf[10];
        read(p1[0], buf, sizeof(buf));
        fprintf(1, "%d: received %s\n", getpid(), buf);

        write(p2[1], "pong", 4);

        close(p1[0]);
        close(p2[1]);
    } else {
        // ===== 父进程 =====
        close(p1[0]); // 父进程不读 p1
        close(p2[1]); // 父进程不写 p2

        write(p1[1], "ping", 4);

        char buf[10];
        read(p2[0], buf, sizeof(buf));
        fprintf(1, "%d: received %s\n", getpid(), buf);

        close(p1[1]);
        close(p2[0]);
    }

    exit(0);
}
