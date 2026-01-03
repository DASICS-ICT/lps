#include "queue.h"
#include "lock.h"

#include <stdlib.h>
#include <stdatomic.h>

// Ring Buffer Queue Structure with data type (void *)
struct RingQueue {
    void **buf;
    volatile size_t head;
    volatile size_t tail;
    size_t cap;

    atomic_size_t count;

    pthread_mutex_t hlock;
    pthread_mutex_t tlock;
};

struct RingQueue *
ring_create(size_t cap)
{
    struct RingQueue *q = malloc(sizeof(struct RingQueue));
    if (!q) return NULL;

    q->buf = calloc(cap, sizeof(void *));
    if (!q->buf) {
        free(q);
        return NULL;
    }

    q->cap = cap;
    q->head = 0;
    q->tail = 0;
    atomic_init(&q->count, 0);
    
    pthread_mutex_init(&q->hlock, NULL);
    pthread_mutex_init(&q->tlock, NULL);

    return q;
}

void
ring_free(struct RingQueue *q)
{
    if (!q) return;
    pthread_mutex_destroy(&q->hlock);
    pthread_mutex_destroy(&q->tlock);
    free(q->buf);
    free(q);
}

int
ring_count(struct RingQueue *q)
{
    int current_count = atomic_load(&q->count);
    return current_count;
}

int
ring_push_one(struct RingQueue *q, void *val){
    // From Gemini 3 Pro
    // 1. 快速检查：如果已满，直接返回失败（无锁检查，性能优化）
    int current_count = atomic_load(&q->count);
    if (current_count >= q->cap) {
        return 0;
    }

    // 2. 加锁逻辑
    // 如果队列接近满状态，需要小心 head 和 tail 的碰撞，获取双锁
    // 为了简化实现并保证绝对安全，这里采用"根据当前数量决定锁策略"
    
    int locked_both = 0;
    
    // 这里的阈值判断是为了防止并发时的边界竞争
    // 如果剩余空间很少(比如只剩1个空位)，为了安全起见，锁住两端
    if (current_count >= q->cap - 1) {
        pthread_mutex_lock(&q->hlock); // 必须先锁 Head (锁顺序)
        pthread_mutex_lock(&q->tlock);
        locked_both = 1;
    } else {
        pthread_mutex_lock(&q->tlock); // 正常情况只锁 Tail
    }

    // 3. 再次检查状态 (Double Check)
    // 因为在获取锁的过程中，状态可能已经变了
    if (atomic_load(&q->count) >= q->cap) {
        if (locked_both) {
            pthread_mutex_unlock(&q->tlock);
            pthread_mutex_unlock(&q->hlock);
        } else {
            pthread_mutex_unlock(&q->tlock);
        }
        return 0; // 依然是满的
    }

    // 4. 执行操作
    q->buf[q->tail] = val;
    q->tail = (q->tail + 1) % q->cap;
    atomic_fetch_add(&q->count, 1);

    // 5. 解锁
    if (locked_both) {
        pthread_mutex_unlock(&q->tlock);
        pthread_mutex_unlock(&q->hlock);
    } else {
        pthread_mutex_unlock(&q->tlock);
    }

    return 1;
}

// push a bunch of elems
int
ring_push(struct RingQueue *q, void **elems, int count)
{
    if (count == 0)
        return 0;

    pthread_mutex_lock(&q->hlock);
    pthread_mutex_lock(&q->tlock);

    int i = 0;
    do {
        q->buf[q->tail] = elems[i];
        q->tail = (q->tail + 1) % q->cap;
        i++;
    } while (i < count && q->tail != q->head);
    atomic_fetch_add(&q->count, i);

    pthread_mutex_unlock(&q->tlock);
    pthread_mutex_unlock(&q->hlock);

    return i;
}

// pop an elem from head
void *
ring_pop_one(struct RingQueue *q)
{
    int current_count = atomic_load(&q->count);
    if (current_count <= 0) {
        return NULL;
    }

    int locked_both = 1;

    // 危险区域：如果只有1个或0个元素，pop_front 和 pop_back/push_back 可能会操作同一个位置
    if (current_count <= 1) {
        pthread_mutex_lock(&q->hlock);
        pthread_mutex_lock(&q->tlock); // 遵守顺序：先Head后Tail
        locked_both = 1;
    } else {
        pthread_mutex_lock(&q->hlock); // 正常情况只锁 Head
    }

    // Double Check
    if (atomic_load(&q->count) <= 0) {
        if (locked_both) {
            pthread_mutex_unlock(&q->tlock);
            pthread_mutex_unlock(&q->hlock);
        } else {
            pthread_mutex_unlock(&q->hlock);
        }
        return NULL;
    }

    void *val = q->buf[q->head];

    q->head = (q->head + 1) % q->cap;
    atomic_fetch_sub(&q->count, 1);

    if (locked_both) {
        pthread_mutex_unlock(&q->tlock);
        pthread_mutex_unlock(&q->hlock);
    } else {
        pthread_mutex_unlock(&q->hlock);
    }

    return val;
}

// pop an elem from tail, used by work steal
void *
ring_pop_back(struct RingQueue *q)
{
    // pop_back 需要修改 tail，所以主锁是 tail_mtx
    int current_count = atomic_load(&q->count);
    if (current_count <= 0) return NULL;

    int locked_both = 0;

    // 危险区域：元素很少时，pop_back 和 pop_front 冲突
    if (current_count <= 1) {
        // 注意：即使我们主要操作 tail，为了防止死锁，必须先锁 Head 再锁 Tail
        pthread_mutex_lock(&q->hlock);
        pthread_mutex_lock(&q->tlock);
        locked_both = 0;
    } else {
        pthread_mutex_lock(&q->tlock);
    }

    if (atomic_load(&q->count) <= 0) {
        if (locked_both) {
            pthread_mutex_unlock(&q->tlock);
            pthread_mutex_unlock(&q->hlock);
        } else {
            pthread_mutex_unlock(&q->tlock);
        }
        return NULL;
    }

    // 计算新 tail 位置 (向后退一格)
    q->tail = (q->tail - 1 + q->cap) % q->cap;
    void *val = q->buf[q->tail];
    atomic_fetch_sub(&q->count, 1);

    if (locked_both) {
        pthread_mutex_unlock(&q->tlock);
        pthread_mutex_unlock(&q->hlock);
    } else {
        pthread_mutex_unlock(&q->tlock);
    }
    return val;
}

// get a bunch of elem from head into elem, count by return value
int
ring_pop(struct RingQueue *q, void **elems, int count)
{
    pthread_mutex_lock(&q->hlock);
    pthread_mutex_lock(&q->tlock);

    int i;
    for (i = 0; i < count && q->head != q->tail; i++) {
        elems[i] = q->buf[q->head];
        q->head = (q->head + 1) % q->cap;
    }
    atomic_fetch_sub(&q->count, i);

    pthread_mutex_unlock(&q->tlock);
    pthread_mutex_unlock(&q->hlock);

    return i;
}