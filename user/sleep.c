#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc <= 1 || argc > 2){
        fprintf(2, "Usage: sleep <time>\n");
        exit(1);
    };
    int time = atoi(argv[1]);
    if(time < 0){
        fprintf(2, "Usage: sleep <time> > 0\n");
        exit(1);
    }
    sleep(time);
    exit(0);
}