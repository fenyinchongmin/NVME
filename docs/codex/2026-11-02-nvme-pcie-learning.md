# NVMe PCI 驱动框架学习

会话 ID：`01a19b76-4c8d-7b00-8b42-ffb5dc7e6bf4`

时间显示为北京时间（UTC+08:00）。正文保留本地对话中的历史路径和示例。

## 2026-11-02 16:53:21 · 用户

### Stage 1：PCIe 设备驱动框架

第一步

文件：`nvme_pci.c`

实现：

1. 定义 `struct pci_device_id`。
2. 定义 `struct pci_driver`。
3. 实现 `probe()` 和 `remove()`。
4. 使自己的驱动能够匹配三星 NVMe SSD。
5. 申请 BAR0 并映射 MMIO。
6. 读取 NVMe 的 CAP、VS、CSTS 寄存器。

这是我后面将要写的东西，现在我想根据这个源码先进行学习，帮我找找对应的位置

## 2026-11-02 16:53:25 · 助手进度

我先定位这份源码里的 NVMe PCI 驱动，再按你列的 6 个目标标出对应函数和阅读顺序。

## 2026-11-02 16:54:22 · 助手进度

这份 SDK 的对应文件是 `kernel/drivers/nvme/host/pci.c`。你列的 6 项都有参考实现；有个阅读时容易漏掉的点：BAR 映射在 `probe()` 中完成，而 PCI 使能和部分寄存器读取放在异步执行的 `nvme_reset_work()` 中。

## 2026-11-02 16:55:45 · 助手

你准备写的 `nvme_pci.c`，在这份 SDK 中对应的参考源码是 **[kernel/drivers/nvme/host/pci.c](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c)**。按你列的 6 个目标，可以这样找：

| 你的目标 | 对应位置 | 阅读重点 |
|---|---|---|
| 1. 定义 `pci_device_id` 匹配表 | [pci.c:3198](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:3198)，`nvme_id_table[]` | `PCI_DEVICE()`、`PCI_DEVICE_CLASS()`、末尾的 `{ 0, }`，以及 `MODULE_DEVICE_TABLE()` |
| 2. 定义 `pci_driver` | [pci.c:3287](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:3287)，`nvme_driver` | `.name`、`.id_table`、`.probe`、`.remove` 如何连接起来 |
| 3. 实现 `probe()`、`remove()` | [pci.c:2879](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2879)，`nvme_probe()`；[pci.c:3004](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:3004)，`nvme_remove()` | 设备私有数据、资源申请、失败回退、卸载时释放资源 |
| 4. 匹配三星 NVMe SSD | [pci.c:3236](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:3236) | 三星 PM1725、PM1725a 的具体 ID 匹配 |
| 5. 申请 BAR 并映射 MMIO | [pci.c:2785](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2785)，`nvme_dev_map()`；[pci.c:1690](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:1690)，`nvme_remap_bar()` | 资源申请、BAR0 地址和长度、`ioremap()` |
| 6. 读取 CAP、VS、CSTS | CAP：[pci.c:2395](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2395)；VS：[pci.c:1721](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:1721)；CSTS：[pci.c:2381](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2381) | `lo_hi_readq()` 读取 64 位寄存器，`readl()` 读取 32 位寄存器 |

下面几个细节对你这个阶段比较有用。

**三星设备的匹配方式**

匹配表里有这两个条目，省略兼容性标志后是：

```c
{ PCI_DEVICE(0x144d, 0xa821), /* Samsung PM1725 */ ... },
{ PCI_DEVICE(0x144d, 0xa822), /* Samsung PM1725a */ ... },
```

`0x144d` 是三星厂商 ID，后面的值是具体设备 ID。你以后写自己的驱动时，应填写手头 SSD 的实际 ID。

此外，[pci.c:3282](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:3282) 还有通用匹配：

```c
{ PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_EXPRESS, 0xffffff) },
```

因此，**三星设备即使没有单独列在表里，也可能通过 NVMe 类别匹配**。前面的具体设备条目主要还携带了各型号的兼容性处理标志。

**BAR 申请和映射是两个步骤**

把下面几处连起来看：

```c
/* nvme_dev_map()：申请设备的内存 BAR 资源 */
pci_request_mem_regions(pdev, "nvme");

/* nvme_remap_bar()：检查 BAR0 长度，映射 BAR0 */
pci_resource_len(pdev, 0);
dev->bar = ioremap(pci_resource_start(pdev, 0), size);
```

这里的参数 `0` 表示 **BAR0**；`dev->bar` 保存映射后的 MMIO 虚拟基址。注意，源码的 `pci_request_mem_regions()` 申请所有内存 BAR 的资源，后面的这次 `ioremap()` 才是明确映射 BAR0。

释放操作在 [pci.c:2456](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2456) 的 `nvme_dev_unmap()`：

```c
iounmap(dev->bar);
pci_release_mem_regions(to_pci_dev(dev->dev));
```

设备使能则在 [pci.c:2368](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2368) 的 `nvme_pci_enable()`，重点看 `pci_enable_device_mem()`。

**三个寄存器的读取**

偏移定义在 [kernel/include/linux/nvme.h:101](/home/lubancat/LubanCat_SDK/kernel/include/linux/nvme.h:101)：

