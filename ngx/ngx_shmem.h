#ifndef _NGX_SHMEM_H_INCLUDED_
#define _NGX_SHMEM_H_INCLUDED_

#include "config.h"

typedef struct {
    u_char      *addr;		//共享内存地址
    size_t       size;		//内存大小
    ngx_str_t    name;		//共享内存名字
    ngx_uint_t   exists;   /* unsigned  exists:1;  */
} ngx_shm_t;


ngx_int_t ngx_shm_alloc(ngx_shm_t *shm);
void ngx_shm_free(ngx_shm_t *shm);

#endif 