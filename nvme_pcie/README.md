# NVMe PCI 驱动框架练习

这里保存 SDK 中 `nvme_pcie/` 的源码和配置快照。

当前实现：三星 PCI ID 匹配表、PCI 驱动注册、probe/remove 日志。
BAR0 申请、MMIO 映射、CAP/VS/CSTS 读取尚待实现。
匹配 ID 当前为 `144d:a802`，使用前应在目标板确认实际设备 ID。

## 构建

需要已经配置并构建的目标内核，以及 ARM64 交叉编译器。
仓库不包含 LubanCat SDK 内核源码和工具链。

本目录位于仓库内部，构建时显式指定 SDK 内核路径：

```bash
make -C nvme_pcie KDIR=/absolute/path/to/LubanCat_SDK/kernel
```

在本次开发环境中使用：

```bash
make -C nvme_pcie KDIR=/home/lubancat/LubanCat_SDK/kernel
```

默认目标编译 `nvme_pcie.ko` 并生成本目录的 `compile_commands.json`。
`.clangd` 使用此数据库解析内核头文件和宏。
数据库、编译产物和缓存不提交到 Git；首次使用或清理后运行 make 重新生成。

```bash
make -C nvme_pcie KDIR=/absolute/path/to/kernel modules
make -C nvme_pcie KDIR=/absolute/path/to/kernel clean
```

当前模块只完成驱动绑定框架。probe 返回 0 表示绑定成功。
原生 nvme 驱动已经占用设备时，仅加载此模块通常不会进入 probe。
ARM64 模块应运行在与构建配置匹配的目标内核上。

## 学习记录

相关讨论见 [Codex 会话存档](../docs/codex/README.md)。
