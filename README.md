# mars_sim —— 火星基地失控 72 小时

一个可自动运行、自动比较结果的火星基地仿真程序。程序读取三个输入文件
（基地初始状态、事件序列、行动序列），按规定的时间片顺序推进仿真，
最后把固定格式的结果写入结果文件。

## 构建

在项目根目录执行：

```sh
make clean
make
```

等价的手工编译命令（教师删除可执行文件和中间文件后可用它重新编译）：

```sh
gcc -std=c11 -Wall -Wextra -pedantic -O2 -o mars_sim *.c
```

只使用 C11 标准库，无第三方依赖。

## 运行

```sh
./mars_sim --base base.txt --events events.txt --actions actions.txt --output result.txt
```

四个参数均为必需，顺序不限：

| 参数 | 含义 |
| --- | --- |
| `--base FILE` | 读取基地初始状态 |
| `--events FILE` | 读取事件序列 |
| `--actions FILE` | 读取行动序列 |
| `--output FILE` | 写入固定格式的结果文件 |

正常仿真时，结果只写入 `--output` 指定的文件；终端只输出错误信息。

## 退出状态

- `0`：仿真正常完成（无论结果是 SURVIVED 还是 FAILED）
- `2`：输入错误（命令行参数错误、文件无法打开、格式/范围非法、
  行动数量与事件数量不一致等）
- `1`：不可恢复的运行错误（结果文件无法写入、内存分配失败）

输入错误时仍会写出 `--output` 文件，但内容固定为两行，不会被误认为正确结果：

```
RESULT ERROR
REASON 具体错误原因
```

## 目录结构

```
mars_sim/
├── main.c        命令行解析与仿真主循环
├── state.c/h     基地状态、base.txt 读取、结算/截断/失败判定/得分
├── event.c/h     events.txt 读取与事件应用（含故障激活）
├── action.c/h    actions.txt 读取与行动执行（含失败计数）
├── report.c/h    12 行结果文件与错误结果文件的写入
├── parse.c/h     逐行读取、token 切分、严格整数解析
├── Makefile
├── README.md
├── 项目报告.md
└── tests/        公开测试用例与运行脚本
```

## 测试

`tests/` 下每个 case 目录包含 `base.txt` / `events.txt` / `actions.txt`，
正常用例附带 `expected_result.txt`。构建完成后运行：

```sh
cd tests
sh run_tests.sh
```

覆盖场景：题目完整示例、氧气耗尽失败、温度失败、舱体归零失败、
关闭实验舱（含重复关舱失败）、重复激活机器人失败、
初始值越界、行动数量与事件数量不一致。

## 关键规则实现要点

- 每个时间片严格按「事件 → 行动 → 结算 → 截断 → 计时并判失败 → 记录」执行。
- 事件阶段不截断，行动条件判断看到的是事件扣减后的实时值。
- 截断阶段：POWER/OXYGEN/WATER 不小于 0，HULL 限制在 0-100，TEMPERATURE 不截断。
- 失败条件：OXYGEN=0 或 HULL=0 或 TEMPERATURE<0，TIME 为失败发生的时间片编号。
- RESTART COMMUNICATION 不需要机器人，成功也不计入 REPAIRED。
- SCORE = TIME + 10×REPAIRED − 5×FAILED_ACTION + POWER/10 + OXYGEN/10
  + WATER/10 + HULL/10，C 整数除法，小于 0 时输出 0。
