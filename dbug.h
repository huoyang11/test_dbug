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

struct breakpoint
{
    char *name;
    uint8_t type;
    uint8_t htype;
    uint64_t addr;
    uint64_t orig_code;
};

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
    ngx_pool_t *pool;
    pid_t pid;
    int status;                     //进程的状态
    ngx_array_t *bps;               //断点数组
    ngx_array_t *cmd;
    struct user_regs_struct regs;   //调试运行的上下文
    struct fun_arr *funs;           //函数符号表
    struct breakpoint *current_bp;  //当前断下的断点
};

int init_dbug(struct dbug_struct *);
int uninit_dbug(struct dbug_struct *);
int insert_INT3_bp(pid_t pid,struct breakpoint *bp);
int insert_hard_break(pid_t pid,struct breakpoint *bp);
int resume_INT3_bp_next(pid_t pid,struct breakpoint *bp,struct user_regs_struct *regs);
int find_current_SF_bp(struct dbug_struct *dbug);
int find_current_HW_bp(struct dbug_struct *dbug);
int dbug_pro(struct dbug_struct *dbug,char *path,char **argv);
int attach_pro(struct dbug_struct *dbug,pid_t pid);

#endif