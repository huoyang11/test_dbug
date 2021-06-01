#include "config.h"


static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size,
    ngx_uint_t align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

//创建内存池
ngx_pool_t *
ngx_create_pool(size_t size)
{
    ngx_pool_t  *p;
	//malloc 申请系统内存
    p = calloc(1, size);
    if (p == NULL) {
        return NULL;
    }
	
    p->d.last = (u_char *) p + sizeof(ngx_pool_t); 	//指向内存未使用的地方
    p->d.end = (u_char *) p + size;					//内存末尾
    p->d.next = NULL;
    p->d.failed = 0;
	//减去ngx_pool_t本身的长度
    size = size - sizeof(ngx_pool_t);
	//NGX_MAX_ALLOC_FROM_POOL 在mgx_pagesize未初始化的时候未-1 无符号中表示最大
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;			//当前使用的内存池	
    p->large = NULL;
    p->cleanup = NULL;

    return p;
}

//销毁内存池
void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;
	//遍历并调用所有清理函数
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            LOG("run cleanup: %p", c);
            c->handler(c->data);
        }
    }

	//释放大于初始化内存大小的内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }
	//释放所有内存池
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        free(p);

        if (n == NULL) {
            break;
        }
    }
}

//清空使用内存
void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;
	//释放大于初始化内存大小的内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }
	//释放内存,让所有内存池指向最开始未分配的地方
    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }
	//指向第一个内存池
    pool->current = pool;
    pool->large = NULL;
}

//申请内存,使用内存对齐
void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
    if (size <= pool->max) {//小于初始化内存大小的内存
        return ngx_palloc_small(pool, size, 1);
    }

	//大于初始化内存大小的内存
    return ngx_palloc_large(pool, size);
}

//申请内存,不使用内存对齐
void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }

    return ngx_palloc_large(pool, size);
}


static ngx_inline void *
ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_t  *p;

    p = pool->current;

    do {
        m = p->d.last;		//获取内存末尾指针

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT); //设置内存对齐
        }

        if ((size_t) (p->d.end - m) >= size) {   //剩余内存比未分配内存大
            p->d.last = m + size;				 //移动内存末尾指针,返回内存地址

            return m;
        }

        p = p->d.next;						//比未分配内存小,试着让下一块内存分配

    } while (p);
	//如果每一块内存都不足以分配
    return ngx_palloc_block(pool, size);
}


static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;
	//计算内存块大小 尾指针减起始指针
    psize = (size_t) (pool->d.end - (u_char *) pool);
	//malloc
    m = calloc(1, psize);
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ngx_pool_data_t);		//m指向未使用的内存
    m = ngx_align_ptr(m, NGX_ALIGNMENT);//内存对齐
    new->d.last = m + size;				//分配size内存
	//遍历内存块,不遍历最后一块内存块
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {		//超过4次分配内存,current指向下一个内存块
            pool->current = p->d.next;
        }
    }
	//链表尾插
    p->d.next = new;

    return m;							//返回分配的内存
}


static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;
	//malloc
    p = calloc(1,size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;
	//查找前三个链表有没有alloc为空
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }
		
        if (n++ > 3) {
            break;
        }
    }
	//在内存池中申请内存
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        free(p);
        return NULL;
    }

    large->alloc = p;
	//头插法
    large->next = pool->large;
    pool->large = large;

    return p;
}

//向系统申请一块内存,然后挂到large链表
void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;
	//malloc
    p = calloc(1, size);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        free(p);
        return NULL;
    }

    large->alloc = p;
	//头插法
    large->next = pool->large;
    pool->large = large;

    return p;
}

//释放内存(只遍历large链表中的内存)
ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            LOG("free: %p", l->alloc);
            free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}

//申请内存,并且置0
void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        memset(p, 0 ,size);
    }

    return p;
}

//添加一个清理函数内存
ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;
	//申请函数链表内存和data内存
    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
	//头插法
    c->next = p->cleanup;
    p->cleanup = c;

    LOG("add cleanup: %p", c);

    return c;
}

//调用清理函数中与参数fd对应的ngx_pool_cleanup_file
void
ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

//关闭文件描述符
void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    LOG("file cleanup: fd:%d",
                   c->fd);

    if (close(c->fd) == -1) {
        LOG(" \"%s\" failed", c->name);
    }
}

//删除文件,关闭文件描述符
void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    LOG("file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (unlink(c->name) == -1) {
        err = errno;

        if (err != ENOENT) {
            LOG(" \"%s\" failed", c->name);
        }
    }

    if (close(c->fd) == -1) {
        LOG(" \"%s\" failed", c->name);
    }
}