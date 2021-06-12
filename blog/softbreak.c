#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define INT_3 0xcc

void bar(int num)
{
    printf("num = %d\n",num);
}

int main(int argc,char *argv[])
{
    pid_t pid;
    int status = 0;
    struct user_regs_struct regs;

    pid = fork();

    if (pid == 0) { //子进程
        if (ptrace(PTRACE_TRACEME,0,NULL,NULL) < 0) {
            perror("ptrace TRACEME err");
            return -1;
        }

        pid_t pid = getpid();

        if (kill(pid,SIGSTOP) != 0) {
            perror("kill sigstop err");
        }

        for (int i = 0;i < 10;i++) {
            printf("%d\n",i);
            bar(111);
        }

        return 0;
    } else if (pid < 0) {
        perror("fork err");
        return -1;
    }
    //监听子进程的状态
    wait(&status);
    if (WIFEXITED(status))
        return 0;

    //保存原来的字节码
    uint64_t orig_code = ptrace(PTRACE_PEEKTEXT,pid,(void *)bar,0);
    //在地址的开头插入0xcc(插入软件断点)
	ptrace(PTRACE_POKETEXT, pid, (void *)bar, (orig_code & 0xFFFFFFFFFFFFFF00) | INT_3);

    //让子进程继续跑
    if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
        perror("ptrace SYSCALL err");
        return -1;
    }

    while (1) {

        wait(&status);
        if (WIFEXITED(status))
            return 0;

        //对比输出子进程断下的地址和bar地址
        ptrace(PTRACE_GETREGS,pid,NULL,&regs);
        printf("0x%llx\n",regs.rip);
        printf("%p\n",(void *)bar);

        //恢复之前的代码值
        ptrace(PTRACE_POKETEXT,pid,(void *)bar,orig_code);
        //0xcc占一个字节,让ip寄存器恢复
        regs.rip = regs.rip - 1;
        //设置寄存器的值(主要用于恢复ip寄存器)
        ptrace(PTRACE_SETREGS,pid,0,&regs);

        //用于看效果,确保子进程是真的断下
        sleep(1);

        //执行一条汇编指令,然后子进程会暂停
        ptrace(PTRACE_SINGLESTEP,pid,0,0);
        //等待子进程停止
        wait(NULL);

        //断点恢复
        ptrace(PTRACE_POKETEXT, pid, (void *)bar, (orig_code & 0xFFFFFFFFFFFFFF00) | INT_3);
        //子进程继续执行
        if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
            perror("ptrace SYSCALL err");
            return -1;
        }
    }

    return 0;
}