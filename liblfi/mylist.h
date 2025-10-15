#ifndef MY_LINUX_LIST_H
#define MY_LINUX_LIST_H

#include <stddef.h> // For offsetof

// ----------------- 核心结构和宏 -----------------

/**
 * @brief 单向链表节点
 * 这个结构体将被嵌入到你的自定义结构体中
 */
struct list_node {
    struct list_node *next;
};

/**
 * @brief 声明并初始化一个链表头
 * @name: 链表头的变量名
 */
#define LIST_HEAD(name) \
    struct list_node name = { NULL }

/**
 * @brief 在运行时初始化一个链表头
 * @param head: 指向链表头的指针
 */
#define INIT_LIST_HEAD(head) \
    do { (head)->next = NULL; } while (0)

// ----------------- `container_of` 宏 -----------------

/**
 * @brief 根据结构体成员指针获取整个结构体的指针
 * @ptr: 指向成员的指针
 * @type: 包含该成员的结构体类型
 * @member: 成员在该结构体中的名称
 */
#define container_of(ptr, type, member) ({ \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type, member) ); \
})

/**
 * @brief 一个简化的宏，通过链表节点指针直接获取包含它的结构体
 */
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

// ----------------- 链表操作函数/宏 -----------------

/**
 * @brief 在链表头部添加一个新节点
 * @param new_node: 指向新节点的指针
 * @param head: 指向链表头的指针
 */
static inline void list_add(struct list_node *new_node, struct list_node *head) {
    new_node->next = head->next;
    head->next = new_node;
}

/**
 * @brief 删除一个节点 (单向链表实现，效率为 O(n))
 * 注意：这与 Linux 内核的双向链表 O(1) 删除不同。
 * @param entry: 要删除的节点
 * @param head: 链表头
 */
static inline void list_del(struct list_node *entry, struct list_node *head) {
    struct list_node *prev = head;
    // 遍历查找要删除节点的前一个节点
    while (prev->next != NULL && prev->next != entry) {
        prev = prev->next;
    }
    // 如果找到了，就跳过它
    if (prev->next == entry) {
        prev->next = entry->next;
    }
}

/**
 * @brief 检查链表是否为空
 * @return 如果为空返回 1, 否则返回 0
 */
static inline int list_empty(const struct list_node *head) {
    return head->next == NULL;
}

// ----------------- 链表遍历宏 -----------------

/**
 * @brief 遍历链表 (遍历的是 list_node)
 * @pos: 一个 `struct list_node *` 类型的游标
 * @head: 指向链表头的指针
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != NULL; pos = pos->next)

/**
 * @brief 遍历链表 (遍历的是包含 list_node 的自定义结构体)
 * 这是最常用的遍历方式。
 * @pos: 一个指向你的自定义结构体的指针，用作循环游标
 * @head: 指向链表头的指针
 * @member: list_node 在你的自定义结构体中的成员名
 */
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != NULL; \
         pos = list_entry(pos->member.next, typeof(*pos), member)) \
    if (&pos->member != (head)->next && (head)->next == NULL) break; else /* 处理空链表进入循环的边界情况 */

#endif /* MY_LINUX_LIST_H */
