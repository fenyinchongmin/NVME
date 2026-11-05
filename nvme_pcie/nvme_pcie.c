#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

/* PCI 核心用这张表判断驱动是否支持某个设备。 */
static const struct pci_device_id lab_nvme_ids[] =
{
    { PCI_DEVICE(0x144d, 0xa802) },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, lab_nvme_ids);

/* 先定义回调，再在下面的 pci_driver 中引用它们。 */
static int lab_nvme_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
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
