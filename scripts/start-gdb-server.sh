#!/bin/bash

# 确保脚本在项目根目录运行
cd "$(dirname "$0")/.."

# 输出就绪信号（供 VS Code tasks.json 捕获）
echo "QEMU-GDB-SERVER-READY"

# 启动 QEMU（参数与 Makefile 中一致）
exec qemu-system-riscv64 \
    -nographic \
    -machine virt \
    -m 4G \
    -kernel build/QEMU/kernel \
    -S -gdb tcp::15234