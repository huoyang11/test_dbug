#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>

#include "mem.h"

#define WORD    (sizeof(void *))

int read_pro_mem(pid_t child,uint64_t addr,char *str,int len)
{
    int i = 0;
    int n = len / WORD;
    char *current_p = str;
    uint64_t data;

    for (;i < n;i++) {
        if ((data = ptrace(PTRACE_PEEKTEXT,child,addr+i*WORD,NULL)) < 0) {
            perror("read mem err");
            return -1;
        }

        memcpy(current_p,&data,WORD);
        current_p += WORD;
    }

    int remainder = len % WORD;
    if (remainder != 0) {
        if ((data = ptrace(PTRACE_PEEKTEXT,child,addr+i*WORD,NULL)) < 0) {
            perror("read mem err");
            return -1;
        }
        memcpy(current_p,&data,remainder);
    }

    return 0;
}

int write_pro_mem(pid_t child,uint64_t addr,char *str,int len)
{
    int i = 0;
    int n = len / WORD;
    char *current_p = str;
    uint64_t data;

    for (;i < n;i++) {
        memcpy(&data,current_p,WORD);
        if (ptrace(PTRACE_POKETEXT,child,addr +i*WORD,data) < 0) {
            perror("write mem err");
            return -1;
        }
        current_p += WORD;
    }

    int remainder = len % WORD;
    if (remainder != 0) {
        if ((data = ptrace(PTRACE_PEEKTEXT,child,addr+i*WORD,NULL)) < 0) {
            perror("read mem err");
            return -1;
        }

        //printf("data = %lx  remainder = %d\n",data,remainder);

        memcpy(&data,current_p,remainder);
        //printf("mem set data = %lx  remainder = %d\n",data,remainder);
        if (ptrace(PTRACE_POKETEXT,child,addr+i*WORD,data) < 0) {
            perror("write mem err");
            return -1;
        }

        //if ((data = ptrace(PTRACE_PEEKTEXT,child,addr+i*WORD,NULL)) < 0) {
        //    perror("read mem err");
        //    return -1;
        //}
        //printf("data = %lx  remainder = %d\n",data,remainder);
    }

    return 0;
}