| 寄存器 | 相对 MMIO 基址的偏移 | 宽度 | 含义 |
|---|---|---|---|
| CAP | `0x0000` | 64 位 | 控制器能力 |
| VS | `0x0008` | 32 位 | NVMe 版本 |
| CSTS | `0x001c` | 32 位 | 控制器状态 |

把源码中的读取动作单独提取出来，就是：

```c
u64 cap  = lo_hi_readq(dev->bar + NVME_REG_CAP);
u32 vs   = readl(dev->bar + NVME_REG_VS);
u32 csts = readl(dev->bar + NVME_REG_CSTS);
```

这些偏移属于 **BAR0 中的 NVMe 寄存器空间**。CAP 是 64 位，源码用 `lo_hi_readq()` 分两次读取并组合结果。

**建议按调用关系阅读**

先看文件末尾的 `nvme_driver` 和 [nvme_init()](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:3302)，再沿着下面的关系找：

```text
pci_register_driver()
  → 根据 nvme_id_table 匹配设备
  → nvme_probe()
      → nvme_dev_map()
          → nvme_remap_bar()              申请资源、映射 BAR0
      → nvme_reset_ctrl()
          → 调度 nvme_reset_work()
              → nvme_pci_enable()        使能 PCI，读取 CSTS、CAP
              → nvme_pci_configure_admin_queue()
                                         读取 VS
```

**这里最容易漏看的是异步初始化：`probe()` 通过工作队列启动后续流程，寄存器读取分散在后续函数中。** 你当前阶段先关注上面的 PCI 注册、匹配、映射和读取路径；队列、PRP、块层部分可以留到后面学习。

## 2026-11-03 16:20:13 · 用户

以probe()为例，我如果想自己实现它，我看这个linux源码，它里面又了很多已有的函数，比如用到函数a，然后我点进去，发现函数又运用了函数b，这样层层实现，而我自己想要实现这个驱动编写，学习NVME_PCIE，好让我去面试建兴存储的ssd开发，我无法去判断我的写的函数功能哪些是自己要写的，那些开源用linux自带的

## 2026-11-03 16:20:26 · 助手进度

你卡住的是“自己的驱动要负责什么、哪些能力由内核提供”这条边界。我会拿这份源码的 `probe()` 做例子，把它调用的函数分清楚，再对应到你第一阶段要写的内容。

## 2026-11-03 16:23:39 · 助手

**你需要自己实现的是“设备初始化流程和 NVMe 协议逻辑”；内存分配、PCI 资源管理、MMIO 访问、DMA、中断等通用机制，调用 Linux 提供的 API。**

写驱动本来就会大量调用内核函数。你要掌握的是：**为什么在这里调用、传什么参数、成功后获得什么、失败后如何清理，以及接下来硬件应该进入什么状态。**

结合你现在的 Stage 1，可以把边界划得很具体。

**先把源码中的函数分成三类。**

| 类别 | 这份源码中的例子 | 你写独立学习驱动时怎么处理 |
|---|---|---|
| Linux 通用 API | `kzalloc()`、`pci_enable_device_mem()`、`pci_request_region()`、`ioremap()`、`readl()` | 直接调用，学习使用约定 |
| 现有 NVMe PCI 驱动的内部函数 | `nvme_dev_map()`、`nvme_remap_bar()`、`nvme_pci_enable()` | 阅读其职责，实现符合当前阶段的自己的版本 |
| Linux NVMe 核心框架接口 | `nvme_init_ctrl()`、`nvme_reset_ctrl()` | 接入现有 NVMe host 框架时使用；你当前的 Stage 1 可以先不引入 |

另外，`struct pci_device_id`、`struct pci_driver` 的**类型已经由 Linux 定义**。你要编写的是自己的匹配表、驱动实例和回调函数。

比如：

```c
static struct pci_driver my_driver = {
    .name     = "my_nvme",
    .id_table = my_ids,
    .probe    = my_probe,
    .remove   = my_remove,
};
```

其中结构体类型、字段和回调签名由 Linux 规定；`my_ids`、`my_probe()`、`my_remove()` 的内容由你提供。

**拿实际的 `nvme_probe()` 拆开看，边界就清楚了。**

在 [pci.c:2879](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2879) 中：

```c
dev = kzalloc_node(...);
```

这是内核内存分配 API。你需要决定分配多大的私有结构、保存哪些状态，并处理分配失败；分配器本身由 Linux 实现。

```c
pci_set_drvdata(pdev, dev);
```

这是内核 PCI API。它把你的私有数据关联到 PCI 设备，方便 `remove()` 等回调通过 `pci_get_drvdata()` 取回来。

```c
result = nvme_dev_map(dev);
```

这是**现有 NVMe 驱动自己写的辅助函数**。它在 [pci.c:2785](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2785) 中定义：

```c
static int nvme_dev_map(struct nvme_dev *dev)
```

进入这个函数后，又会看到：

```c
pci_request_mem_regions(...);  /* Linux API */
nvme_remap_bar(...);           /* 该驱动的内部函数 */
```

再进入 `nvme_remap_bar()`，最终是：

```c
pci_resource_len(...);         /* Linux API */
pci_resource_start(...);       /* Linux API */
ioremap(...);                  /* Linux API */
```

