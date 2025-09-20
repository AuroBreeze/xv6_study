#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc != 1){
        fprintf(2, "usage: sysinfo");
    }
    struct sysinfo infos;

    info(&infos);

    fprintf(1,"Free memory: %d bytes\n", infos.freemem);
    fprintf(1, "Active processes: %d\n", infos.nproc);
    exit(0);
    return 0;
}

// 0x00000000800001ae