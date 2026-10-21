#include "report.h"

#include <stdio.h>

bool report_write_result(const char *path, const State *st, const char *result)
{
    /* 以二进制方式写入，保证任何平台下行尾都是单个 \n */
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        return false;
    }

    fprintf(fp, "RESULT %s\n", result);
    fprintf(fp, "TIME %d\n", st->time);
    fprintf(fp, "POWER %d\n", st->power);
    fprintf(fp, "OXYGEN %d\n", st->oxygen);
    fprintf(fp, "WATER %d\n", st->water);
    fprintf(fp, "TEMPERATURE %d\n", st->temperature);
    fprintf(fp, "HULL %d\n", st->hull);
    fprintf(fp, "ROBOT %d\n", st->robot);
    fprintf(fp, "ACTIVE_ROBOT %d\n", st->active_robot);
    fprintf(fp, "REPAIRED %d\n", st->repaired);
    fprintf(fp, "FAILED_ACTION %d\n", st->failed_action);
    fprintf(fp, "SCORE %d\n", state_score(st));

    if (fclose(fp) != 0) {
        return false;
    }
    return true;
}

bool report_write_error(const char *path, const char *reason)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        return false;
    }

    fprintf(fp, "RESULT ERROR\n");
    fprintf(fp, "REASON %s\n", reason);

    if (fclose(fp) != 0) {
        return false;
    }
    return true;
}
