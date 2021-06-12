#ifndef _MEM_H_
#define _MEM_H_

#include <unistd.h>
#include <stdint.h>
//读取一个进程内存的值
int read_pro_mem(pid_t child,uint64_t addr,char *str,int len);
//写入一个进程内存的值
int write_pro_mem(pid_t child,uint64_t addr,char *str,int len);

#endif