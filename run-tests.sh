#!/usr/bin/env bash
# Run from any directory: bash /path/to/Assignment-2/run-tests.sh
set -euo pipefail
cd "$(dirname "$0")"

llvm_root="${LLVM_DIR:-}"
if [[ -z "$llvm_root" && -n "${SVF_DIR:-}" ]]; then
    llvm_root="$SVF_DIR/llvm-16.0.0.obj"
fi
clang_bin="${CFGA_CLANG:-${llvm_root:+$llvm_root/bin/}clang}"
opt_bin="${CFGA_OPT:-${llvm_root:+$llvm_root/bin/}opt}"
"$clang_bin" --version
"$opt_bin" --version
command -v python3 >/dev/null
if [[ ! -x Assignment-2-CFA/cfga ]]; then
    echo '未找到 cfga，请先运行 bash build.sh Assignment-2-CFA' >&2
    exit 1
fi

for src in Assignment-2-CFA/Test-Cases/*.c; do
    name=$(basename "$src" .c)
    out="Assignment-2-CFA/results/$name"
    mkdir -p "$out"
    "$clang_bin" -S -emit-llvm -O0 -fno-discard-value-names \
        -Xclang -disable-O0-optnone "$src" -o "$out/$name.ll"
    "$opt_bin" -S -passes=mem2reg "$out/$name.ll" -o "$out/$name.opt.ll"
    # Remove old outputs so a failed run cannot appear to pass.
    rm -f "$out/$name.opt.ll.res.txt" "$out/$name.opt.ll.icfg.dot"
    if command -v timeout >/dev/null; then
        timeout 30s ./Assignment-2-CFA/cfga "$out/$name.opt.ll" > "$out/run.log" 2>&1
    else
        ./Assignment-2-CFA/cfga "$out/$name.opt.ll" > "$out/run.log" 2>&1
    fi
    test -s "$out/$name.opt.ll.icfg.dot"
    python3 - "$name" "$out/$name.opt.ll.res.txt" <<'PY'
import re
import sys
from pathlib import Path
name, filename = sys.argv[1:]
text = Path(filename).read_text()
lines = [line.strip() for line in text.splitlines() if line.strip()]
assert lines, f'{name}: 未找到入口到出口的路径'
assert all(re.fullmatch(r'(\d+,\s*)+', line) for line in lines), '路径格式不正确'
assert len(set(lines)) == len(lines), '存在重复路径'
# Expected context-simple structural paths for the supplied tests, not executions.
expected = {'simple-call': 1, 'multi-call': 1, 'branch': 2, 'loop': 1, 'buffer': 4}
assert len(lines) == expected[name], (
    f'{name}: 预期 {expected[name]} 条结构路径，实际 {len(lines)} 条；'
    '请检查 ICFG 和课程 SVF 外部函数模型是否不同'
)
print(f'PASS: {name}, {len(lines)} paths (count/format checks)')
PY
    if command -v dot >/dev/null; then
        dot -Tsvg "$out/$name.opt.ll.icfg.dot" -o "$out/icfg.svg"
    fi
done
printf '\n全部 5 个用例通过路径数量和格式检查。请对照 ICFG 继续检查调用/返回匹配。\n'