所以，你自己写一个 `my_map_bar0()`，主要工作就是：

> 选择 BAR0 → 检查资源和长度 → 申请资源 → 调用映射 API → 保存地址 → 处理失败。

**这个流程由你实现，其中的资源管理和地址映射能力由 Linux 提供。** 辅助函数叫什么、拆成几个函数，都可以由你决定。

**你当前的 `probe()`，只需要承担下面这些职责。**

```text
my_probe()                              你实现并组织整个流程
 ├─ 分配自己的设备结构                   调用 kzalloc()
 ├─ 使能 PCI 内存访问                    调用 pci_enable_device_mem()
 ├─ 检查并申请 BAR0                      调用 PCI 资源 API
 ├─ 映射 BAR0                           调用 pci_iomap() 或 ioremap()
 ├─ 读取 CAP、VS、CSTS                   调用 lo_hi_readq()、readl()
 ├─ 保存自己的设备结构                   调用 pci_set_drvdata()
 └─ 某一步失败时，释放之前获得的资源       你设计回退路径
```

相应地，`my_remove()` 由你编写，调用内核 API 完成解除映射、释放 BAR、禁用 PCI 设备、释放私有内存。

**完成这条路径，就完成了你定义的 Stage 1。** 原版 `probe()` 里面的 PRP 池、管理队列、工作队列、命名空间、块设备注册，都属于后续功能。

原版驱动把部分初始化放到异步 reset 流程，是完整驱动的设计。你的第一阶段可以用较简单的同步流程完成 PCI 使能、映射和寄存器读取。

**判断一个函数能否直接使用，可以按这个顺序检查。**

1. **看声明和所属模块。**\
   `include/linux/pci.h` 中的 PCI API、`linux/slab.h` 中的内存 API，是常见的通用接口。某个驱动目录里的辅助函数，需要进一步判断其可见性和依赖。

2. **看是否是 `.c` 文件中的 `static` 函数。**\
   例如 `static int nvme_dev_map(...)`，只能在当前编译单元内使用，你的独立模块不能直接调用它。\
   注意：头文件中的 `static inline` 常常就是正常提供给驱动使用的接口，不能只看到 `static` 就下结论。

3. **外部模块调用非内联函数时，还要看符号是否导出。**\
   可以查 `EXPORT_SYMBOL()` 或 `EXPORT_SYMBOL_GPL()`。单纯去掉 `static`，不能让另一个模块自动获得调用能力。

4. **能调用，还要满足它的使用前提。**\
   例如 [nvme_reset_ctrl()](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/core.c:150) 确实导出了，但它主要是改变状态并调度 `reset_work`。它依赖已经建立好的 NVMe 核心对象和回调，并不提供独立完成硬件复位的全部逻辑。

因此，你当前不必为了调用 `nvme_init_ctrl()`，先把整个 `struct nvme_ctrl`、状态机、工作队列都搭起来。

**读源码时，也需要设置“停止下钻”的条件。**

遇到内核通用 API，先弄懂这些问题：

- 它完成什么事？
- 参数和返回值是什么？
- 调用前需要满足什么条件？
- 获得的资源何时、通过什么函数释放？

能回答这些，就可以回到当前驱动流程。

例如，理解 `pci_request_region()` 是申请设备已有的 BAR 资源范围、可能失败、需要配对释放，对 Stage 1 已经足够。资源树和锁的内部实现可以以后再读。

而遇到 `nvme_pci_configure_admin_queue()` 这样的设备协议逻辑，到了相应学习阶段，就应该继续深入。

你后面的实现边界可以这样递进：

| 阶段 | 你重点实现 | 继续复用 Linux 的能力 |
|---|---|---|
| PCI/MMIO | 匹配表、`probe/remove`、资源回退、寄存器读取流程 | PCI、内存分配、MMIO API |
| Admin Queue | 控制器启停顺序、RDY 超时等待、SQ/CQ 配置 | DMA 内存分配、MMIO API |
| Identify | 命令构造、提交、敲 doorbell、检查 CQ 完成和状态 | DMA、寄存器访问、已有协议结构体定义 |
| 中断与 I/O | 提交和完成路径、并发、超时处理 | IRQ、同步机制，后续再学习块层接口 |

为了面试，你可以逐渐练到能解释：

> “我用 Linux DMA API 分配队列内存，自己把队列的 DMA 地址配置进 NVMe 寄存器；自己填 SQ 命令、更新 doorbell，并根据 CQ 的 phase、CID 和状态字段处理完成。”

这里每一项都能明确区分：哪些机制来自 Linux，哪些 NVMe 行为由你实现。

你现在学习的是**主机侧 NVMe PCIe 驱动**。如果目标岗位偏 SSD 固件，这部分能帮助你理解主机与 SSD 的交互；FTL、NAND 管理等控制器内部逻辑，还需要按岗位要求另外学习。

## 2026-11-04 15:08:54 · 用户

看到我在kernel目录下重新写了个自己的框架吗，pcie_nvme的，我自己写的话，需要包括哪些头文件，makefile，我该怎么一步步来，告诉我我先怎么完善框架

## 2026-11-04 15:09:04 · 助手进度

