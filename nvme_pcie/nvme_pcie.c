#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/nvme.h>

struct nvme_dev;
struct nvme_queue;


struct nvme_queue {
    struct nvme_dev *dev;
    int qid;
};

struct nvme_dev {
	struct nvme_queue *queues;
	struct device *dev;
    u32 __iomem *dbs;
    unsigned long bar_mapped_size;
    void __iomem *bar;
};
/* PCI 核心用这张表判断驱动是否支持某个设备。 */
static const struct pci_device_id lab_nvme_ids[] =
{
    { PCI_DEVICE(0x144d, 0xa802) },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, lab_nvme_ids);


static int lab_nvme_dev_map(struct nvme_dev *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev->dev);
    unsigned long map_size = NVME_REG_DBS + 4096;
    int ret;
    /* 1. 确认 BAR0 是内存资源，可以通过 MMIO 访问。 */
    if(!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM))
        return -ENODEV;
    /* 2. 确认 BAR0 的实际长度足够，避免访问越界。 */
    if (pci_resource_len(pdev, 0) < map_size)
        return -ENODEV;

    /* 3. 申请 BAR0 地址区间的使用权。 */
    ret = pci_request_region(pdev, 0, "lab_nvme");
    if (ret)
        return ret;
	/* 4. 将 BAR0 前 8 KB 映射到内核虚拟地址空间。 */
    dev->bar = pci_iomap(pdev, 0, map_size);
    if (!dev->bar) {
        /* 映射失败，归还刚才申请的 BAR0。 */
        pci_release_region(pdev, 0);
        return -ENOMEM;
    };

	/* 5. 保存 Doorbell 区域的起始虚拟地址。 */
    dev->dbs = dev->bar + NVME_REG_DBS;

    return 0;
}

/* 先定义回调，再在下面的 pci_driver 中引用它们。 */
static int lab_nvme_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int result = -ENOMEM;
    struct nvme_dev *dev;
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;
    dev->dev = &pdev->dev;
    pci_set_drvdata(pdev, dev);


    result = lab_nvme_dev_map(dev);
    dev_info(&pdev->dev, "NVMe probe: vendor=%04x device=%04x\n",
             pdev->vendor, pdev->device);
    
    
    /* 返回 0 表示绑定成功；后续在这里添加 PCI 使能和 BAR0 映射。 */
    return 0;
}

static void lab_nvme_remove(struct pci_dev *pdev)
{
    dev_info(&pdev->dev, "NVMe driver removed\n");
}

static struct pci_driver lab_nvme_driver = {
    .name     = "lab_nvme",
    .id_table = lab_nvme_ids,
    .probe    = lab_nvme_probe,
    .remove   = lab_nvme_remove,
};

/* 自动生成模块入口和出口，注册与注销上面的 PCI 驱动。 */
module_pci_driver(lab_nvme_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Learning NVMe PCI driver framework");
