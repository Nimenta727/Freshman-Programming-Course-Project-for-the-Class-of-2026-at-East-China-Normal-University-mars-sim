#ifndef EVENT_H
#define EVENT_H

#include <stddef.h>

#include "state.h"

/* 六种合法事件 */
typedef enum {
    EV_SOLAR_FAULT,          /* 变化资源并激活 SOLAR 故障 */
    EV_OXYGEN_LEAK,          /* 变化资源并激活 OXYGEN 故障 */
    EV_COMMUNICATION_LOSS,   /* 变化资源并激活 COMMUNICATION 故障 */
    EV_DUST_STORM,           /* 只变化资源 */
    EV_WATER_CONTAMINATION,  /* 只变化资源 */
    EV_METEOR_STRIKE         /* 只变化资源 */
} EventType;

typedef struct {
    Resource res;
    int delta;
} EventDelta;

typedef struct {
    EventType type;
    EventDelta deltas[3];   /* 每行 1 到 3 组资源变化 */
    int delta_count;
} Event;

/* 读取 events.txt。成功时分配并填充 *out_events（调用方负责 free），
 * 事件数量写入 *out_n。
 * 返回 0 成功；1 运行时错误（如内存）；2 输入错误。失败时填写 err。 */
int event_load_all(const char *path, Event **out_events, int *out_n,
                   char *err, size_t err_size);

/* 把本时间片事件应用到状态：逐项相加资源变化，故障事件同时激活故障 */
void event_apply(const Event *ev, State *st);

#endif
