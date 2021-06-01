#ifndef _ELF_PARSE_H_
#define _ELF_PARSE_H_

#include <unistd.h>
#include <stdint.h>

struct func_org
{
    char *name;
    uint64_t addr;
};

struct fun_arr
{
    char *word;
    int size;
    struct func_org data[0];
};

struct fun_arr * open_funs(pid_t pid);

int find_func(struct fun_arr *funs,const char *func);

void foreach_fun(struct fun_arr *funs);

int close_funs(struct fun_arr *funs);

#endif