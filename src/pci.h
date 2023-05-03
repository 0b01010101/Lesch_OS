#ifndef     PCI_H
#define     PCI_H

#include    "common.h"

//CONFIG ADDR of REGISTER
//Offset:
#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02
#define PCI_COMMAND              0x04
#define PCI_STATUS               0x06
#define PCI_REVISION_ID          0x08
#define PCI_PROG_IF              0x09
#define PCI_SUBCLASS             0x0a
#define PCI_CLASS                0x0b
#define PCI_CACHE_LINE_SIZE      0x0c
#define PCI_LATENCY_TIMER        0x0d
#define PCI_HEADER_TYPE          0x0e
#define PCI_BIST                 0x0f
#define PCI_BAR0                 0x10
#define PCI_BAR1                 0x14
#define PCI_BAR2                 0x18
#define PCI_BAR3                 0x1C
#define PCI_BAR4                 0x20
#define PCI_BAR5                 0x24
#define PCI_INTERRUPT_LINE       0x3C
#define PCI_SECONDARY_BUS        0x19

// Device type
#define PCI_HEADER_TYPE_DEVICE   0
#define PCI_HEADER_TYPE_BRIDGE   1
#define PCI_HEADER_TYPE_CARDBUS  2
#define PCI_TYPE_BRIDGE          0x0604
#define PCI_TYPE_SATA            0x0106
#define PCI_NONE                 0xFFFF

//PORTS
#define PCI_CONFIG_ADDR          0x0CF8
#define PCI_CONFIG_DATA          0x0CFC

#define PER_DEVISE_FUNC          32
#define PER_DEVISE_BUS           32

union pci_dev {

    u32int      bits;
    struct {
        u32int always_zero    : 2;
        u32int field_num      : 6;
        u32int function_num   : 3;
        u32int device_num     : 5;
        u32int bus_num        : 8;
        u32int reserved       : 7;
        u32int enable         : 1;
    };
};

typedef union pci_dev pci_dev_s;

void pci_init(void);
void pci_write(pci_dev_s device, u32int offset, u32int value);
u32int pci_read(pci_dev_s device, u32int offset);
u32int get_dev_type(pci_dev_s dev);
u32int pci_reach_end(pci_dev_s dev);
u32int get_second_bus(pci_dev_s dev);
pci_dev_s pci_scan_func(u16int vendor_id, u16int dev_id, u32int bus, u32int dev, u32int function, s32int dev_type);
pci_dev_s pci_scan_bus(u16int vendor_id, u16int dev_id, u32int bus, s32int dev_type);
pci_dev_s pci_scan_dev(u16int vendor_id, u16int dev_id, u32int bus, u32int dev, s32int dev_type);
pci_dev_s pci_get_device(u16int vendor_id, u16int dev_id, s32int dev_type);

#endif