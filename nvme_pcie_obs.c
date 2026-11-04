// nvme_pcie_obs.c

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

static int __init nvme_obs_init(void)
{
    struct pci_dev *pdev;
    u16 link_status, msix_ctrl;
    int pos;

    /* 查找 PCIe 设备 0000:01:00.0 */
    pdev = pci_get_domain_bus_and_slot(
        0, 1, PCI_DEVFN(0, 0));

    if (!pdev) {
        pr_err("NVMe device not found\n");
        return -ENODEV;
    }

    pr_info("Vendor ID: 0x%04x\n", pdev->vendor);
    pr_info("Device ID: 0x%04x\n", pdev->device);
    pr_info("Class: 0x%06x\n", pdev->class);

    /* 读取 BAR0 资源 */
    pr_info("BAR0: %pR\n", &pdev->resource[0]);
    pr_info("BAR0 size: %llu bytes\n",
            (unsigned long long)pci_resource_len(pdev, 0));

    /* 查找 PCI Express Capability */
    pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);

    if (pos &&
        pci_read_config_word(pdev,
            pos + PCI_EXP_LNKSTA, &link_status) == 0) {

        pr_info("Link speed code: %u\n",
                link_status & PCI_EXP_LNKSTA_CLS);

        pr_info("Link width: x%u\n",
                (link_status & PCI_EXP_LNKSTA_NLW) >> 4);
    }

    /* 查找 MSI-X Capability */
    pos = pci_find_capability(pdev, PCI_CAP_ID_MSIX);

    if (pos &&
        pci_read_config_word(pdev,
            pos + PCI_MSIX_FLAGS, &msix_ctrl) == 0) {

        pr_info("MSI-X vectors supported: %u\n",
                (msix_ctrl & PCI_MSIX_FLAGS_QSIZE) + 1);
    }

    pci_dev_put(pdev);
    return 0;
}

static void __exit nvme_obs_exit(void)
{
    pr_info("NVMe PCIe observer exit\n");
}

module_init(nvme_obs_init);
module_exit(nvme_obs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RK3588 NVMe PCIe observer");