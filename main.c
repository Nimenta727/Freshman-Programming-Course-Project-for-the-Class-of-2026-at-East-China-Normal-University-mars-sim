#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "action.h"
#include "event.h"
#include "report.h"
#include "state.h"

#define ERR_SIZE 512

/* 出错时向 stderr 输出原因；若已知输出路径则同时写入错误结果文件 */
static int fail_with_error(const char *output_path, const char *reason, int code)
{
    fprintf(stderr, "ERROR: %s\n", reason);
    if (output_path != NULL) {
        if (!report_write_error(output_path, reason)) {
            fprintf(stderr, "ERROR: 无法写入结果文件 %s\n", output_path);
            return 1;
        }
    }
    return code;
}

int main(int argc, char **argv)
{
    const char *base_path = NULL;
    const char *events_path = NULL;
    const char *actions_path = NULL;
    const char *output_path = NULL;
    char err[ERR_SIZE];

    /* 解析命令行参数：四个参数均为必需，顺序不限 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            continue; /* 容忍孤立的 "--" 分隔符 */
        } else if (strcmp(argv[i], "--base") == 0 && i + 1 < argc) {
            base_path = argv[++i];
        } else if (strcmp(argv[i], "--events") == 0 && i + 1 < argc) {
            events_path = argv[++i];
        } else if (strcmp(argv[i], "--actions") == 0 && i + 1 < argc) {
            actions_path = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else {
            snprintf(err, sizeof(err), "命令行参数错误：%s", argv[i]);
            return fail_with_error(output_path, err, 2);
        }
    }

    if (base_path == NULL || events_path == NULL ||
        actions_path == NULL || output_path == NULL) {
        snprintf(err, sizeof(err),
                 "缺少必需参数，用法：mars_sim --base FILE --events FILE "
                 "--actions FILE --output FILE");
        return fail_with_error(output_path, err, 2);
    }

    /* 依次读取三个输入文件 */
    State st;
    int code = state_load_base(base_path, &st, err, sizeof(err));
    if (code != 0) {
        return fail_with_error(output_path, err, code);
    }

    Event *events = NULL;
    int n = 0;
    code = event_load_all(events_path, &events, &n, err, sizeof(err));
    if (code != 0) {
        return fail_with_error(output_path, err, code);
    }

    Action *actions = NULL;
    int m = 0;
    code = action_load_all(actions_path, &actions, &m, err, sizeof(err));
    if (code != 0) {
        free(events);
        return fail_with_error(output_path, err, code);
    }

    if (m != n) {
        snprintf(err, sizeof(err), "行动数量 %d 与事件数量 %d 不一致", m, n);
        free(events);
        free(actions);
        return fail_with_error(output_path, err, 2);
    }

    /* 按固定顺序推进每个时间片：
     * 1. 事件   2. 行动   3. 结算   4. 截断   5. 时间加一并检查失败   6. 失败则停止 */
    for (int t = 0; t < n; t++) {
        event_apply(&events[t], &st);
        action_execute(&actions[t], &st);
        state_settle(&st);
        state_clamp(&st);
        st.time = t + 1;
        if (state_check_failure(&st)) {
            st.failed = 1;
            break;
        }
    }

    free(events);
    free(actions);

    if (!report_write_result(output_path, &st,
                             st.failed ? "FAILED" : "SURVIVED")) {
        fprintf(stderr, "ERROR: 无法写入结果文件 %s\n", output_path);
        return 1;
    }
    return 0;
}
