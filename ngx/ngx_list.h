#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include "config.h"

typedef struct ngx_list_part_s  ngx_list_part_t;

struct ngx_list_part_s {
    void             *elts;		//内存
    ngx_uint_t        nelts;	//当前使用的元素个数
    ngx_list_part_t  *next;
};


typedef struct {
    ngx_list_part_t  *last;		//指向链表的最后一个节点
    ngx_list_part_t   part;		//头节点
    size_t            size;		//节点元素的大小
    ngx_uint_t        nalloc;	//节点最大元素个数
    ngx_pool_t       *pool;		//内存池
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}

void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
