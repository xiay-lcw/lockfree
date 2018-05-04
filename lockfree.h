#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hight level API

struct LockFreeStack;
typedef struct LockFreeStack LFStack;

extern LFStack* createLockFreeStack();
extern void destroyLockFreeStack(LFStack* stack);

extern bool isStackLockFree(LFStack* stack);
extern bool isStackEmpty(LFStack* stack);

extern void pushLockFreeStack(LFStack* stack, void* data);
extern bool popLockFreeStack(LFStack* stack, void** data);

// Low level API

struct SLNode;
typedef struct SLNode LFSNode;

extern LFSNode* peelWholeLockFreeStack(LFStack* stack);
extern LFSNode* nextLFSNode(LFSNode* node);

#ifdef __cplusplus
}
#endif
