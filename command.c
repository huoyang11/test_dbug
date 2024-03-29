#include <sys/ptrace.h>
#include <sys/wait.h>
#include <ctype.h>

#include "command.h"
#include "dbug.h"

//命令表
struct command cmd_set[] = {
    {"b",command_break},
    {"c",command_continue},
    {"show",command_show},
    {"dbug",command_dbug},
    {"attach",command_attach},
    {"showreg",command_showreg},
    {"watch",command_watch},
    {"q",command_exit},
    {"exit",command_exit},
    {"quit",command_exit},
    {"showbps",command_showbps},
    {NULL,NULL},
};

int command_break(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    ngx_str_t *str = (ngx_str_t *)((ngx_array_t *)arg)->elts;
    //查找输入的符号
    int index = find_func(dbug->funs,str[1].data);
    if (index == -1) return -1;
    //插入软件断点
    struct breakpoint *bp = ngx_array_push(dbug->bps);
    bp->name = dbug->funs->data[index].name;
    bp->addr = dbug->funs->data[index].addr;
    insert_INT3_bp(dbug->pid,bp);

    return 0;
}

int command_continue(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;
    struct breakpoint *bps = (struct breakpoint *)dbug->bps->elts;//断点数组
    //进程继续执行
    if (ptrace(PTRACE_CONT,dbug->pid,NULL,NULL) < 0) {
        perror("ptrace cont err");
    }

    wait(&dbug->status);
    if (WIFEXITED(dbug->status)) exit(0);

    ptrace(PTRACE_GETREGS,dbug->pid,NULL,&dbug->regs);
    //是否是断点触发
    if (WSTOPSIG(dbug->status) == SIGTRAP) {
        int index = find_current_SF_bp(dbug); //查找是否是软件断点触发
        if (index != -1) {
            printf("trigger breakpoint %lx(%s)\n",bps[index].addr,bps[index].name);
            resume_INT3_bp_next(dbug->pid,&bps[index],&dbug->regs);
        }

        index = find_current_HW_bp(dbug); //查找是否是硬件断点触发
        if (index != -1) {
            printf("trigger HW breakpoint %lx\n",bps[index].addr);
        }
    }

    return 0;
}

int command_show(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;
    //输出进程函数符号
    foreach_fun(dbug->funs);

    return 0;
}

int command_exit(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    kill(dbug->pid,SIGKILL);
    exit(0);

    return 0;
}

int command_showbps(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    int n = dbug->bps->nelts;
    struct breakpoint *bps = (struct breakpoint *)dbug->bps->elts;

    for (int i = 0;i < n;i++) {
        printf("%4d %lx\n",i,bps[i].addr);
    }

    return 0;
}

int command_dbug(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    ngx_array_t *args = (ngx_array_t *)arg;
    ngx_str_t *str = (ngx_str_t *)args->elts;//输入参数
    char **argv = ngx_pcalloc(dbug->pool,sizeof(char *) * (args->nelts));
    //构造进程执行的参数
    for (int i = 0;i < args->nelts;i++) {
        argv[i] = str[i+1].data;
    }

    dbug_pro(dbug,str[1].data,argv);

    return 0;
}

int command_attach(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;
    //附加一个进程
    ngx_str_t *str = (ngx_str_t *)((ngx_array_t *)arg)->elts;
    attach_pro(dbug,atoi(str[1].data));

    return 0;
}

int command_showreg(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;

    struct user_regs_struct *reg = &dbug->regs;
    if (ptrace(PTRACE_GETREGS, dbug->pid, 0, reg) < 0) {
        perror("PTRACE_GETREGS err\n");
    }

    printf("r15:0x%llx\n",reg->r15);
    printf("r14:0x%llx\n",reg->r14);
    printf("r13:0x%llx\n",reg->r13);
    printf("r12:0x%llx\n",reg->r12);
    printf("rbp:0x%llx\n",reg->rbp);
    printf("rbx:0x%llx\n",reg->rbx);
    printf("r11:0x%llx\n",reg->r11);
    printf("r10:0x%llx\n",reg->r10);
    printf("r9:0x%llx\n",reg->r9);
    printf("r8:0x%llx\n",reg->r8);
    printf("rax:0x%llx\n",reg->rax);
    printf("rcx:0x%llx\n",reg->rcx);
    printf("rdx:0x%llx\n",reg->rdx);
    printf("rsi:0x%llx\n",reg->rsi);
    printf("rdi:0x%llx\n",reg->rdi);
    printf("orig_rax:0x%llx\n",reg->orig_rax);
    printf("rip:0x%llx\n",reg->rip);
    printf("cs:0x%llx\n",reg->cs);
    printf("eflags:0x%llx\n",reg->eflags);
    printf("rsp:0x%llx\n",reg->rsp);
    printf("ss:0x%llx\n",reg->ss);
    printf("fs_base:0x%llx\n",reg->fs_base);
    printf("gs_base:0x%llx\n",reg->gs_base);
    printf("ds:0x%llx\n",reg->ds);
    printf("es:0x%llx\n",reg->es);
    printf("fs:0x%llx\n",reg->fs);
    printf("gs:0x%llx\n",reg->gs);

    return 0;
}

int command_watch(struct dbug_struct *dbug,void *arg)
{
    if (!dbug) return -1;
    //插入硬件断点
    ngx_str_t *str = (ngx_str_t *)((ngx_array_t *)arg)->elts;
    struct breakpoint *bp = ngx_array_push(dbug->bps);
    sscanf(str[1].data,"%lx",&bp->addr);
    printf("watch %lx\n",bp->addr);
    insert_hard_break(dbug->pid,bp);

    return 0;
}

int get_command(struct dbug_struct *dbug)
{
    char buf[50] = {0};

    putc('>',stdout);
    fgets(buf,sizeof(buf),stdin);

    ngx_array_t *cmds = dbug->cmd;
    cmds->nelts = 0;//清空数组
    ngx_str_t *str = NULL;
    char *p_data = NULL;
    int len = strlen(buf);
    //以空格分隔命令 分割结果方式dbug->cmd数组里
    for (int i = 0;i < len;i++) {
        if (buf[i] == ' ') continue;//忽略空格
        for (int j = i;j < len;j++) {
            if (isspace(buf[j])) {
                int size = j - i + 1;//字符串长度
                p_data = ngx_pcalloc(dbug->pool,size);
                //拷贝字符串
                memcpy(p_data,buf + i,j - i);
                p_data[size - 1] = 0;
                //添加数组
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

    return 0;
}

int play_command(struct dbug_struct *dbug)
{
    if (!dbug) return -1;

    ngx_array_t *cmds = dbug->cmd;

    struct command *it = cmd_set;
    ngx_str_t *str = (ngx_str_t *)cmds->elts;
    for (;it->name;it++) {
        if (str->len == strlen(it->name) && memcmp(str->data,it->name,str->len) == 0) {//查找命令
            it->fun(dbug,cmds);//命令对应的执行回调
            return 0;
        }
    }

    printf("not command %s\n",str->data);
    return -1;
}