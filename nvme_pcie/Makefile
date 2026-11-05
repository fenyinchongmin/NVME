# 内核构建系统递归读取本文件时，会设置 KERNELRELEASE。
ifneq ($(KERNELRELEASE),)

# 将 nvme_pcie.c 编译为可加载模块 nvme_pcie.ko。
obj-m += nvme_pcie.o

# make clean 时，一并清理为编辑器生成的编译参数数据库。
clean-files += compile_commands.json

else

# 使用本 Makefile 所在目录定位模块，避免依赖调用 make 时的工作目录。
MODULE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# SDK 已配置并编译过的内核目录。可以通过 make KDIR=其他目录 覆盖。
KDIR ?= $(abspath $(MODULE_DIR)/../kernel)

# 目标是 ARM64 开发板；编译器命令由此前缀与 gcc、ld 等名称组成。
# 例如：aarch64-linux-gnu-gcc。命令行传入的值优先于这些默认值。
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
PYTHON ?= python3

.PHONY: all modules compile_commands clean

# 直接执行 make：先编译模块，再更新 clangd 使用的编译参数数据库。
all: compile_commands

# -C 切换到内核目录；M 指定外部模块目录。
# 内核构建系统负责头文件、配置宏、编译和链接参数，无需手工调用 gcc。
modules:
	$(MAKE) -C "$(KDIR)" M="$(MODULE_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" modules

# 从本模块的 .cmd 文件提取真实编译命令，只收集当前模块。
# -d 必须是内核构建目录，因为命令中的相对头文件路径以该目录为基准。
compile_commands: modules
	$(PYTHON) "$(KDIR)/scripts/clang-tools/gen_compile_commands.py" -d "$(KDIR)" -o "$(MODULE_DIR)/compile_commands.json" "$(MODULE_DIR)"

# 清理当前模块的构建产物，不清理整棵内核树。
clean:
	$(MAKE) -C "$(KDIR)" M="$(MODULE_DIR)" ARCH="$(ARCH)" CROSS_COMPILE="$(CROSS_COMPILE)" clean

endif
