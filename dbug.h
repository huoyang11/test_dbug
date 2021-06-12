#ifndef _DBUG_H_
#define _DBUG_H_

#include <stdint.h>
#include <sys/user.h>
#include "ngx/config.h"

#define INT_3 0xCC

#define SFBP    1
#define HWBP    2

#define RBP     1
#define WBP     2

#define DR_OFFSET(num)  ((void *) (& ((struct user *) 0)->u_debugreg[num]))
//断点
struct breakpoint
{
    char *name;         //名称
    uint8_t type;       //断点类型(软件断点,硬件断点)
    uint8_t htype;      //硬件断点的类型(读、写、执行)
    uint64_t addr;      //断点地址
    uint64_t orig_code; //软件断点填充前的值
};
//调试寄存器 dr7
struct DR7 {
    uint64_t L0:1;
    uint64_t G0:1;
    uint64_t L1:1;
    uint64_t G1:1;
    uint64_t L2:1;
    uint64_t G2:1;
    uint64_t L3:1;
    uint64_t G3:1;
    uint64_t res1:8;
    uint64_t rw0:2;
    uint64_t len0:2;
    uint64_t rw1:2;
    uint64_t len1:2;
    uint64_t rw2:2;
    uint64_t len2:2;
    uint64_t rw3:2;
    uint64_t len3:2;
    uint64_t res2:32;
};
//调试寄存器
struct dbugregs
{
    uint64_t dr1;
    uint64_t dr2;
    uint64_t dr3;
    uint64_t dr4;
    uint64_t dr5;
    uint64_t dr6;
    struct DR7 dr7;
};

struct dbug_struct
{
    ngx_pool_t *pool;               //内存池
    pid_t pid;                      //被调试进程的pid
    int status;                     //进程的状态
    ngx_array_t *bps;               //断点数组
    ngx_array_t *cmd;               //命令解析缓存
    struct user_regs_struct regs;   //调试运行的上下文
    struct fun_arr *funs;           //函数符号表
    struct breakpoint *current_bp;  //当前断下的断点
};

int init_dbug(struct dbug_struct *);
int uninit_dbug(struct dbug_struct *);
//插入软件断点
int insert_INT3_bp(pid_t pid,struct breakpoint *bp);
//插入硬件断点
int insert_hard_break(pid_t pid,struct breakpoint *bp);
//恢复软件断点字节码，并且执行一条汇编
int resume_INT3_bp_next(pid_t pid,struct breakpoint *bp,struct user_regs_struct *regs);
//查找软件断点
int find_current_SF_bp(struct dbug_struct *dbug);
//查找硬件断点
int find_current_HW_bp(struct dbug_struct *dbug);
//调试一个进程
int dbug_pro(struct dbug_struct *dbug,char *path,char **argv);
//附加一个进程
int attach_pro(struct dbug_struct *dbug,pid_t pid);

#endif