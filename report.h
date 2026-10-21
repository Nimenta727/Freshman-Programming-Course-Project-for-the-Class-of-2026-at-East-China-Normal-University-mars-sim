#ifndef REPORT_H
#define REPORT_H

#include <stdbool.h>

#include "state.h"

/* 正常结束时写入固定 12 行的结果文件；result 只能是 "SURVIVED" 或 "FAILED"。
 * 成功返回 true；无法写入返回 false。 */
bool report_write_result(const char *path, const State *st, const char *result);

/* 异常无法运行时写入两行错误结果文件（RESULT ERROR / REASON ...）。
 * 成功返回 true；无法写入返回 false。 */
bool report_write_error(const char *path, const char *reason);

#endif
