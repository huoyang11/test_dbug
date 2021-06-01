#include "config.h"

ngx_array_t *
ngx_array_create(ngx_pool_t *p, ngx_uint_t n, size_t size)
{
    ngx_array_t *a;
	//分配内存
    a = ngx_palloc(p, sizeof(ngx_array_t));
    if (a == NULL) {
        return NULL;
    }
	//为ngx_array_t->elts分配内存,初始化nalloc,size,nelts
    if (ngx_array_init(a, p, n, size) != NGX_OK) {
        return NULL;
    }

    return a;
}

void
ngx_array_destroy(ngx_array_t *a)
{
    ngx_pool_t  *p;

    p = a->pool;
	//当前内存池指向的地址刚好是数组的末尾
    if ((u_char *) a->elts + a->size * a->nalloc == p->d.last) {
        p->d.last -= a->size * a->nalloc;
    }
	//当前内存池指向的地址刚好是ngx_array_t的末尾
    if ((u_char *) a + sizeof(ngx_array_t) == p->d.last) {
        p->d.last = (u_char *) a;
    }
}

void *
ngx_array_push(ngx_array_t *a)
{
    void        *elt, *new;
    size_t       size;
    ngx_pool_t  *p;
	//使用元素达到最大
    if (a->nelts == a->nalloc) {

		//当前内存大小
        size = a->size * a->nalloc;

        p = a->pool;
		//当前内存池指向的地址刚好是数组的末尾,并且内存池剩余空间比数组元素大小大
        if ((u_char *) a->elts + size == p->d.last
            && p->d.last + a->size <= p->d.end)
        {
			//分配一个元素的内存
            p->d.last += a->size;
            a->nalloc++;//最大元素个数+1

        } else {
            /* allocate a new array */
			//重新分配2被的内存,拷贝之前的数据
            new = ngx_palloc(p, 2 * size);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, a->elts, size);
            a->elts = new;
            a->nalloc *= 2;
        }
    }
	//得到一个未使用的地址
    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts++;//使用元素+1

    return elt;
}

void *
ngx_array_push_n(ngx_array_t *a, ngx_uint_t n)
{
    void        *elt, *new;
    size_t       size;
    ngx_uint_t   nalloc;
    ngx_pool_t  *p;
	//要分配的内存大小
    size = n * a->size;
	//剩余元素是否足(下面逻辑与ngx_array_push类似)
    if (a->nelts + n > a->nalloc) {

        p = a->pool;

        if ((u_char *) a->elts + a->size * a->nalloc == p->d.last
            && p->d.last + size <= p->d.end)
        {
            p->d.last += size;
            a->nalloc += n;

        } else {

            nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

            new = ngx_palloc(p, nalloc * a->size);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, a->elts, a->nelts * a->size);
            a->elts = new;
            a->nalloc = nalloc;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts += n;

    return elt;
}
