#!/bin/sh
# mars_sim 公开测试脚本
# 用法：在项目根目录 make 之后，进入 tests 目录执行  sh run_tests.sh
# 每个 case 目录包含 base.txt / events.txt / actions.txt，
# 正常用例附带 expected_result.txt；错误用例检查退出码与 RESULT ERROR。

SIM=../mars_sim
[ -x "$SIM" ] || SIM=../mars_sim.exe

pass=0
fail=0

check_normal() {
    dir="$1"
    "$SIM" --base "$dir/base.txt" --events "$dir/events.txt" \
           --actions "$dir/actions.txt" --output "$dir/result.txt"
    code=$?
    if [ $code -ne 0 ]; then
        echo "FAIL $dir: exit code $code, expected 0"
        fail=$((fail + 1))
        return
    fi
    if diff -q "$dir/expected_result.txt" "$dir/result.txt" > /dev/null 2>&1; then
        echo "PASS $dir"
        pass=$((pass + 1))
    else
        echo "FAIL $dir: result.txt 与期望不一致"
        diff "$dir/expected_result.txt" "$dir/result.txt"
        fail=$((fail + 1))
    fi
}

check_error() {
    dir="$1"
    "$SIM" --base "$dir/base.txt" --events "$dir/events.txt" \
           --actions "$dir/actions.txt" --output "$dir/result.txt" 2>/dev/null
    code=$?
    if [ $code -eq 0 ]; then
        echo "FAIL $dir: exit code 0, expected non-zero"
        fail=$((fail + 1))
        return
    fi
    line1=$(head -n 1 "$dir/result.txt" | tr -d '\r')
    if [ "$line1" = "RESULT ERROR" ]; then
        echo "PASS $dir (exit $code)"
        pass=$((pass + 1))
    else
        echo "FAIL $dir: 结果文件首行不是 RESULT ERROR"
        fail=$((fail + 1))
    fi
}

check_normal case01_example
check_normal case02_oxygen_failure
check_normal case03_shutdown_lab
check_normal case04_activate_twice
check_error  case05_error_range
check_error  case06_m_ne_n
check_normal case07_temp_failure
check_normal case08_hull_zero

echo "-----------------------------"
echo "PASS: $pass  FAIL: $fail"
[ $fail -eq 0 ]
