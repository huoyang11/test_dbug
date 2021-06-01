#include "config.h"

#define ngx_cpu_pause()         __asm__ (".byte 0xf3, 0x90")

static ngx_atomic_uint_t
ngx_atomic_cmp_set(ngx_atomic_t *lock, ngx_atomic_uint_t old,
    ngx_atomic_uint_t set)
{
    u_char  res;

    __asm__ volatile (

    "    lock;               "
    "    cmpxchgl  %3, %1;   "
    "    sete      %0;       "

    : "=a" (res) : "m" (*lock), "a" (old), "r" (set) : "cc", "memory");

    return res;
}

static ngx_atomic_int_t
ngx_atomic_fetch_add(ngx_atomic_t *value, ngx_atomic_int_t add)
{
    __asm__ volatile (

    "    lock;            "
    "    xaddl  %0, %1;   "

    : "+r" (add) : "m" (*value) : "cc", "memory");

    return add;
}

static void ngx_shmtx_wakeup(ngx_shmtx_t *mtx);


ngx_int_t
ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr, u_char *name)
{
    mtx->lock = &addr->lock;

    if (mtx->spin == (ngx_uint_t) -1) {
        return NGX_OK;
    }

    mtx->spin = 2048;

    mtx->wait = &addr->wait;
	//1表示进程同步 0是线程同步
    if (sem_init(&mtx->sem, 1, 0) == -1) {
        LOG("sem_init() failed\n");
    } else {
        mtx->semaphore = 1;	//信号量创建成功的标志
    }

    return NGX_OK;
}


void
ngx_shmtx_destroy(ngx_shmtx_t *mtx)
{
    if (mtx->semaphore) {
        if (sem_destroy(&mtx->sem) == -1) {
            LOG("sem_destroy() failed\n");
        }
    }
}

//尝试加锁(自旋锁) 成功返回1
ngx_uint_t
ngx_shmtx_trylock(ngx_shmtx_t *mtx)
{
    return (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, 1));
}

//加锁
void
ngx_shmtx_lock(ngx_shmtx_t *mtx)
{
    ngx_uint_t         i, n;

    LOG("shmtx lock\n");

    for ( ;; ) {
		//使用自旋锁
        if (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, 1)) {
            return;
        }

        for (n = 1; n < mtx->spin; n <<= 1) {

            for (i = 0; i < n; i++) {
                ngx_cpu_pause();
            }

            if (*mtx->lock == 0
                && ngx_atomic_cmp_set(mtx->lock, 0, 1))
            {
                return;
            }
        }
		//自旋锁加锁失败 使用信号量
        if (mtx->semaphore) {
            (void) ngx_atomic_fetch_add(mtx->wait, 1); //等待+1
			//尝试加锁
            if (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, 1)) {
                (void) ngx_atomic_fetch_add(mtx->wait, -1);//加锁成功等待-1
                return;
            }

            LOG( "shmtx wait %d\n", *mtx->wait);
			//使用信号量进入休眠
            while (sem_wait(&mtx->sem) == -1) {
                ngx_err_t  err;

                err = errno;

                if (err != EINTR) {
                    LOG("sem_wait() failed while waiting on shmtx\n");
                    break;
                }
            }

            LOG("shmtx awoke\n");

            continue;
        }
		//让出cpu(没有使用信号量才会执行)
        sched_yield();
    }
}


void
ngx_shmtx_unlock(ngx_shmtx_t *mtx)
{
    if (mtx->spin != (ngx_uint_t) -1) {
        LOG("shmtx unlock\n");
    }
	//自旋锁解锁 => mtx->lock = 0
    if (ngx_atomic_cmp_set(mtx->lock, 1, 0)) {
        ngx_shmtx_wakeup(mtx);//如果使用了信号量则唤醒其他进程
    }
}


ngx_uint_t
ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid)
{
    LOG("shmtx forced unlock\n");

    if (ngx_atomic_cmp_set(mtx->lock, pid, 0)) {
        ngx_shmtx_wakeup(mtx);
        return 1;
    }

    return 0;
}


static void
ngx_shmtx_wakeup(ngx_shmtx_t *mtx)
{
    ngx_atomic_uint_t wait;

    if (!mtx->semaphore) {
        return;
    }

    for ( ;; ) {

        wait = *mtx->wait;

        if ((int) wait <= 0) {
            return;
        }
		//等待-1
        if (ngx_atomic_cmp_set(mtx->wait, wait, wait - 1)) {
            break;
        }
    }

    LOG("shmtx wake %d\n", wait);
	//信号量-1
    if (sem_post(&mtx->sem) == -1) {
        LOG("sem_post() failed while wake shmtx\n");
    }
}