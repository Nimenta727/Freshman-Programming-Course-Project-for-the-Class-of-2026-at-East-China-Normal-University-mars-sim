#include "action.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

/* 把一行行动文本解析为 ActionType */
static bool action_parse_line(char **tokens, int nt, ActionType *out)
{
    if (nt == 1 && strcmp(tokens[0], "WAIT") == 0) {
        *out = ACT_WAIT;
        return true;
    }
    if (nt == 2 && strcmp(tokens[0], "ACTIVATE") == 0 &&
        strcmp(tokens[1], "ROBOT") == 0) {
        *out = ACT_ACTIVATE_ROBOT;
        return true;
    }
    if (nt == 2 && strcmp(tokens[0], "REPAIR") == 0 &&
        strcmp(tokens[1], "SOLAR") == 0) {
        *out = ACT_REPAIR_SOLAR;
        return true;
    }
    if (nt == 2 && strcmp(tokens[0], "REPAIR") == 0 &&
        strcmp(tokens[1], "OXYGEN") == 0) {
        *out = ACT_REPAIR_OXYGEN;
        return true;
    }
    if (nt == 2 && strcmp(tokens[0], "RESTART") == 0 &&
        strcmp(tokens[1], "COMMUNICATION") == 0) {
        *out = ACT_RESTART_COMMUNICATION;
        return true;
    }
    if (nt == 2 && strcmp(tokens[0], "SHUTDOWN") == 0 &&
        strcmp(tokens[1], "LAB") == 0) {
        *out = ACT_SHUTDOWN_LAB;
        return true;
    }
    return false;
}

int action_load_all(const char *path, Action **out_actions, int *out_m,
                    char *err, size_t err_size)
{
    *out_actions = NULL;
    *out_m = 0;

    LineReader rd;
    if (!reader_open(&rd, path)) {
        snprintf(err, err_size, "无法打开文件 %s", path);
        return 2;
    }

    /* 第一条非空记录：行动数量 M */
    int r = reader_next_record(&rd);
    if (r != 1) {
        snprintf(err, err_size, "%s：缺少行动数量行", path);
        reader_close(&rd);
        return 2;
    }
    int m;
    {
        char *tokens[PARSE_MAX_TOKENS];
        int nt = reader_tokenize(&rd, tokens, PARSE_MAX_TOKENS);
        if (nt != 1 || !parse_int_strict(tokens[0], &m) || m < 0) {
            snprintf(err, err_size, "%s 第 %d 行：行动数量错误（应为非负整数）",
                     path, rd.lineno);
            reader_close(&rd);
            return 2;
        }
    }

    Action *actions = malloc((size_t)(m > 0 ? m : 1) * sizeof(Action));
    if (actions == NULL) {
        snprintf(err, err_size, "内存分配失败");
        reader_close(&rd);
        return 1;
    }

    int count = 0;
    while ((r = reader_next_record(&rd)) == 1) {
        if (count >= m) {
            snprintf(err, err_size, "%s 第 %d 行：行动记录数量超过声明的 %d",
                     path, rd.lineno, m);
            free(actions);
            reader_close(&rd);
            return 2;
        }

        char *tokens[PARSE_MAX_TOKENS];
        int nt = reader_tokenize(&rd, tokens, PARSE_MAX_TOKENS);
        if (nt <= 0 || !action_parse_line(tokens, nt, &actions[count].type)) {
            snprintf(err, err_size, "%s 第 %d 行：未知行动", path, rd.lineno);
            free(actions);
            reader_close(&rd);
            return 2;
        }
        count++;
    }

    reader_close(&rd);

    if (r == -1) {
        snprintf(err, err_size, "%s：行过长或读取错误", path);
        free(actions);
        return 2;
    }
    if (count < m) {
        snprintf(err, err_size, "%s：行动记录数量不足，声明 %d 实际 %d",
                 path, m, count);
        free(actions);
        return 2;
    }

    *out_actions = actions;
    *out_m = m;
    return 0;
}

void action_execute(const Action *act, State *st)
{
    switch (act->type) {
    case ACT_WAIT:
        /* 始终合法，不直接改变基地状态 */
        break;

    case ACT_ACTIVATE_ROBOT:
        if (st->robot > 0 && st->active_robot == 0) {
            st->active_robot = 1;
        } else {
            st->failed_action++;
        }
        break;

    case ACT_REPAIR_SOLAR:
        if (st->fault_solar && st->active_robot == 1 && st->power >= 10) {
            st->power -= 10;
            st->fault_solar = 0;
            st->repaired++;
        } else {
            st->failed_action++;
        }
        break;

    case ACT_REPAIR_OXYGEN:
        if (st->fault_oxygen && st->active_robot == 1 && st->power >= 10) {
            st->power -= 10;
            st->fault_oxygen = 0;
            st->repaired++;
        } else {
            st->failed_action++;
        }
        break;

    case ACT_RESTART_COMMUNICATION:
        /* 不需要机器人；成功不计入 REPAIRED */
        if (st->fault_comm && st->power >= 5) {
            st->power -= 5;
            st->fault_comm = 0;
        } else {
            st->failed_action++;
        }
        break;

    case ACT_SHUTDOWN_LAB:
        if (st->lab_on == 1) {
            st->lab_on = 0;
        } else {
            st->failed_action++;
        }
        break;
    }
}
