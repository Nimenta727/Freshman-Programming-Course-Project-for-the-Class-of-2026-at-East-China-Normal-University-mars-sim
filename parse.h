#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include <stdio.h>

#define PARSE_MAX_LINE 4096
#define PARSE_MAX_TOKENS 16

/* 逐行读取器：以二进制方式打开 UTF-8 文本，逐条返回非空记录行 */
typedef struct {
    FILE *fp;
    const char *path;
    int lineno;                  /* 当前物理行号 */
    char buf[PARSE_MAX_LINE];
} LineReader;

/* 打开文件。成功返回 true；无法打开返回 false。 */
bool reader_open(LineReader *rd, const char *path);

/* 关闭文件 */
void reader_close(LineReader *rd);

/* 读取下一条非空记录（空行不计入记录，自动跳过）。
 * 自动处理 UTF-8 BOM 与行尾 \r\n。
 * 返回 1 成功；0 文件结束；-1 行过长或读取错误。 */
int reader_next_record(LineReader *rd);

/* 把当前记录行按空白字符（空格/制表符）切分为 token。
 * token 指针直接指向内部缓冲区。返回 token 数；token 过多返回 -1。 */
int reader_tokenize(LineReader *rd, char **tokens, int max_tokens);

/* 严格解析十进制整数：允许可选符号，其余必须全是数字，且在 int 范围内。 */
bool parse_int_strict(const char *s, int *out);

#endif
