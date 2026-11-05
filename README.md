# NVME：RK3588 PCIe/NVMe 观察模块

本分支新增 [nvme_pcie 驱动框架练习](nvme_pcie/README.md)，
对应本地 SDK 的 `nvme_pcie/` 源码；构建该模块时请显式指定 `KDIR`。
[Codex 会话存档](docs/codex/README.md) 包含相关 PCIe/NVMe 学习对话。
下面继续说明根目录原有的 `nvme_pcie_obs` 观察模块。

面向鲁班猫 5 BTB（RK3588）的 Linux 内核模块学习项目。
当前模块 `nvme_pcie_obs` 根据 PCI 地址查找设备，在内核日志中输出：

- Vendor ID、Device ID 和设备类别。
- BAR0 的资源范围及大小。
- PCIe 链路速率编码、协商后的链路宽度。
- MSI-X 表支持的向量数量。

目前实现的是 PCIe 设备信息观察功能，尚未实现 NVMe 命令提交、队列管理和块设备读写。
目标地址在 `nvme_pcie_obs.c` 中固定为 `0000:01:00.0`；请先在目标板上用
`lspci -D` 确认实际地址，再按需调整 `pci_get_domain_bus_and_slot()` 的参数。

## 获取项目与编译环境

在 x86_64 主机上开发，使用 `aarch64-linux-gnu-gcc` 为 RK3588 编译。
`ARCH=arm64` 和 clangd 的 `--target=aarch64-linux-gnu` 都描述目标板。
`KERNEL_DIR` 应指向与板上运行内核匹配、已配置并构建的 SDK 内核目录。
当前默认使用 `../kernel`。

需要 Git、GNU Make、Python 3 和对应的 ARM64 交叉编译工具链。
本仓库只包含模块项目，内核源码和工具链由 LubanCat SDK 提供。

在 SDK 根目录克隆为 `pcie_nvme`（仅在该目录尚不存在时执行）：

```bash
git clone https://github.com/fenyinchongmin/NVME.git pcie_nvme
```

预期目录结构：

```text
LubanCat_SDK/
├── kernel/
└── pcie_nvme/
    ├── Makefile
    ├── README.md
    └── nvme_pcie_obs.c
```

如果项目放在其他位置，可在项目目录指定内核路径：

```bash
make KERNEL_DIR=/absolute/path/to/LubanCat_SDK/kernel
```

## 在目标板运行

编译完成后，将 `nvme_pcie_obs.ko` 复制到运行匹配内核的 RK3588 板上。
在板上模块所在目录执行：

```bash
sudo insmod nvme_pcie_obs.ko
sudo dmesg | tail -n 30
sudo rmmod nvme_pcie_obs
```

设备不存在时，模块初始化返回 `-ENODEV`，日志显示 `NVMe device not found`。

## 日常使用

在 SDK 根目录运行：

```bash
make -C pcie_nvme
```

默认先编译模块，成功后自动刷新 `kernel/compile_commands.json`。
索引刷新只扫描内核和模块的构建记录，不重新编译整个内核。
按下面的方法配置 SDK 根目录的 `.clangd` 后，它会覆盖 `pcie_nvme/` 的所有子目录。
在其中添加文件不需要逐文件修改编辑器配置。

其他命令：

```bash
make -C pcie_nvme modules  # 只编译模块
make -C pcie_nvme index    # 只从已有构建记录刷新数据库
make -C pcie_nvme clean    # 清理本模块的构建产物
```

只修改已有源码时，无需修改编辑器配置。新增 `.c` 必须参与一次成功的编译，
才能得到它自己的准确编译记录；仅创建文件或仅运行 `index` 不会产生这条记录。
不需要手工编辑 `compile_commands.json`，它是生成文件。

## VS Code / clangd 配置

使用 clangd 扩展时，在 SDK 根目录创建 `.clangd`，与 `kernel/`、`pcie_nvme/` 同级：

```yaml
If:
  PathMatch: [kernel/.*, pcie_nvme/.*]
CompileFlags:
  CompilationDatabase: kernel
  Compiler: clang
  Add: [--target=aarch64-linux-gnu]
  Remove: [aarch64-linux-gnu-gcc, -mabi=lp64, -fconserve-stack, --param=*]
```

这让内核源码和模块共用编译数据库，并按 ARM64 目标解析。
`Remove` 处理当前 SDK 的 GCC 包装命令和 GCC 特有参数；实际编译仍由 Makefile 调用 GCC。
若模块目录换了名字，需要相应修改 `PathMatch`。

在 VS Code 中打开整个 SDK 目录，启用 clangd 的后台索引。
第一次编译并添加上述配置后，执行 `clangd: Restart language server`，等待索引完成再按 F12。
在 SDK 外开发时，可将此配置放到同时包含模块与内核的共同父目录，并调整匹配范围和数据库路径。

## 新增文件时改什么

下面的 Kbuild 设置均放在 Makefile 顶部的 `ifneq ($(KERNELRELEASE),)` 分支中。

### 新增一个独立模块

例如新增 `pcie_test.c`，希望生成独立的 `pcie_test.ko`，保留现有目标并追加：

```makefile
obj-m := nvme_pcie_obs.o
obj-m += pcie_test.o
```

然后运行 `make -C pcie_nvme`。

### 新增一个属于同一模块的源文件

例如 `nvme_pcie_obs.c` 和 `pcie_regs.c` 共同生成 `nvme_obs.ko`，
将原来的 `obj-m` 替换为：

```makefile
obj-m := nvme_obs.o
nvme_obs-y := nvme_pcie_obs.o pcie_regs.o
```

以后再添加 `nvme_queue.c`，只需在 `nvme_obs-y` 后面加 `nvme_queue.o`。
子目录里的文件同样可以写为 `hw/pcie_regs.o`。
最终模块名应与各个组成对象区分开，避免把 `nvme_obs.o` 列入自己的组成列表。
同一个模块只保留一组 `module_init` / `module_exit`。

### 新增头文件

头文件不加入 `obj-m` 或 `nvme_obs-y`，在使用它的源文件中包含：

```c
#include "pcie_regs.h"
```

如果统一放到本模块的 `include/` 目录，在 Kbuild 分支中添加：

```makefile
ccflags-y += -I$(src)/include
```

然后重新执行默认构建，让索引得到更新后的头文件搜索路径。

## 在 pcie_nvme 之外新增模块目录

例如 SDK 根目录下新增 `another_driver/`，需要一次性配置：

1. 给新目录准备自己的 Kbuild/Makefile，并成功编译该模块。
2. 在 SDK 根目录 `.clangd` 的 `PathMatch` 中加入 `another_driver/.*`。
3. 在 `pcie_nvme/Makefile` 的外层构建分支中设置
   `EXTRA_MODULE_DIRS := $(abspath ../another_driver)`，然后运行 `make -C pcie_nvme index`。

这样，本 Makefile 每次刷新数据库时都会包含两个模块目录。
新目录编译后也要刷新这个共享数据库；多个目录共用数据库时，应由同一处统一生成，
避免各自生成只包含自身记录的数据库而互相覆盖。

## 跳转仍未更新时

先确认终端编译成功，再等待 clangd 后台索引。
若仍使用旧结果，在 VS Code 命令面板执行 `clangd: Restart language server`。
跳到 `pci.h` 中的声明但尚未跳到 `.c` 实现时，可先打开实现文件帮助建立索引。

参考：[内核外部模块构建说明](https://docs.kernel.org/kbuild/modules.html)、
[clangd 编译数据库说明](https://clangd.llvm.org/design/compile-commands)。
