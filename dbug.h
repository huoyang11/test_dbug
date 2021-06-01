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

struct breakpoint
{
    char *name;
    uint8_t type;
    uint8_t htype;
    uint64_t addr;
    uint64_t orig_code;
};

struct dbug_struct
{
    ngx_pool_t *pool;
    pid_t pid;
    int status;                     //进程的状态
    ngx_array_t *bps;               //断点数组
    struct user_regs_struct regs;   //调试运行的上下文
    struct fun_arr *funs;           //函数符号表
    struct breakpoint *current_bp;  //当前断下的断点
};

int init_dbug(struct dbug_struct *);
int uninit_dbug(struct dbug_struct *);
int insert_INT3_bp(pid_t pid,struct breakpoint *bp);
int resume_INT3_bp_next(pid_t pid,struct breakpoint *bp,struct user_regs_struct *regs);
int find_current_INT3_bp(struct dbug_struct *dbug);
int dbug_pro(struct dbug_struct *dbug,char *path,...);
int attach_pro(struct dbug_struct *dbug,pid_t pid);

#endif