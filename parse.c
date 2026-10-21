#include "parse.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

bool reader_open(LineReader *rd, const char *path)
{
    /* 以二进制方式打开，避免 Windows 下的 CRLF 转换，\r 由读取逻辑自行处理 */
    rd->fp = fopen(path, "rb");
    if (rd->fp == NULL) {
        return false;
    }
    rd->path = path;
    rd->lineno = 0;
    return true;
}

void reader_close(LineReader *rd)
{
    if (rd->fp != NULL) {
        fclose(rd->fp);
        rd->fp = NULL;
    }
}

/* 判断一行是否只含空白字符 */
static bool line_is_blank(const char *s)
{
    while (*s != '\0') {
        if (*s != ' ' && *s != '\t') {
            return false;
        }
        s++;
    }
    return true;
}

int reader_next_record(LineReader *rd)
{
    while (fgets(rd->buf, (int)sizeof(rd->buf), rd->fp) != NULL) {
        rd->lineno++;

        size_t len = strlen(rd->buf);
        /* 行过长：未读到换行符且未到文件末尾 */
        if (len > 0 && rd->buf[len - 1] != '\n' && !feof(rd->fp)) {
            return -1;
        }

        /* 去除 UTF-8 BOM（只可能出现在第一行开头） */
        if (rd->lineno == 1 && len >= 3 &&
            (unsigned char)rd->buf[0] == 0xEF &&
            (unsigned char)rd->buf[1] == 0xBB &&
            (unsigned char)rd->buf[2] == 0xBF) {
            memmove(rd->buf, rd->buf + 3, len - 2);
            len -= 3;
        }

        /* 去除行尾 \n 和 \r */
        while (len > 0 && (rd->buf[len - 1] == '\n' || rd->buf[len - 1] == '\r')) {
            rd->buf[--len] = '\0';
        }

        /* 空行不计入记录 */
        if (line_is_blank(rd->buf)) {
            continue;
        }
        return 1;
    }
    return 0;
}

int reader_tokenize(LineReader *rd, char **tokens, int max_tokens)
{
    int n = 0;
    char *p = rd->buf;

    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (n >= max_tokens) {
            return -1;
        }
        tokens[n++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p != '\0') {
            *p++ = '\0';
        }
    }
    return n;
}

bool parse_int_strict(const char *s, int *out)
{
    if (s == NULL || *s == '\0') {
        return false;
    }

    const char *p = s;
    if (*p == '+' || *p == '-') {
        p++;
    }
    if (!isdigit((unsigned char)*p)) {
        return false;
    }
    for (; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p)) {
            return false;
        }
    }

    errno = 0;
    long v = strtol(s, NULL, 10);
    if (errno == ERANGE || v > INT_MAX || v < INT_MIN) {
        return false;
    }
    *out = (int)v;
    return true;
}
