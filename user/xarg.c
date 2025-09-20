#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

// xv6 的 xargs 实现：
// 从 stdin 读取 token，把它们拼到命令后面执行
int
main(int argc, char *argv[])
{
    if(argc < 2){
        fprintf(2, "Usage: xargs command [args...]\n");
        exit(1);
    }

    char buf[512];
    int n;
    int m = 0; // 已经读到多少字节
    char *xargv[MAXARG];
    int i;

    // 先把命令行参数拷贝进去 (跳过 xargs 本身)
    for(i = 1; i < argc; i++){
        xargv[i-1] = argv[i];
        // fprintf(2, "xargv: argv[%d] = %s\n", i-1, xargv[i-1]);
    }
    int fixed = argc - 1; // 固定参数数量

    // 从 stdin 一点点读
    while((n = read(0, buf + m, sizeof(buf) - m - 1)) > 0){
        // fprintf(2, "xargv: read %d bytes\n", n);
        m += n;
        buf[m] = 0;
        // fprintf(2, "xargv: buf = %s\n", buf);

        char *p = buf;
        char *start = p;
        for(; *p; p++){
            if(*p == ' ' || *p == '\n'){
                if(start != p){
                    // 把 token 塞进 argv
                    *p = 0;  // 切断 token
                    xargv[fixed] = start;
                    xargv[fixed+1] = 0;
                    // fprintf(2, "xargv[fixed]: argv[%d] = %s\n", fixed, xargv[fixed]);
                    // fprintf(2, "xargv[fixed+1]: argv[%d] = %s\n", fixed+1, xargv[fixed+1]);
                    if(fork() == 0){
                        // fprintf(2, "xargv: fork %s\n", xargv[0]);
                        // fprintf(2, "xargv: exec %s\n", xargv[1]);
                        exec(xargv[0], xargv);
                        fprintf(2, "xargv: exec %s failed\n", xargv[0]);
                        exit(1);
                    }
                    wait(0);
                }
                start = p+1;
            }
        }

        // 把最后一个残留 token 移到 buf 开头（避免跨 read）
        if(start < p){
            m = (int)(p - start);            // 强制转 int，防止报错
            memmove(buf, start, (uint)m);  // 这里用 uint
        }else{
            m = 0;
        }
    }

    exit(0);
}
