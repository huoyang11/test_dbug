#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_

#include "config.h"

#define NGX_MAX_ALLOC_FROM_POOL  (4096 - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;
//清理函数链表
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;
    void                 *data;
    ngx_pool_cleanup_t   *next;
};

typedef struct ngx_pool_s        ngx_pool_t;
typedef struct ngx_pool_large_s  ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;
    void                 *alloc;
};

typedef struct {
    u_char               *last;			//指向当前申请内存的末尾
    u_char               *end;			//内存末尾位置
    ngx_pool_t           *next;			//下一个内存池
    ngx_uint_t            failed;
}ngx_pool_data_t;

struct ngx_pool_s {
    ngx_pool_data_t       d;
    size_t                max;			//能在内存块中分配的最大内存大小
    ngx_pool_t           *current;		//当前使用的内存池
    ngx_pool_large_t     *large;		//>max的内存或者使用ngx_pmemalign函数分配内存的集合
    ngx_pool_cleanup_t   *cleanup;		//清理函数集合
};

//用于做清理函数的data
typedef struct {
    int                   fd;
    u_char               *name;
}ngx_pool_cleanup_file_t;

//创建、销毁、重置
ngx_pool_t *ngx_create_pool(size_t size);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);    //内存对齐,内存申请
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);   //不使用对齐
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);   //申请并且清0
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);   //向系统申请一块内存,然后挂到large链表
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);     //释放large链表中p地址内存

ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);   //添加一个清理函数,在内存池销毁的时候调用
void ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd); //调用清理函数中与参数fd对应的ngx_pool_cleanup_file()函数
void ngx_pool_cleanup_file(void *data);                //关闭文件描述符
void ngx_pool_delete_file(void *data);                 //删除文件,关闭文件描述符


#endif /* _NGX_PALLOC_H_INCLUDED_ */
