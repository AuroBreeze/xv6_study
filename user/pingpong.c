#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc != 1){
        fprintf(2, "Usage: pingpong\n");
    }
    int p[2];
    pipe(p);
    uint pid = fork();
    uint parent_pid = getpid();

    if(pid == 0){
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        write(1," ",1);
    }else{
        
    }

    return 0;
}
