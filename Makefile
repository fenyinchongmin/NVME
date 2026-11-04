ifneq ($(KERNELRELEASE),)
# Kbuild：每个 obj-m 目标生成一个 .ko。
obj-m := nvme_pcie_obs.o

# 多个 .c 组成一个模块的示例（替换上面的 obj-m）：
# obj-m := nvme_obs.o
# nvme_obs-y := nvme_pcie_obs.o pcie_regs.o

else
# x86_64 是开发主机；RK3588 的目标架构是 ARM64。
KERNEL_DIR ?= $(abspath ../kernel)
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
PYTHON ?= python3

# 如有其他外部模块目录，在这里追加绝对路径，一并保留其索引。
# EXTRA_MODULE_DIRS := $(abspath ../another_driver)
EXTRA_MODULE_DIRS ?=

export ARCH CROSS_COMPILE

.PHONY: all modules index clean

# 默认先编译成功，再更新 clangd 数据库；make -j 时也保持此顺序。
all: modules
	$(MAKE) index

modules:
	$(MAKE) -C "$(KERNEL_DIR)" M="$(CURDIR)" modules

# 从已有的 Kbuild .cmd 文件提取实际参数，不重新编译整个内核。
index:
	$(PYTHON) "$(KERNEL_DIR)/scripts/clang-tools/gen_compile_commands.py" \
		-d "$(KERNEL_DIR)" -o "$(KERNEL_DIR)/compile_commands.json" \
		"$(KERNEL_DIR)" "$(CURDIR)" $(EXTRA_MODULE_DIRS)

clean:
	$(MAKE) -C "$(KERNEL_DIR)" M="$(CURDIR)" clean
endif