我先看一下你在 `kernel` 下新建的 `pcie_nvme` 框架，再根据现有文件说明头文件、Makefile 和第一阶段代码该怎么补。

## 2026-11-04 15:10:23 · 助手进度

看到了，你的文件实际在 SDK 根目录的 `nvme_pcie/nvme_pcie.c`，目前有三星 ID 表和一个空的 `probe()`。这个位置可以按外部模块构建。建议先补齐“注册驱动、`probe/remove` 打印日志”这一步，编译通过后再加 BAR0 映射；我也在核对这套 SDK 的构建环境，避免给你用错内核或工具链。

## 2026-11-04 15:11:56 · 助手进度

建议的最小框架已在临时目录用 SDK 内核编译通过，生成了 ARM64 的 `.ko`。你当前是在 x86_64 主机上给 ARM64 内核编译，所以要显式指定工具链；下面给你的命令会对应这套 SDK。当前环境也没有看到 NVMe PCI 设备，因此这一步先以编译通过为验收。

## 2026-11-04 15:12:54 · 助手

看到了，你的文件实际在 SDK 根目录的 **[nvme_pcie/nvme_pcie.c](/home/lubancat/LubanCat_SDK/nvme_pcie/nvme_pcie.c:1)**，目前包含三星 ID 表和一个空的 `probe()`。

**先把它完善到“能够编译成模块、注册 PCI 驱动”的程度，再往 `probe()` 中添加 BAR0 操作。** 这个目录可以直接作为外部模块目录使用：

```text
LubanCat_SDK/
├── kernel/
└── nvme_pcie/
    ├── nvme_pcie.c
    └── Makefile
```

下面的最小示例，我已经在临时目录用你的 SDK 内核编译通过。

**第一步，补齐 C 文件的基本组成。**

当前只需要三个头文件：

| 头文件 | 当前用途 |
|---|---|
| `<linux/module.h>` | 模块注册辅助宏、模块信息 |
| `<linux/init.h>` | 初始化和退出相关定义 |
| `<linux/pci.h>` | PCI 设备类型、匹配表、驱动注册接口 |

你可以按下面的顺序补全自己的文件：

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

/* 1. 描述这个驱动支持哪些设备 */
static const struct pci_device_id lab_nvme_ids[] = {
    { PCI_DEVICE(0x144d, 0xa802) },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, lab_nvme_ids);

/* 2. 设备匹配成功后，由 PCI 核心调用 */
static int lab_nvme_probe(struct pci_dev *pdev,
                          const struct pci_device_id *id)
{
    dev_info(&pdev->dev,
             "lab_nvme probe: vendor=%04x device=%04x\n",
             pdev->vendor, pdev->device);

    return 0;
}

/* 3. 设备与驱动解除绑定时调用 */
static void lab_nvme_remove(struct pci_dev *pdev)
{
    dev_info(&pdev->dev, "lab_nvme remove\n");
}

/* 4. 把匹配表和回调函数交给 PCI 核心 */
static struct pci_driver lab_nvme_driver = {
    .name     = "lab_nvme",
    .id_table = lab_nvme_ids,
    .probe    = lab_nvme_probe,
    .remove   = lab_nvme_remove,
};

