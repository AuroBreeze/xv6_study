#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

static char *_basename(char *path){
    char *p;

    for(p = path+strlen(path); p>=path && *p != '/'; --p);
    ++p;
    return p;
}
int find(char *path, char *name){
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, O_RDONLY)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return -1;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return -1;
    }
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        close(fd);
        return -1;
    }
    strcpy(buf, path);
    
    short type = st.type;
    if(type == T_FILE || type == T_DEVICE){
        char *basename = _basename(path);
        if(strcmp(name, basename) == 0){
            fprintf(1, "%s\n", path);
        }
    }else if(type == T_DIR){
        p = buf+strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            if(de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                fprintf(2, "find: connt stat %s\n", buf);
                continue;
            }
            find(buf, name);
        }
    }
    close(fd);
    return 1;
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(2, "Usage: find <path> <name>\n");
        exit(0);
    }
    find(argv[1], argv[2]);
    exit(0);
    return 0;
}


