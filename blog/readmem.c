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
    int status = 0;
    uint64_t num = 0;

    pid = fork();

    if (pid == 0) { //子进程
        if (ptrace(PTRACE_TRACEME,0,NULL,NULL) < 0) {
            perror("ptrace TRACEME err");
            return -1;
        }

		pid_t pid = getpid();
        num = 20;

		if (kill(pid,SIGSTOP) != 0) {
			perror("kill sigstop err");
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

    uint64_t tem = 0;

    tem = ptrace(PTRACE_PEEKDATA,pid,&num,NULL);
    printf("read num = %ld\n",tem);
    printf("this num = %ld\n",num);

    //让子进程继续跑
    if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
        perror("ptrace SYSCALL err");
        return -1;
    }

    return 0;
}