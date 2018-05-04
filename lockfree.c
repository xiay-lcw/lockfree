#include <stdatomic.h>
#include <stdlib.h>

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
    LFSNode* node = (LFSNode*)malloc(sizeof(LFSNode));
    node->data = data;
    LFSHead old_head;
    LFSHead new_head;
    do
    {
        old_head = atomic_load(&stack->head);
        atomic_store(&node->next, old_head.top);
        new_head.top = node;
        new_head.counter = old_head.counter;
    }
    while (!atomic_compare_exchange_strong(&stack->head, &old_head, new_head));
}

bool popLockFreeStack(LFStack* stack, void** data)
{
    LFSHead old_head;
    LFSHead new_head;
    do
    {
        if (!fetchTopLockFreeStack(stack, &old_head)) { return false; }
        new_head.top = atomic_load(&old_head.top->next);
        new_head.counter = old_head.counter+1;
    }
    while (!atomic_compare_exchange_strong(&stack->head, &old_head, new_head));

    *data = old_head.top->data;
    free(old_head.top);
    return true;
}

bool isStackEmpty(LFStack* stack)
{
    return atomic_load(&stack->head).top == NULL;
}

// Low level APIs for LFStack

LFSNode* peelWholeLockFreeStack(LFStack* stack)
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

LFSNode* nextLFSNode(LFSNode* node)
{
    return atomic_load(&node->next);
}
