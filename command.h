#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "ngx/config.h"
#include "elf_parse.h"
#include "dbug.h"

typedef int (*command_fun)(struct dbug_struct *,void *);

struct command
{
    char *name;         //命令名字
    command_fun fun;    //对应的回调
};

//软件断点
int command_break(struct dbug_struct *dbug,void *arg);
//继续执行
int command_continue(struct dbug_struct *dbug,void *arg);
//查看调试进程的函数符号
int command_show(struct dbug_struct *dbug,void *arg);
//debug 程序
int command_dbug(struct dbug_struct *dbug,void *arg);
//attach 程序
int command_attach(struct dbug_struct *dbug,void *arg);
//查看寄存器
int command_showreg(struct dbug_struct *dbug,void *arg);
//硬件断点
int command_watch(struct dbug_struct *dbug,void *arg);
//退出
int command_exit(struct dbug_struct *dbug,void *arg);
//查看断点
int command_showbps(struct dbug_struct *dbug,void *arg);
//获取一个命令
int get_command(struct dbug_struct *dbug);
//执行一个命令
int play_command(struct dbug_struct *dbug);

#endif