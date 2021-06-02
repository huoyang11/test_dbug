#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "ngx/config.h"
#include "elf_parse.h"
#include "dbug.h"

typedef int (*command_fun)(struct dbug_struct *,void *);

struct command
{
    char *name;
    command_fun fun;
};

int command_break(struct dbug_struct *dbug,void *arg);
int command_continue(struct dbug_struct *dbug,void *arg);
int command_show(struct dbug_struct *dbug,void *arg);
int command_dbug(struct dbug_struct *dbug,void *arg);
int command_attach(struct dbug_struct *dbug,void *arg);
int command_showreg(struct dbug_struct *dbug,void *arg);
int get_command(struct dbug_struct *dbug);
int play_command(struct dbug_struct *dbug);

#endif