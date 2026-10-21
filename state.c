#include "state.h"

#include <stdio.h>
#include <string.h>

#include "parse.h"

/* base.txt 中的字段描述：名称、合法范围、写入位置、是否已出现 */
typedef struct {
    const char *name;
    int min;
    int max;
    int *field;
    bool seen;
} FieldSpec;

Resource resource_from_name(const char *name)
{
    if (strcmp(name, "POWER") == 0) return RES_POWER;
    if (strcmp(name, "OXYGEN") == 0) return RES_OXYGEN;
    if (strcmp(name, "WATER") == 0) return RES_WATER;
    if (strcmp(name, "TEMPERATURE") == 0) return RES_TEMPERATURE;
    if (strcmp(name, "HULL") == 0) return RES_HULL;
    return RES_INVALID;
}

int state_load_base(const char *path, State *st, char *err, size_t err_size)
{
    /* 运行期状态的固定初始值 */
    st->active_robot = 0;
    st->lab_on = 1;
    st->fault_solar = 0;
    st->fault_oxygen = 0;
    st->fault_comm = 0;
    st->repaired = 0;
    st->failed_action = 0;
    st->time = 0;
    st->failed = 0;

    FieldSpec specs[] = {
        {"POWER",       0, 1000, &st->power,       false},
        {"OXYGEN",      0, 1000, &st->oxygen,      false},
        {"WATER",       0, 1000, &st->water,       false},
        {"TEMPERATURE", -100, 100, &st->temperature, false},
        {"HULL",        0, 100,  &st->hull,        false},
        {"ROBOT",       0, 10,   &st->robot,       false},
    };
    const int field_count = (int)(sizeof(specs) / sizeof(specs[0]));

    LineReader rd;
    if (!reader_open(&rd, path)) {
        snprintf(err, err_size, "无法打开文件 %s", path);
        return 2;
    }

    int records = 0;
    int r;
    while ((r = reader_next_record(&rd)) == 1) {
        char *tokens[PARSE_MAX_TOKENS];
        int nt = reader_tokenize(&rd, tokens, PARSE_MAX_TOKENS);
        if (nt != 2) {
            snprintf(err, err_size, "%s 第 %d 行：格式错误，应为“字段名 数值”",
                     path, rd.lineno);
            reader_close(&rd);
            return 2;
        }

        int idx = -1;
        for (int i = 0; i < field_count; i++) {
            if (strcmp(tokens[0], specs[i].name) == 0) {
                idx = i;
                break;
            }
        }
        if (idx < 0) {
            snprintf(err, err_size, "%s 第 %d 行：未知字段 %s",
                     path, rd.lineno, tokens[0]);
            reader_close(&rd);
            return 2;
        }
        if (specs[idx].seen) {
            snprintf(err, err_size, "%s 第 %d 行：字段重复 %s",
                     path, rd.lineno, tokens[0]);
            reader_close(&rd);
            return 2;
        }

        int value;
        if (!parse_int_strict(tokens[1], &value)) {
            snprintf(err, err_size, "%s 第 %d 行：不是整数 %s %s",
                     path, rd.lineno, tokens[0], tokens[1]);
            reader_close(&rd);
            return 2;
        }
        if (value < specs[idx].min || value > specs[idx].max) {
            snprintf(err, err_size, "%s 第 %d 行：数值范围错误 %s %s",
                     path, rd.lineno, tokens[0], tokens[1]);
            reader_close(&rd);
            return 2;
        }

        *specs[idx].field = value;
        specs[idx].seen = true;
        records++;
    }

    reader_close(&rd);

    if (r == -1) {
        snprintf(err, err_size, "%s：行过长或读取错误", path);
        return 2;
    }
    if (records < field_count) {
        for (int i = 0; i < field_count; i++) {
            if (!specs[i].seen) {
                snprintf(err, err_size, "%s：字段缺失 %s", path, specs[i].name);
                return 2;
            }
        }
    }
    if (records > field_count) {
        /* 多余记录只可能是重复或未知字段，正常情况下已在循环中拦截 */
        snprintf(err, err_size, "%s：记录数量错误", path);
        return 2;
    }
    return 0;
}

void state_apply_delta(State *st, Resource res, int delta)
{
    switch (res) {
    case RES_POWER:       st->power += delta;       break;
    case RES_OXYGEN:      st->oxygen += delta;      break;
    case RES_WATER:       st->water += delta;       break;
    case RES_TEMPERATURE: st->temperature += delta; break;
    case RES_HULL:        st->hull += delta;        break;
    default:                                        break;
    }
}

void state_settle(State *st)
{
    if (st->lab_on) {
        st->oxygen -= 2;
        st->water -= 1;
    } else {
        st->oxygen -= 1;
    }
}

void state_clamp(State *st)
{
    if (st->power < 0) st->power = 0;
    if (st->oxygen < 0) st->oxygen = 0;
    if (st->water < 0) st->water = 0;
    if (st->hull < 0) st->hull = 0;
    if (st->hull > 100) st->hull = 100;
    /* TEMPERATURE 不做截断 */
}

bool state_check_failure(const State *st)
{
    return st->oxygen == 0 || st->hull == 0 || st->temperature < 0;
}

int state_score(const State *st)
{
    int score = st->time
              + 10 * st->repaired
              - 5 * st->failed_action
              + st->power / 10
              + st->oxygen / 10
              + st->water / 10
              + st->hull / 10;
    if (score < 0) {
        score = 0;
    }
    return score;
}
