#include <sys/ptrace.h>
#include <sys/wait.h>
#include <ctype.h>

#include "command.h"
#include "dbug.h"

struct command cmd_set[] = {
    {"b",command_break},
    {"c",command_continue},
    {"show",command_show},
    {"dbug",command_dbug},
    {"attach",command_attach},
    {NULL,NULL},
};

int command_break(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    ngx_str_t *str = (ngx_str_t *)((ngx_array_t *)arg)->elts;

    int index = find_func(dbug->funs,str[1].data);
    if (index == -1) return -1;
    
    struct breakpoint *bp = ngx_array_push(dbug->bps);
    bp->name = dbug->funs->data[index].name;
    bp->addr = dbug->funs->data[index].addr;
    insert_INT3_bp(dbug->pid,bp);

    return 0;
}

int command_continue(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;
    struct breakpoint *bps = (struct breakpoint *)dbug->bps->elts;

    if (ptrace(PTRACE_CONT,dbug->pid,NULL,NULL) < 0) {
        perror("ptrace cont err");
    }

    wait(&dbug->status);
    if (WIFEXITED(dbug->status)) exit(0);

    ptrace(PTRACE_GETREGS,dbug->pid,NULL,&dbug->regs);

    if (WSTOPSIG(dbug->status) == SIGTRAP) {
        int index = find_current_INT3_bp(dbug);
        if (index != -1) {
            printf("trigger breakpoint %lx(%s)\n",bps[index].addr,bps[index].name);
            resume_INT3_bp_next(dbug->pid,&bps[index],&dbug->regs);
        }
    }

    return 0;
}

int command_show(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    foreach_fun(dbug->funs);

    return 0;
}

int command_dbug(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    ngx_str_t *str = (ngx_str_t *)((ngx_array_t *)arg)->elts;
    dbug_pro(dbug,str[1].data,NULL);

    return 0;
}

int command_attach(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    ngx_str_t *str = (ngx_str_t *)((ngx_array_t *)arg)->elts;
    attach_pro(dbug,atoi(str[1].data));

    return 0;
}

ngx_array_t *get_command(struct dbug_struct *dbug)
{
    char buf[50] = {0};

    putc('>',stdout);
    fgets(buf,sizeof(buf),stdin);

    ngx_array_t *cmds = ngx_array_create(dbug->pool,10,sizeof(ngx_str_t));
    ngx_str_t *str = NULL;
    char *p_data = NULL;
    int len = strlen(buf);
    for (int i = 0;i < len;i++) {
        if (buf[i] == ' ') continue;
        for (int j = i;j < len;j++) {
            if (isspace(buf[j])) {
                int size = j - i + 1;
                p_data = ngx_pcalloc(dbug->pool,size);
                memcpy(p_data,buf + i,j - i);
                p_data[size - 1] = 0;
                str = ngx_array_push(cmds);
                //ngx_str_set(str, p_data);
                str->data = p_data;
                str->len = strlen(p_data);
                //printf("len = %ld %s\n",strlen(p_data),p_data);
                i = j;
                break;
            }
        }
    }

    return cmds;
}

int play_command(struct dbug_struct *dbug,ngx_array_t *cmds)
{
    if (!cmds) return -1;

    struct command *it = cmd_set;
    ngx_str_t *str = (ngx_str_t *)cmds->elts;
    for (;it->name;it++) {
        if (str->len == strlen(it->name) && memcmp(str->data,it->name,str->len) == 0) {
            it->fun(dbug,cmds);
            return 0;
        }
    }

    printf("not command %s\n",str->data);
    return -1;
}