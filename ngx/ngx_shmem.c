#include "config.h"

ngx_int_t
ngx_shm_alloc(ngx_shm_t *shm)
{
    shm->addr = (u_char *) mmap(NULL, shm->size,
                                PROT_READ|PROT_WRITE,
                                MAP_ANON|MAP_SHARED, -1, 0);

    if (shm->addr == MAP_FAILED) {
        LOG("mmap(MAP_ANON|MAP_SHARED, %lu) failed\n", shm->size);
        return NGX_ERROR;
    }

    return NGX_OK;
}

void
ngx_shm_free(ngx_shm_t *shm)
{
    if (munmap((void *) shm->addr, shm->size) == -1) {
        LOG("munmap(%p, %lu) failed\n", shm->addr, shm->size);
    }
}