/* 5. 注册模块初始化和退出函数 */
module_pci_driver(lab_nvme_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Learning NVMe PCI driver framework");
```

这里有三个需要理解的地方：

- **`probe()` 必须返回一个 `int`。** 返回 `0` 表示绑定成功，负错误码表示失败。你现在的空函数缺少返回值。
- **`MODULE_DEVICE_TABLE()` 提供模块的设备匹配信息。** 实际驱动注册由注册接口完成。
- **`module_pci_driver()` 替你生成初始化和退出函数**，内部调用 `pci_register_driver()`、`pci_unregister_driver()`。用了它，就不用再手写一套 `module_init()` 和 `module_exit()`。

`0x144d:0xa802` 先保留你填写的值，后续需要在目标板上确认实际 SSD 的 PCI ID。

**第二步，创建最简单的 Makefile。**

在 `nvme_pcie/Makefile` 中先写一行：

```makefile
obj-m += nvme_pcie.o
```

含义是：让内核构建系统把 `nvme_pcie.c` 编译成可加载模块 `nvme_pcie.ko`。

然后执行：

```bash
cd /home/lubancat/LubanCat_SDK/nvme_pcie

make -C ../kernel M="$PWD" \
    ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
```

这些参数分别表示：

| 参数 | 含义 |
|---|---|
| `-C ../kernel` | 使用 SDK 内核的构建系统 |
| `M="$PWD"` | 构建当前目录中的外部模块 |
| `ARCH=arm64` | 目标架构为 ARM64 |
| `CROSS_COMPILE=aarch64-linux-gnu-` | 使用对应的交叉编译工具链 |
| `modules` | 构建模块 |

**不需要自己用 `gcc` 编译，也不需要手动复制内核头文件或配置一堆 `-I` 路径。** 内核构建系统会处理这些。

我检查过，你的 SDK 已有内核配置、生成头文件和 `Module.symvers`。临时示例生成的模块信息是：

```text
架构：     ARM aarch64
vermagic： 5.10.160 SMP mod_unload aarch64
```

它面向与这套 SDK 构建匹配的 ARM64 内核；当前执行主机是 x86_64，不能直接加载这个模块。

**第三步，编译通过后，再按小步骤扩充 `probe()`。**

| 顺序 | 你添加的内容 | 完成标准 |
|---|---|---|
| ① | 上面的基本框架 | 成功生成 `nvme_pcie.ko` |
| ② | 私有设备结构、分配与释放 | `probe()` 保存数据，`remove()` 能取回并释放 |
| ③ | PCI 使能、申请并映射 BAR0 | 每一步检查返回值，失败时释放此前取得的资源 |
| ④ | 读取 CAP、VS、CSTS | 能打印三个寄存器的值，并解释宽度和偏移 |

后续按功能添加头文件即可：

| 添加的功能 | 对应头文件 |
|---|---|
| `kzalloc()`、`kfree()` | `<linux/slab.h>` |
| `ioremap()`、`readl()` 等 MMIO 操作 | `<linux/io.h>` |
| `NVME_REG_CAP` 等寄存器定义 | `<linux/nvme.h>` |
| `lo_hi_readq()` 读取 64 位 CAP | `<linux/io-64-nonatomic-lo-hi.h>` |

其中 `<linux/nvme.h>` 是可复用的协议定义；现阶段不需要引入 `drivers/nvme/host/nvme.h` 中完整驱动框架的私有对象。

最后，**编译成功、模块加载成功、`probe()` 被调用，是三个不同的检查点**。只有存在匹配且可以绑定的 PCI 设备时，才会进入你的 `probe()`；SSD 已由原生 `nvme` 驱动占用时，仅加载你的模块通常不会触发它。

你现在先完成上面的 C 文件和单行 Makefile，以成功生成 `.ko` 作为这一小步的验收标准。

## 2026-11-04 15:23:45 · 用户

为什么我的nvme_pcie.c文件里这么多标红，还有请帮我完善makefile，并写上注释

## 2026-11-04 15:23:50 · 助手进度

我先检查你当前的代码、Makefile 和编辑器配置，区分标红是代码错误还是头文件路径问题，再补好带注释的 Makefile 并验证编译。

## 2026-11-04 15:25:24 · 助手进度

编译器确认了两处代码错误：`pci_driver` 引用了尚未声明的 `probe/remove`，而 `probe()` 缺少返回值。另外，你用的是 clangd，本模块还没有自己的编译参数数据库。我会修正这两处代码，并让 Makefile 自动生成供 clangd 使用的编译参数，添加必要注释。

## 2026-11-04 15:26:52 · 助手进度

模块已经编译通过。clangd 还遇到了 GCC 的 `-mabi=lp64` 参数不兼容问题，我已在本模块的编辑器配置中处理；这不会改变实际编译参数。现在 clangd 检查也通过了，接着验证清理和重新构建。

## 2026-11-04 15:27:51 · 助手

已修改完成，Makefile 中也补了中文注释。

标红主要有三个原因：

1. **函数先使用、后声明**：`pci_driver` 引用 `lab_nvme_probe/remove` 时，编译器还不认识它们。已把驱动结构体移到函数定义后面。
2. **`probe()` 缺少返回值**：它返回 `int`，已补上日志和 `return 0`。
3. **clangd 缺少本模块的编译参数**：内核代码需要架构头文件、配置宏等信息，普通 C 文件配置不足以解析。

对应修改：

- [nvme_pcie.c](/home/lubancat/LubanCat_SDK/nvme_pcie/nvme_pcie.c:1)：修复上述代码错误，补充 `MODULE_DEVICE_TABLE()` 和注释。
- [Makefile](/home/lubancat/LubanCat_SDK/nvme_pcie/Makefile:1)：配置 SDK 内核路径、ARM64 工具链、编译与清理目标，并自动生成 `compile_commands.json`。
- [.clangd](/home/lubancat/LubanCat_SDK/nvme_pcie/.clangd:1)：读取真实编译参数，处理 clangd 不兼容的 GCC 参数。

现在可以直接执行：

```bash
cd /home/lubancat/LubanCat_SDK/nvme_pcie

make          # 编译模块，同时更新编辑器编译参数
make clean    # 清理本模块的构建产物
make V=1      # 查看详细编译命令
```

**已验证清理后重新编译成功，生成 ARM64 模块；clangd 检查为 0 errors。**

如果 VS Code 仍显示旧的红线，按 `Ctrl+Shift+P`，执行 **`clangd: Restart language server`**。

## 2026-11-04 16:03:31 · 用户

static int nvme_dev_map(struct nvme_dev *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev->dev);

	if (pci_request_mem_regions(pdev, "nvme"))
		return -ENODEV;

	if (nvme_remap_bar(dev, NVME_REG_DBS + 4096))
		goto release;

	return 0;
  release:
	pci_release_mem_regions(pdev);
	return -ENODEV;
}
在这个nvme_probe函数里面详细分析这个nvme_dev_map函数，具体作用，详细分析

## 2026-11-04 16:03:37 · 助手进度

我会把它和 `nvme_remap_bar()` 连起来分析，重点讲清楚：它申请了什么资源、映射了什么地址，以及失败时如何回退。

## 2026-11-04 16:07:07 · 助手

这个函数位于 [pci.c:2785](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2785)。它在 `nvme_probe()` 中负责完成两件事：

1. **向 Linux 申请该 PCI 设备的内存 BAR 资源使用权。**
2. **把 BAR0 前面的一段寄存器空间映射到内核虚拟地址，供后续访问。**

执行成功后，后面的驱动代码就有了 `dev->bar`，可以通过它访问 NVMe 控制器寄存器。

下面按实际执行顺序分析。

**1. 先理解传入的 `dev` 是什么。**

```c
static int nvme_dev_map(struct nvme_dev *dev)
```

这里的 `struct nvme_dev` 是 **NVMe PCI 驱动自己定义的设备私有结构体**，定义在 [pci.c:112](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:112)。

与当前函数关系最大的成员是：

```c
struct nvme_dev {
    /* 其他成员省略 */
    struct device *dev;
    void __iomem *bar;
    unsigned long bar_mapped_size;
    u32 __iomem *dbs;
};
```

| 成员 | 用途 |
|---|---|
| `dev` | 指向 Linux 通用设备对象 |
| `bar` | 保存 BAR0 映射后的 MMIO 虚拟基址 |
| `bar_mapped_size` | 记录目前已经映射了多少字节 |
| `dbs` | 指向映射区域中的 doorbell 起始位置 |

函数前面的 `static` 表示它是当前 `.c` 文件内部使用的函数。

返回值遵循这里的约定：

```text
0       → 成功
负错误码 → 失败
```

**2. 从 NVMe 私有结构中找回 PCI 设备对象。**

```c
struct pci_dev *pdev = to_pci_dev(dev->dev);
```

这里涉及三个结构体：

| 类型 | 谁定义 | 表示什么 |
|---|---|---|
| `struct nvme_dev` | NVMe PCI 驱动 | 驱动自己的设备状态 |
| `struct pci_dev` | Linux PCI 子系统 | PCI 设备信息、BAR 资源等 |
| `struct device` | Linux 设备模型 | 各类设备共用的基础对象 |

`struct pci_dev` 内部包含一个 `struct device`：

```c
struct pci_dev {
    /* 其他成员 */
    struct device dev;
};
```

在前面的 `nvme_probe()` 中，[pci.c:2902](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2902) 已经执行了：

```c
dev->dev = get_device(&pdev->dev);
```

因此，NVMe 私有结构中的 `dev->dev`，指向的就是 PCI 设备内部的那个通用设备对象。

`to_pci_dev()` 的定义是：

```c
#define to_pci_dev(n) container_of(n, struct pci_dev, dev)
```

它通过内部成员的地址，找回外层 `struct pci_dev` 的地址。

**这一步只是恢复指针，没有创建新设备，也没有分配内存。**

---

**3. 申请设备内存 BAR 的资源使用权。**

```c
if (pci_request_mem_regions(pdev, "nvme"))
    return -ENODEV;
```

先看参数：

| 参数 | 含义 |
|---|---|
| `pdev` | 要申请哪个 PCI 设备的资源 |
| `"nvme"` | 登记资源使用者的名称，便于查看和排查 |

`"nvme"` 是资源登记名称，设备匹配仍由 PCI ID 表决定。

`pci_request_mem_regions()` 的定义在 [pci.h:1691](/home/lubancat/LubanCat_SDK/kernel/include/linux/pci.h:1691)：

```c
return pci_request_selected_regions(pdev,
                    pci_select_bars(pdev, IORESOURCE_MEM), name);
```

可以把它理解为：

```text
找出设备的内存 BAR
    ↓
逐个申请对应地址区间的使用权
    ↓
在内核资源管理系统中登记使用者
```

**这里申请的是该设备所有符合条件的内存 BAR，范围不只限于 BAR0。**

“申请资源”具体指什么？假设设备 BAR0 对应的主机资源区间是：

```text
0x80000000 ～ 0x80003fff
```

驱动向内核登记：我要使用这段地址范围。如果存在资源占用冲突，申请就可能失败。

它与普通内存分配的区别可以这样看：

| 操作 | 获得什么 |
|---|---|
| `kzalloc()` | 用于保存软件数据的普通内存 |
| `pci_request_mem_regions()` | 设备已有 BAR 地址区间的使用权 |
| `ioremap()` | 用于访问设备地址区间的内核虚拟地址 |

所以，执行完 `pci_request_mem_regions()`，**还没有得到可供 `readl()` 使用的 MMIO 地址**。

这段代码也等价于：

```c
int ret;

ret = pci_request_mem_regions(pdev, "nvme");
if (ret != 0)
    return -ENODEV;
```

底层发生资源冲突时通常返回 `-EBUSY`，这里统一转换成了 `-ENODEV`。因此，看到这个返回值时，还要结合日志判断原因。

申请多个 BAR 时，如果中途失败，PCI API 内部会释放本次已经申请成功的资源，所以这里可以直接返回。

**4. 映射 BAR0，重点理解长度参数。**

```c
if (nvme_remap_bar(dev, NVME_REG_DBS + 4096))
    goto release;
```

`nvme_remap_bar()` 是这个驱动自己写的辅助函数，定义在 [pci.c:1690](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:1690)。

它的第二个参数是：**希望从 BAR0 起始位置映射多少字节。**

在 [linux/nvme.h:131](/home/lubancat/LubanCat_SDK/kernel/include/linux/nvme.h:131) 中：

```c
NVME_REG_DBS = 0x1000;
```

所以：

```text
NVME_REG_DBS + 4096
= 0x1000 + 0x1000
= 0x2000
= 8192 字节
```

这次映射覆盖 BAR0 的以下偏移：

| 相对 BAR0 的偏移 | 内容 |
|---|---|
| `0x0000` | CAP |
| `0x0008` | VS |
| `0x001c` | CSTS |
| `0x1000` | Doorbell 区域开始 |
| `0x2000` | 本次映射的结束边界，不包含该位置 |

也就是：

```text
[0x0000, 0x1000)  控制器寄存器区域
[0x1000, 0x2000)  初始覆盖的 doorbell 区域
```

**8192 字节是驱动此次选择的初始映射长度，并不表示 BAR0 的硬件长度就是 8192 字节。** 后续会根据队列数量、doorbell 间隔等需求调整映射范围。这里的 `4096` 也是固定字节数，不必把它理解为当前系统的 `PAGE_SIZE`。

再进入 `nvme_remap_bar()`，逐句看。

```c
if (size <= dev->bar_mapped_size)
    return 0;
```

如果已有映射足够大，就直接复用。

初次进入时，`dev` 来自 `kzalloc_node()`，因此：

```c
dev->bar = NULL;
dev->bar_mapped_size = 0;
```

本次需要 8192 字节，会继续往下执行。

```c
if (size > pci_resource_len(pdev, 0))
    return -ENOMEM;
```

`pci_resource_len(pdev, 0)` 获取 BAR0 的资源长度，参数 `0` 表示 BAR0。

这一步确保所需范围没有超过 BAR0。

```c
if (dev->bar)
    iounmap(dev->bar);
```

如果原来存在一个较小的映射，先解除旧映射。第一次调用时 `dev->bar == NULL`，跳过这一步。

真正建立映射的是：

```c
dev->bar = ioremap(pci_resource_start(pdev, 0), size);
```

这里：

- `pci_resource_start(pdev, 0)`：取得 BAR0 的主机资源起始地址。
- `size`：本次需要映射的长度。
- `ioremap()`：建立设备地址区间到内核虚拟地址的映射。
- `dev->bar`：保存返回的 MMIO 虚拟基址。

地址关系可以写成：

```text
BAR0 的主机资源地址
        ↓ ioremap()
内核 MMIO 虚拟地址 dev->bar
        ↓ 加上寄存器偏移
readl() / writel() 访问控制器寄存器
```

应当通过 `pci_resource_start()` 获取资源地址，因为 PCI 总线地址与主机物理地址可能经过平台转换。

随后检查映射是否成功：

```c
if (!dev->bar) {
    dev->bar_mapped_size = 0;
    return -ENOMEM;
}
```

成功后保存状态：

```c
dev->bar_mapped_size = size;
dev->dbs = dev->bar + NVME_REG_DBS;
```

此时：

```text
dev->bar             → BAR0 映射起点
dev->bar_mapped_size  → 8192
dev->dbs             → 映射起点 + 0x1000
```

`dbs` 只是一个指向 doorbell 区域的地址。SQ/CQ 队列本身还没有在这里创建。

**5. 映射失败，为什么跳到 `release`？**

回到外层函数：

```c
if (nvme_remap_bar(dev, NVME_REG_DBS + 4096))
    goto release;
```

执行到这一步，说明前面的资源申请已经成功。如果映射失败，驱动不能继续初始化，就需要撤销已经成功完成的资源申请：

```c
release:
    pci_release_mem_regions(pdev);
    return -ENODEV;
```

`release:` 是普通 C 标签，`goto release` 跳到这里执行清理。

整个函数的三条路径是：

| 路径 | 清理动作 | 返回值 |
|---|---|---|
| BAR 资源申请失败 | 申请 API 内部已经回退 | `-ENODEV` |
| 申请成功，映射失败 | 释放已申请的 BAR 资源 | `-ENODEV` |
| 申请成功，映射成功 | 保留资源和映射供后续使用 | `0` |

这里可能会产生一个疑问：**映射失败为什么没有 `iounmap()`？**

因为这是第一次映射：

- 如果长度检查失败，根本没有建立映射。
- 如果 `ioremap()` 失败，返回的是 `NULL`，也没有成功建立的映射需要解除。

因此，这条失败路径只需释放此前成功申请的资源使用权。

这体现了驱动初始化的一个基本原则：**失败时，根据已经完成的步骤，按相反顺序释放资源。**

**6. 成功返回后，`probe()` 获得了什么？**

```c
return 0;
```

它表示：

- 内存 BAR 资源已经申请成功。
- BAR0 的初始映射已经建立。
- `dev->bar`、`dev->bar_mapped_size`、`dev->dbs` 已准备好。

后续使能设备后，就可以执行这样的访问：

```c
u32 vs = readl(dev->bar + NVME_REG_VS);
```

要注意，**`nvme_dev_map()` 自身只负责资源申请和映射**。PCI 使能、寄存器读取、控制器启动、管理队列创建，都由后续流程完成。

如果映射成功，但 `probe()` 后面的初始化失败，会调用 [nvme_dev_unmap()](/home/lubancat/LubanCat_SDK/kernel/drivers/nvme/host/pci.c:2456)：

```c
static void nvme_dev_unmap(struct nvme_dev *dev)
{
    if (dev->bar)
        iounmap(dev->bar);

    pci_release_mem_regions(to_pci_dev(dev->dev));
}
```

这时需要依次解除映射、释放资源，因为这两件事此前都已成功。

对应到你自己的驱动，**`nvme_dev_map()` 和 `nvme_remap_bar()` 是可以参考并实现的流程封装；`pci_request_mem_regions()`、`pci_resource_start()`、`ioremap()` 等基础能力由 Linux 提供，直接调用。**

## 2026-11-05 09:13:05 · 用户

```kotlin
return pci_request_selected_regions(pdev,
                    pci_select_bars(pdev, IORESOURCE_MEM), name);
```

可以把它理解为：

找出设备的内存 BAR\
&#x20;   ↓\
逐个申请对应地址区间的使用权\
&#x20;   ↓\
在内核资源管理系统中登记使用者

这个函数我还是没有理解

## 2026-11-05 09:14:02 · 助手

可以把这句理解为：**根据设备已有的 BAR 信息，选出内存类型的 BAR，然后向 Linux 登记这些地址范围由当前驱动使用。**

我们用具体例子走一遍。

先把原来的一行拆成两行：

```c
int bars;

/* 选择哪些 BAR */
bars = pci_select_bars(pdev, IORESOURCE_MEM);

/* 申请使用这些 BAR */
return pci_request_selected_regions(pdev, bars, name);
```

这里两个函数做的事情不同：**第一个负责选择，第二个负责登记占用。**

**① `pdev` 中已经保存了设备的 BAR 信息。**

PCI 枚举和资源分配后，Linux 会把设备资源记录到 `pdev->resource[]` 中。

假设某个 PCI 设备有以下资源，地址仅用于举例：

| BAR | 资源类型 | 起始地址 | 长度 |
|---|---|---|---|
| BAR0 | 内存资源 | `0x80000000` | `0x4000` |
| BAR2 | 内存资源 | `0x90000000` | `0x1000` |
| BAR4 | I/O 端口资源 | `0x2000` | `0x100` |

`IORESOURCE_MEM` 表示：**我要筛选“内存地址空间类型”的资源。** 设备 MMIO 寄存器就属于这类资源。

**② `pci_select_bars()` 返回一个“选择结果”。**

```c
bars = pci_select_bars(pdev, IORESOURCE_MEM);
```

根据上面的表，它选择：

```text
BAR0：符合
BAR2：符合
BAR4：类型不同，跳过
```

这个选择结果用**位图**表示：

```text
位编号：  5 4 3 2 1 0
位的值：  0 0 0 1 0 1
                ↑   ↑
              BAR2 BAR0
```

因此：

```c
bars = (1 << 0) | (1 << 2);
     = 5;
```

**这个 `5` 表示“选择 BAR0 和 BAR2”。** 每一个二进制位，对应一个 BAR 编号。

到这里，只完成了选择，还没有申请资源。

**③ `pci_request_selected_regions()` 根据位图逐个申请。**

现在第二行相当于：

```c
pci_request_selected_regions(pdev, 5, "nvme");
```

它会根据位图，分别申请 BAR0 和 BAR2，效果相当于调用：

```c
pci_request_region(pdev, 0, "nvme");
pci_request_region(pdev, 2, "nvme");
```

实际实现还会检查返回值、处理失败回退。

具体申请的地址范围，从 `pdev` 中取：

```text
BAR0 → 0x80000000 ～ 0x80003fff
BAR2 → 0x90000000 ～ 0x90000fff
```

然后 Linux 在资源管理系统中登记占用：

| 地址范围 | 申请前 | 申请成功后 |
|---|---|---|
| BAR0 的范围 | 尚未被驱动占用 | 使用者：`nvme` |
| BAR2 的范围 | 尚未被驱动占用 | 使用者：`nvme` |

`"nvme"` 就是这份登记中的使用者名称。

如果某段地址范围已经存在占用冲突，申请会失败。如果 BAR0 申请成功、BAR2 申请失败，这个函数会把刚申请的 BAR0 也释放掉，再返回错误。

**④ 为什么还要申请？地址已经有了呀？**

因为“知道地址”和“取得使用权”是两个步骤。

可以类比车位：

- `pdev->resource[]`：记录车位的位置和大小。
- `pci_select_bars()`：选出你要使用的几个车位。
- `pci_request_selected_regions()`：检查占用情况，并登记由你使用。

对应驱动，登记地址范围有助于 Linux 发现资源冲突，避免多个使用者争用同一段设备资源。

**所以，这行代码执行成功后，驱动取得了 BAR 地址范围的使用权。接下来还要调用 `ioremap()` 获得 MMIO 虚拟地址，然后才能用 `readl()`、`writel()` 访问寄存器。**

```text
pci_select_bars()                 选择哪些 BAR
        ↓
pci_request_selected_regions()   登记地址范围的使用权
        ↓
ioremap()                        建立 MMIO 地址映射
        ↓
readl() / writel()               读写设备寄存器
```
