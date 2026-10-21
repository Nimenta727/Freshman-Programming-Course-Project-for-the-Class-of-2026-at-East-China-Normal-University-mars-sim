#ifndef ACTION_H
#define ACTION_H

#include <stddef.h>

#include "state.h"

/* 六种合法行动 */
typedef enum {
    ACT_WAIT,
    ACT_ACTIVATE_ROBOT,
    ACT_REPAIR_SOLAR,
    ACT_REPAIR_OXYGEN,
    ACT_RESTART_COMMUNICATION,
    ACT_SHUTDOWN_LAB
} ActionType;

typedef struct {
    ActionType type;
} Action;

/* 读取 actions.txt。成功时分配并填充 *out_actions（调用方负责 free），
 * 行动数量写入 *out_m。
 * 返回 0 成功；1 运行时错误（如内存）；2 输入错误。失败时填写 err。 */
int action_load_all(const char *path, Action **out_actions, int *out_m,
                    char *err, size_t err_size);

/* 检查条件并执行行动；行动失败只把 FAILED_ACTION 加 1，不改变其他状态 */
void action_execute(const Action *act, State *st);

#endif
