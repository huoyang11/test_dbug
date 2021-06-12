#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc,char *argv[])
{
    pid_t pid;
    int orig_rax;
    int iscalling = 0;
    int status = 0;
    uint64_t arg1,arg2,arg3;
    //一组寄存器的值
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

        write(STDOUT_FILENO,"aaaa -> ",8);
        write(STDOUT_FILENO,"bbbb -> ",8);
        write(STDOUT_FILENO,"cccc -> ",8);

        return 0;
    } else if (pid < 0) {
        perror("fork err");
        return -1;
    }
    //监听子进程的状态
    wait(&status);
    if (WIFEXITED(status))
		return 0;

    //让子进程在调用系统调用时暂停
    if (ptrace(PTRACE_SYSCALL,pid,NULL,NULL) < 0) {
        perror("ptrace SYSCALL err");
        return -1;
    }

    while (1) {
        wait(&status);
        if (WIFEXITED(status))
		    break;

        ptrace(PTRACE_GETREGS,pid,NULL,&regs);//获取寄存器整个结构
        //在struct user结构体中的偏移
        orig_rax = regs.orig_rax;//获取系统调用号
        if (orig_rax == SYS_write) {
            if (!iscalling) {//系统调用前
                iscalling = 1;
                arg1 = regs.rdi;
                arg2 = regs.rsi;
                arg3 = regs.rdx;
            } else {//系统调用后
                printf("%lld = write(%ld,\"%s\",%ld)\n",regs.rax,arg1,(char *)arg2,arg3);
                iscalling = 0;
            }
        }
        //让子进程在调用系统调用时暂停
        if (ptrace(PTRACE_SYSCALL,pid,NULL,NULL) < 0) {
            perror("CONT err");
            return -1;
        }
    }

    return 0;
}