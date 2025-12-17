#include "sys/sys.h"

typedef enum {
    YIELD = 0,
} RuntimeCallType;

int
sys_runtime_call(struct LPSThread *t, int type)
{
    switch (type) {
    case YIELD:
        lps_thread_exit(t);
        break;
    default:
        return -LINUX_EINVAL;
    }
    return 0;
}