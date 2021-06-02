#ifndef _MEM_H_
#define _MEM_H_

#include <unistd.h>
#include <stdint.h>

int read_pro_mem(pid_t child,uint64_t addr,char * str,int len);

int write_pro_mem(pid_t child,uint64_t addr,char * str,int len);

#endif