#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc,char *argv[])
{
    pid_t pid;
    int status = 0;
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

        printf("child exit\n");

        return 0;
    } else if (pid < 0) {
        perror("fork err");
        return -1;
    }
    //监听子进程的状态
    wait(&status);
    if (WIFEXITED(status))
		return 0;
    //获取子进程的寄存器
    if (ptrace(PTRACE_GETREGS,pid,NULL,&regs) < 0) {
		perror("get regs err");
	}

    printf("rax = %llx\n",regs.rax);
    printf("rip = %llx\n",regs.rip);
    printf("rbp = %llx\n",regs.rbp);
    printf("rsp = %llx\n",regs.rsp);

    sleep(2);
    //让子进程继续执行
    if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
        perror("CONT err");
    }

    return 0;
}