# 静态程序分析 · 作业 2：上下文敏感控制流分析

- 姓名：马蕴哲
- 学号：3220262773

## 任务与目录

按照 Lec03 第32–41页，在 ICFG 上用 DFS 枚举从 `main` 入口到 `main` 出口的路径，使用调用栈匹配 call 和 return，并调用框架提供的 `recordPath` 保存结果。

图片及课件中的 `Assignment-3-CFA/CFGA.cpp`、`Assignment-3-IR/Test-Cases` 与本仓库命名不一致。本仓库实际位置为：

- `Assignment-2-CFA/CFGA.cpp`：主程序、analyze 和 DFS 实现。
- `Assignment-2-CFA/CFGA.h`：分析器声明及状态类型。
- `Assignment-2-CFA/cfga_lib.cpp`：原有入口/出口选择、路径保存与输出逻辑，保持原样。
- `Assignment-2-CFA/Test-Cases/`：5个原始测试，保持原样。
- `run-tests.sh`：虚拟机批量测试脚本。
- [作业2说明.md](作业2说明.md)：原理、操作步骤和判定标准。

## 实现要点

1. 普通过程内边：直接继续 DFS，调用栈不变。
2. 调用边：将调用点的节点 ID 入栈，递归，回溯时出栈。
3. 返回边：只有其目标返回节点对应的调用点 ID 与栈顶一致时才允许通过；递归前出栈，回溯后恢复。
4. 访问状态是 `(节点 ID, 调用栈)`，且 visited 仅针对当前路径。回溯时删除状态，避免遗漏其它分支；不同调用点进入同一个函数不会互相干扰。
5. 到达 main 出口且调用栈为空时记录路径。reachablePaths 使用集合去重。
6. 输出 ICFG DOT，便于对照结果文件中的节点 ID。

这是结构上的上下文敏感路径枚举，不求解分支条件，不判断断言、污点传播或内存安全。按照课件的 visited 剪枝方式，当前路径不重复访问相同 `(节点, 调用栈)` 状态，不枚举任意次循环。提供的测试没有递归；无界递归会产生无限调用上下文，不在此实现的支持范围内。

## 在虚拟机编译

使用现有课程 SVF/LLVM 16/Z3 环境，在仓库根目录执行：

```bash
# 如 LLVM_DIR 未设置，而课程 LLVM 位于 SVF_DIR 下：
export LLVM_DIR="${LLVM_DIR:-$SVF_DIR/llvm-16.0.0.obj}"
export PATH="$LLVM_DIR/bin:$PATH"
clang --version
opt --version
bash build.sh Assignment-2-CFA
ls -lh Assignment-2-CFA/cfga
```

使用 `bash` 而不是 `source`。构建脚本会清理并重建 `build/`，任何构建步骤失败即停止。

## 单个测试

```bash
mkdir -p Assignment-2-CFA/results/simple-call
clang -S -emit-llvm -O0 -fno-discard-value-names \
  -Xclang -disable-O0-optnone \
  Assignment-2-CFA/Test-Cases/simple-call.c \
  -o Assignment-2-CFA/results/simple-call/simple-call.ll
opt -S -passes=mem2reg \
  Assignment-2-CFA/results/simple-call/simple-call.ll \
  -o Assignment-2-CFA/results/simple-call/simple-call.opt.ll
./Assignment-2-CFA/cfga \
  Assignment-2-CFA/results/simple-call/simple-call.opt.ll
cat Assignment-2-CFA/results/simple-call/simple-call.opt.ll.res.txt
```

`.res.txt` 每行一条路径，由逗号分隔的 ICFG 节点 ID 组成。具体编号以虚拟机生成的 ICFG 为准。

## 全部测试

```bash
bash run-tests.sh
```

脚本需要 Python 3；若存在 Graphviz，会同时生成 SVG，若没有 Graphviz，仍可检查路径结果。可通过 `CFGA_CLANG=clang-16 CFGA_OPT=opt-16 bash run-tests.sh` 指定工具。确认版本输出均为16。

| 用例 | 默认课程模型下预期结构路径数 | 重点 |
| --- | ---: | --- |
| simple-call | 1 | 一次调用和返回 |
| multi-call | 1 | 两次 inc 调用都出现，不能错配返回或遗漏第二次调用 |
| branch | 2 | if/else 两条路径，bar 返回到各自调用点 |
| loop | 1 | 状态重复时剪枝，不展开100次循环 |
| buffer | 4 | 两次调用各有两种分支组合，且调用上下文独立 |

这里统计的是图上的结构路径，不是实际可行执行：buffer 的常量参数实际上限制 initialize 分支，但本分析不求解这些条件。路径数量以原测试、mem2reg 和课程外部函数模型为前提；若 SVF 版本改变 malloc/free 的建模，可导致数量变化，应检查 DOT，而不要直接修改代码凑数量。

`PASS` 只代表运行成功、输出格式及数量符合检查，不代表自动评分通过。还需对照 DOT 人工核对路径边和 call/return 配对。

## 图的查看

```bash
dot -Tsvg \
  Assignment-2-CFA/results/multi-call/multi-call.opt.ll.icfg.dot \
  -o Assignment-2-CFA/results/multi-call/icfg.svg
xdg-open Assignment-2-CFA/results/multi-call/icfg.svg
```

若未安装 Graphviz，Ubuntu 可执行 `sudo apt install graphviz`。Graphviz 若报告端口警告，需留意连线显示；它不改变 `.res.txt` 中记录的路径。

## 验证状态与提交

本机已运行隔离的 C++ 算法测试，使用实际 analyze/dfs 方法和最小图接口，覆盖分支汇合、循环剪枝、重复调用、错误返回、嵌套调用、外部调用及重复 analyze。该测试不依赖真实 SVF，不能替代课程环境编译和5个 C 测试的端到端验证。后者需在虚拟机执行，本机尚未验证。

虚拟机确认通过后，按图片要求向延河课堂提交本作业仓库和测试集。测试源文件已保留；results 目录默认忽略，如需将验证产物一并提交，可明确选择：

```bash
git add -f Assignment-2-CFA/results/
```

不要提交 `build/` 或 Linux 可执行文件。提交前在虚拟机确认全部结果，再更新本节的实际验证情况。
