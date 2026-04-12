#!/bin/bash

# ARM 交叉工具链：仓库内为压缩包，首次使用前解压到 tools/15.2.rel1-arm64
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="$SCRIPT_DIR/../tools"
TOOLCHAIN_DIR="$TOOLS_DIR/15.2.rel1-arm64"
ARCHIVE="$TOOLS_DIR/arm-gnu-toolchain-15.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz"
GCC_LOCAL="$TOOLCHAIN_DIR/bin/aarch64-none-linux-gnu-gcc"

if [[ -x "$GCC_LOCAL" ]]; then
    if [[ ":$PATH:" != *":$TOOLCHAIN_DIR/bin:"* ]]; then
        echo "检测到已解压的本地工具链，正在加入 PATH..."
        export PATH="$TOOLCHAIN_DIR/bin:$PATH"
    fi
elif command -v aarch64-none-linux-gnu-gcc >/dev/null 2>&1; then
    echo "已检测到 PATH 中的 aarch64-none-linux-gnu-gcc，跳过解压。"
else
    if [[ ! -f "$ARCHIVE" ]]; then
        echo "错误：未找到工具链压缩包，请将以下文件放入仓库后再编译：" >&2
        echo "  $ARCHIVE" >&2
        exit 1
    fi
    echo "正在解压 ARM GNU 工具链到 $TOOLCHAIN_DIR ..."
    mkdir -p "$TOOLCHAIN_DIR"
    if ! tar -xJf "$ARCHIVE" -C "$TOOLCHAIN_DIR" --strip-components=1; then
        echo "错误：解压失败，请确认系统 tar 支持 xz（-J），且压缩包完整。" >&2
        exit 1
    fi
    if [[ ! -x "$GCC_LOCAL" ]]; then
        echo "错误：解压后未找到预期编译器: $GCC_LOCAL" >&2
        echo "若压缩包顶层目录结构与官方包不一致，请检查包内路径并调整 --strip-components。" >&2
        exit 1
    fi
    export PATH="$TOOLCHAIN_DIR/bin:$PATH"
fi

export GCC_COLORS=auto

echo "========================================="
echo "GCC 交叉编译环境"
echo "========================================="
if [[ -x "$GCC_LOCAL" ]]; then
    echo "工具链路径: $TOOLCHAIN_DIR/bin"
elif command -v arm-none-linux-gnueabihf-gcc >/dev/null 2>&1; then
    echo "使用 PATH 中的: $(command -v aarch64-none-linux-gnu-gcc)"
fi
echo "GCC_COLORS=auto"
echo ""
echo "测试 GCC 版本："
aarch64-none-linux-gnu-gcc -v 2>&1 | head -n 5
echo ""
echo "可以开始编译 u-boot"
echo "========================================="
echo ""

#make clean

# make quark-luoorshi-h5_defconfig ARCH=arm64

make quark-luoorshi-h5_defconfig ARCH=arm64

# 合并内核升级后新增的 Kconfig 项，全部用默认值填充，避免 make 时进入交互问答
make olddefconfig ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-

# make menuconfig ARCH=arm64

make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- Image dtbs modules 2>&1 | tee build.log

