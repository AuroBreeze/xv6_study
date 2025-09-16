#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc != 1){
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }

    int p1[2], p2[2];
    pipe(p1); // parent -> child
    pipe(p2); // child -> parent

    int pid = fork();
    if(pid == 0){
        // 子进程
        char buf[10];
        read(p1[0], buf, 4);   // 从 parent 读 "ping"
        fprintf(1, "%d: received %s\n", getpid(), buf);

        write(p2[1], "pong", 4); // 回应 parent
    }else{
        // 父进程
        write(p1[1], "ping", 4); // 发给 child

        char buf[10];
        read(p2[0], buf, 4);   // 收到 "pong"
        fprintf(1, "%d: received %s\n", getpid(), buf);
    }

    exit(0);
}
