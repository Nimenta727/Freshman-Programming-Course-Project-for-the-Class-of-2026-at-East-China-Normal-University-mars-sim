#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include <stddef.h>

/* 事件/基地状态中允许出现的五种资源 */
typedef enum {
    RES_POWER = 0,
    RES_OXYGEN,
    RES_WATER,
    RES_TEMPERATURE,
    RES_HULL,
    RES_INVALID = -1
} Resource;

/* 基地状态 */
typedef struct {
    int power;          /* 电力 */
    int oxygen;         /* 氧气 */
    int water;          /* 水资源 */
    int temperature;    /* 温度 */
    int hull;           /* 舱体完整度 0-100 */
    int robot;          /* 维修机器人总数（来自 base.txt，仿真中不减少） */
    int active_robot;   /* 是否已有机器人被激活：0 或 1 */
    int lab_on;         /* 实验舱是否运行：初始 1，SHUTDOWN LAB 后为 0 */
    int fault_solar;    /* SOLAR 故障 */
    int fault_oxygen;   /* OXYGEN 故障 */
    int fault_comm;     /* COMMUNICATION 故障 */
    int repaired;       /* 成功维修次数 */
    int failed_action;  /* 行动失败次数 */
    int time;           /* 已处理的时间片数 */
    int failed;         /* 是否已失败 */
} State;

/* 读取 base.txt 并初始化状态。
 * 返回 0 成功；1 运行时错误（如内存）；2 输入错误。失败时填写 err。 */
int state_load_base(const char *path, State *st, char *err, size_t err_size);

/* 事件造成的资源变化（此阶段不做截断） */
void state_apply_delta(State *st, Resource res, int delta);

/* 时间片结算：LAB_ON=1 时 OXYGEN-2、WATER-1；LAB_ON=0 时 OXYGEN-1、WATER 不变 */
void state_settle(State *st);

/* 截断：POWER/OXYGEN/WATER >= 0；HULL 限制在 0-100；TEMPERATURE 不截断 */
void state_clamp(State *st);

/* 失败判定：OXYGEN=0 或 HULL=0 或 TEMPERATURE<0 */
bool state_check_failure(const State *st);

/* 得分：TIME + 10*REPAIRED - 5*FAILED_ACTION + POWER/10 + OXYGEN/10 + WATER/10 + HULL/10
 * C 语言整数除法；结果小于 0 时输出 0 */
int state_score(const State *st);

/* 资源名 -> Resource；未知返回 RES_INVALID */
Resource resource_from_name(const char *name);

#endif
