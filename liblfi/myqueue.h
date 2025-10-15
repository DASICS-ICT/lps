#ifndef MY_LINUX_QUEUE_H
#define MY_LINUX_QUEUE_H

#include "mylist.h" // 引入我们之前实现的链表

// ----------------- 核心结构和宏 -----------------

/**
 * @brief 队列结构体
 * 包含一个链表头和一个指向尾部节点的指针，以实现 O(1) 入队
 */
struct queue {
    struct list_node head; // 链表头 (虚拟节点)
    struct list_node *tail; // 指向最后一个真实节点
};

/**
 * @brief 声明并初始化一个队列
 * @name: 队列变量名
 */
#define QUEUE_INIT(name) \
    struct queue name = { { NULL }, &(name.head) }

/**
 * @brief 在运行时初始化一个队列
 * @param q: 指向要初始化的队列的指针
 */
static inline void queue_init(struct queue *q) {
    INIT_LIST_HEAD(&q->head);
    q->tail = &q->head;
}

// ----------------- 队列操作函数 -----------------

/**
 * @brief 检查队列是否为空
 * @param q: 指向队列的指针
 * @return 如果为空返回 1, 否则返回 0
 */
static inline int queue_is_empty(const struct queue *q) {
    return list_empty(&q->head);
}

/**
 * @brief 入队操作 (添加到队列尾部)
 * @param q: 指向队列的指针
 * @param new_node: 指向要添加的新节点的 list_node 成员的指针
 */
static inline void queue_enqueue(struct queue *q, struct list_node *new_node) {
    new_node->next = NULL;
    q->tail->next = new_node;
    q->tail = new_node;
}

/**
 * @brief 出队操作 (从队列头部移除)
 * @param q: 指向队列的指针
 * @return 返回被移除的节点的 list_node 指针，如果队列为空则返回 NULL
 */
static inline struct list_node* queue_dequeue(struct queue *q) {
    if (queue_is_empty(q)) {
        return NULL;
    }

    struct list_node *first = q->head.next;
    
    // 更新链表头
    q->head.next = first->next;

    // 关键：如果出队的是最后一个元素，队列将变空
    // 此时必须重置 tail 指针，使其指回虚拟头节点
    if (q->head.next == NULL) {
        q->tail = &q->head;
    }

    first->next = NULL; // 断开被移除节点的连接
    return first;
}

/**
 * @brief 查看队首元素 (不移除)
 * @param q: 指向队列的指针
 * @return 返回队首元素的 list_node 指针，如果队列为空则返回 NULL
 */
static inline struct list_node* queue_peek(struct queue *q) {
    if (queue_is_empty(q)) {
        return NULL;
    }
    return q->head.next;
}

#endif /* MY_LINUX_QUEUE_H */
