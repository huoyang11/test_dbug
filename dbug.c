#include <stdarg.h>
#include <sys/ptrace.h>

#include "dbug.h"
#include "mem.h"
#include "elf_parse.h"

int insert_INT3_bp(pid_t pid,struct breakpoint *bp)
{
    if (!bp) return -1;

    bp->type = SFBP;
    uint64_t addr = bp->addr;
    //if ((bp->orig_code = ptrace(PTRACE_PEEKTEXT,pid,(void *)addr,0)) < 0) {
    //    perror("read mem err");
    //}
    ////printf("code %lx\n",orig_code);
    //if (ptrace(PTRACE_POKETEXT, pid, (void *)addr, (bp->orig_code & 0xFFFFFFFFFFFFFF00) | INT_3) < 0) {
    //    perror("insert INT3 err");
    //}

    read_pro_mem(pid,addr,(void *)&bp->orig_code,1);
    char bit = INT_3;
    write_pro_mem(pid,addr,&bit,1); 

    printf("break %lx(%s)\n",bp->addr,bp->name);

    return 0;
}

int insert_hard_break(pid_t pid,struct breakpoint *bp)
{
    struct DR7 dr_7 = {0};
 
    bp->type = HWBP;
    if (ptrace(PTRACE_POKEUSER, pid, DR_OFFSET(0), (void *)bp->addr) < 0) {
        perror("tracer, faile to set DR_0\n");
    }

    dr_7.L0 = 1;
    dr_7.G0 = 1;

    uint64_t dr7 = 0;
    memcpy(&dr7,&dr_7,8);

    if (ptrace(PTRACE_POKEUSER, pid, DR_OFFSET(7), (void *)dr7) < 0) {
        perror("tracer, faile to set DR_7\n");
    }

    return 0;
}

int resume_INT3_bp_next(pid_t pid,struct breakpoint *bp,struct user_regs_struct *regs)
{
    if (!bp || !(bp->type & SFBP)) return -1;

    uint64_t addr = bp->addr;
    //if (ptrace(PTRACE_POKETEXT,pid,(void *)addr,bp->orig_code) < 0) {
    //    perror("write mem err");
    //}
    write_pro_mem(pid,addr,(void *)&bp->orig_code,1);

    regs->rip = bp->addr;
    ptrace(PTRACE_SETREGS,pid,NULL,regs);
    //执行一条汇编
    ptrace(PTRACE_SINGLESTEP,pid,0,0);
    wait(NULL);

    //printf("resume %lx\n",(bp->orig_code & 0xFFFFFFFFFFFFFF00) | INT_3);
    //ptrace(PTRACE_POKETEXT, pid, (void *)addr, (bp->orig_code & 0xFFFFFFFFFFFFFF00) | INT_3);
    //断点恢复
    char bit = INT_3;
    write_pro_mem(pid,addr,&bit,1);

    return 0;
}

int find_current_SF_bp(struct dbug_struct *dbug)
{
    if (!dbug) return -1;

    int i = 0;
    int size = dbug->bps->nelts;
    struct breakpoint *bps = dbug->bps->elts;

    for (;i < size;i++) {
        if (!(bps[i].type & SFBP))
            continue;

        if (bps[i].addr == (dbug->regs.rip - 1)) 
            break;
    }

    if (i == size) return -1;

    return i;
}

int find_current_HW_bp(struct dbug_struct *dbug)
{
    if (!dbug) return -1;

    int i = 0;
    int size = dbug->bps->nelts;
    struct breakpoint *bps = dbug->bps->elts;

    for (;i < size;i++) {
        if (!(bps[i].type & HWBP))
            continue;

        if (bps[i].addr == dbug->regs.rip) 
            break;
    }

    if (i == size) return -1;

    return i;
}

int init_dbug(struct dbug_struct *dbug)
{
    if (!dbug) return -1;

    ngx_pool_t *pool = ngx_create_pool(4096);
    dbug->pool = pool;
    dbug->bps = ngx_array_create(pool,10,sizeof(struct breakpoint));
    dbug->cmd = ngx_array_create(pool,10,sizeof(ngx_str_t));

    return 0;
}

int uninit_dbug(struct dbug_struct *dbug)
{
    if (!dbug) return -1;

    ngx_destroy_pool(dbug->pool);

    return 0;
}

int dbug_pro(struct dbug_struct *dbug,char *path,char **argv)
{
    pid_t pid = 0;
    int size = 0;
    ngx_pool_t *pool = dbug->pool;

    pid = fork();

    if (pid == 0) {
        if (ptrace(PTRACE_TRACEME,0,NULL,NULL) < 0) {
            perror("PTRACE_TRACEME err");
            return -1;
        }

        if (execvp(path,argv) < 0) {
            perror("exec err");
            return -1;
        }
    } else if (pid < 0) {
        perror("fork err");
        return -1;
    }

    wait(&dbug->status);
    if (WIFEXITED(dbug->status)) return -1;

    dbug->pid = pid;
    //获取函数符号
    dbug->funs = open_funs(dbug->pid);

    return 0;
}

int attach_pro(struct dbug_struct *dbug,pid_t pid)
{
    ngx_pool_t *pool = dbug->pool;

    if (ptrace(PTRACE_ATTACH,pid,NULL,NULL) < 0) {
        perror("ATTACH err");
        return -1;
    }

    if (waitpid(pid, &dbug->status, 0) < 0) {
        perror("wait err");
    }
    if (WIFEXITED(dbug->status)) return -1;

    dbug->pid = pid;
    dbug->funs = open_funs(dbug->pid);

    return 0;
}