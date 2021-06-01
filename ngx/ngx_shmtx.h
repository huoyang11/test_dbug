#ifndef _NGX_SHMTX_H_INCLUDED_
#define _NGX_SHMTX_H_INCLUDED_

#include "config.h"

typedef struct {
    ngx_atomic_t   lock;
    ngx_atomic_t   wait;
} ngx_shmtx_sh_t;


typedef struct {
    ngx_atomic_t  *lock;		//自旋锁
    ngx_atomic_t  *wait;		//信号量的数量 和 sem同步的值
    ngx_uint_t     semaphore;	//信号量创建成功的标志
    sem_t          sem;			//信号量
    ngx_uint_t     spin;		//自旋锁的等待因子,一次轮完之后没有加锁成功就进入休眠
} ngx_shmtx_t;


ngx_int_t ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr,
    u_char *name);
void ngx_shmtx_destroy(ngx_shmtx_t *mtx);
ngx_uint_t ngx_shmtx_trylock(ngx_shmtx_t *mtx);
void ngx_shmtx_lock(ngx_shmtx_t *mtx);
void ngx_shmtx_unlock(ngx_shmtx_t *mtx);
ngx_uint_t ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid);


#endif
