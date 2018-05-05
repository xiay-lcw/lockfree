#include <stdatomic.h>
#include <stdlib.h>

#ifdef __linux__
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#endif

#include "lockfree.h"

struct SLNode {
    volatile struct SLNode* next;
    void*   data;   
};

typedef struct LockFreeStackHead {
    volatile LFSNode*  top;
    volatile uint32_t counter;
} LFSHead;

struct LockFreeStack {
    volatile LFSHead head;
    bool enabled;
};

// Internal helper functions for LFStack

static inline bool fetchTopLockFreeStack(LFStack* stack, LFSHead* head)
{
    return (*head = atomic_load(&stack->head)).top != NULL;
}

#ifdef __linux__
static inline int futex(int *uaddr, int futex_op, int val,
                        const struct timespec *timeout,   /* or: uint32_t val2 */
                        int *uaddr2, int val3)
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}
#endif

static inline void futexWait(volatile uint32_t* key, uint32_t current)
{
#ifdef __linux__
    futex(key, FUTEX_WAIT, current, NULL, NULL, 0);
#endif
}

static inline void futexWakeOne(volatile uint32_t* key)
{
#ifdef __linux__
    futex(key, FUTEX_WAKE, 1, NULL, NULL, 0);
#endif
}

static inline void futexWakeAll(volatile uint32_t* key)
{
#ifdef __linux__
    futex(key, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
#endif
}

// Low level APIs for LFStack

#define CREATE_LFS_NODE() (LFSNode*)malloc(sizeof(LFSNode))
#define DESTROY_LFS_NODE(node) free(node)

LFSNode* createLFSNode()
{
    return CREATE_LFS_NODE();
}

void destroyLFSNode(LFSNode* node)
{
    DESTROY_LFS_NODE(node);
}

LFSNode* nextLFSNode(LFSNode* node)
{
    return atomic_load(&node->next);
}

extern void* getLFSNodeData(LFSNode* node)
{
    return node->data;
}

LFSNode* peelOffAll(LFStack* stack)
{
    LFSHead old_head;
    LFSHead new_head;
    do
    {
        if (!fetchTopLockFreeStack(stack, &old_head)) { return NULL; }
        new_head.top = NULL;
        new_head.counter = old_head.counter;
    }
    while (!atomic_compare_exchange_strong(&stack->head, &old_head, new_head));

    return old_head.top;
}

// High level API for LFStack

LFStack* createLockFreeStack()
{
    return (LFStack*)malloc(sizeof(LFStack));
}

void destroyLockFreeStack(LFStack* stack)
{
    free(stack);
}

bool isStackLockFree(LFStack* stack)
{
    return atomic_is_lock_free(&stack->head);
}

void pushLockFreeStack(LFStack* stack, void* data)
{
    LFSNode* node = CREATE_LFS_NODE();
    node->data = data;
    LFSHead old_head;
    LFSHead new_head;
    do
    {
        old_head = atomic_load(&stack->head);
        atomic_store(&node->next, old_head.top);
        new_head.top = node;
        new_head.counter = old_head.counter+1;
    }
    while (!atomic_compare_exchange_strong(&stack->head, &old_head, new_head));

    if (old_head.top == NULL) { futexWakeOne(&stack->head.counter); }
}

bool popLockFreeStack(LFStack* stack, void** data)
{
    LFSHead old_head;
    LFSHead new_head;
    do
    {
        if (!fetchTopLockFreeStack(stack, &old_head)) { return false; }
        new_head.top = atomic_load(&old_head.top->next);
        new_head.counter = old_head.counter;
    }
    while (!atomic_compare_exchange_strong(&stack->head, &old_head, new_head));

    *data = old_head.top->data;
    DESTROY_LFS_NODE(old_head.top);
    return true;
}

bool isStackEmpty(LFStack* stack)
{
    return atomic_load(&stack->head).top == NULL;
}

void waitStackNonEmpty(LFStack* stack)
{
    LFSHead old_head = atomic_load(&stack->head);
    if (old_head.top == NULL)
    {
        futexWait(&stack->head.counter, old_head.counter);
    }
}

void wakeStackWaiters(LFStack* stack)
{
    futexWakeAll(&stack->head.counter);
}
