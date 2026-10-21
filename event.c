#include "event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

static const struct {
    const char *name;
    EventType type;
} kEventTable[] = {
    {"SOLAR_FAULT",         EV_SOLAR_FAULT},
    {"OXYGEN_LEAK",         EV_OXYGEN_LEAK},
    {"COMMUNICATION_LOSS",  EV_COMMUNICATION_LOSS},
    {"DUST_STORM",          EV_DUST_STORM},
    {"WATER_CONTAMINATION", EV_WATER_CONTAMINATION},
    {"METEOR_STRIKE",       EV_METEOR_STRIKE},
};

static bool event_type_from_name(const char *name, EventType *out)
{
    for (size_t i = 0; i < sizeof(kEventTable) / sizeof(kEventTable[0]); i++) {
        if (strcmp(name, kEventTable[i].name) == 0) {
            *out = kEventTable[i].type;
            return true;
        }
    }
    return false;
}

int event_load_all(const char *path, Event **out_events, int *out_n,
                   char *err, size_t err_size)
{
    *out_events = NULL;
    *out_n = 0;

    LineReader rd;
    if (!reader_open(&rd, path)) {
        snprintf(err, err_size, "无法打开文件 %s", path);
        return 2;
    }

    /* 第一条非空记录：事件数量 N，范围 1-72 */
    int r = reader_next_record(&rd);
    if (r != 1) {
        snprintf(err, err_size, "%s：缺少事件数量行", path);
        reader_close(&rd);
        return 2;
    }
    int n;
    {
        char *tokens[PARSE_MAX_TOKENS];
        int nt = reader_tokenize(&rd, tokens, PARSE_MAX_TOKENS);
        if (nt != 1 || !parse_int_strict(tokens[0], &n) || n < 1 || n > 72) {
            snprintf(err, err_size, "%s 第 %d 行：事件数量错误（应为 1-72 的整数）",
                     path, rd.lineno);
            reader_close(&rd);
            return 2;
        }
    }

    Event *events = malloc((size_t)n * sizeof(Event));
    if (events == NULL) {
        snprintf(err, err_size, "内存分配失败");
        reader_close(&rd);
        return 1;
    }

    int count = 0;
    while ((r = reader_next_record(&rd)) == 1) {
        if (count >= n) {
            snprintf(err, err_size, "%s 第 %d 行：事件记录数量超过声明的 %d",
                     path, rd.lineno, n);
            free(events);
            reader_close(&rd);
            return 2;
        }

        char *tokens[PARSE_MAX_TOKENS];
        int nt = reader_tokenize(&rd, tokens, PARSE_MAX_TOKENS);

        /* 事件名 + 1 到 3 组资源变化：token 总数只能是 3、5、7 */
        if (nt < 3 || nt > 7 || nt % 2 == 0) {
            snprintf(err, err_size,
                     "%s 第 %d 行：事件格式错误（资源名与变化量需成对出现，1 到 3 组）",
                     path, rd.lineno);
            free(events);
            reader_close(&rd);
            return 2;
        }

        Event ev;
        ev.delta_count = (nt - 1) / 2;
        if (!event_type_from_name(tokens[0], &ev.type)) {
            snprintf(err, err_size, "%s 第 %d 行：未知事件名 %s",
                     path, rd.lineno, tokens[0]);
            free(events);
            reader_close(&rd);
            return 2;
        }

        for (int i = 0; i < ev.delta_count; i++) {
            const char *res_name = tokens[1 + i * 2];
            const char *delta_str = tokens[2 + i * 2];
            Resource res = resource_from_name(res_name);
            if (res == RES_INVALID) {
                snprintf(err, err_size, "%s 第 %d 行：未知资源名 %s",
                         path, rd.lineno, res_name);
                free(events);
                reader_close(&rd);
                return 2;
            }
            int delta;
            if (!parse_int_strict(delta_str, &delta)) {
                snprintf(err, err_size, "%s 第 %d 行：资源变化量不是整数 %s %s",
                         path, rd.lineno, res_name, delta_str);
                free(events);
                reader_close(&rd);
                return 2;
            }
            ev.deltas[i].res = res;
            ev.deltas[i].delta = delta;
        }

        events[count++] = ev;
    }

    reader_close(&rd);

    if (r == -1) {
        snprintf(err, err_size, "%s：行过长或读取错误", path);
        free(events);
        return 2;
    }
    if (count < n) {
        snprintf(err, err_size, "%s：事件记录数量不足，声明 %d 实际 %d",
                 path, n, count);
        free(events);
        return 2;
    }

    *out_events = events;
    *out_n = n;
    return 0;
}

void event_apply(const Event *ev, State *st)
{
    for (int i = 0; i < ev->delta_count; i++) {
        state_apply_delta(st, ev->deltas[i].res, ev->deltas[i].delta);
    }

    /* 故障事件同时激活对应故障 */
    switch (ev->type) {
    case EV_SOLAR_FAULT:        st->fault_solar = 1;  break;
    case EV_OXYGEN_LEAK:        st->fault_oxygen = 1; break;
    case EV_COMMUNICATION_LOSS: st->fault_comm = 1;   break;
    default:                                          break;
    }
}
