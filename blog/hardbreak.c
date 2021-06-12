#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define DR_OFFSET(num) 	((void *) (& ((struct user *) 0)->u_debugreg[num]))

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

    //设置dr0寄存器为bar的地址
    if (ptrace(PTRACE_POKEUSER, pid, DR_OFFSET(0), (void *)bar) < 0) {
		perror("tracer, faile to set DR_0\n");
	}

    uint64_t dr_7 = 0;
    //设置对应的标准位使得dr0寄存器的地址生效
	dr_7 = dr_7 | 0x01;//L0位,局部
	dr_7 = dr_7 | 0x02;//G0位,全局
    //设置dr7寄存器的值
	if (ptrace(PTRACE_POKEUSER, pid, DR_OFFSET(7), dr_7) < 0) {
		perror("tracer, faile to set DR_7\n");
	}

    //让子进程继续跑
    if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
        perror("ptrace SYSCALL err");
        return -1;
    }

    while (1) {

        wait(&status);
        if (WIFEXITED(status))
            return 0;

        ptrace(PTRACE_GETREGS,pid,NULL,&regs);
        //对比子进程当前断下的地址和函数bar的地址
        printf("function bar() = %p\n",(void *)bar);
        printf("break rip = %llx\n",regs.rip);

        //观察子进程是否暂停
        sleep(1);
 
        if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
            perror("ptrace SYSCALL err");
            return -1;
        }
    }

    return 0;
}