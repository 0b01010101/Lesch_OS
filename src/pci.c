#include "pci.h"

u32int pci_map_size[100];
pci_dev_s dev_null = {0};

void pci_init(void) {

    pci_map_size[PCI_VENDOR_ID] = 2;
    pci_map_size[PCI_DEVICE_ID] = 2;
    pci_map_size[PCI_COMMAND] = 2;
    pci_map_size[PCI_STATUS] = 2;

    pci_map_size[PCI_SUBCLASS] = 1;
    pci_map_size[PCI_CLASS] = 1;
    pci_map_size[PCI_CACHE_LINE_SIZE] = 1;
    pci_map_size[PCI_LATENCY_TIMER] = 1;
    pci_map_size[PCI_HEADER_TYPE] = 1;
    pci_map_size[PCI_BIST] = 1;
    pci_map_size[PCI_INTERRUPT_LINE] = 1;
    pci_map_size[PCI_SECONDARY_BUS] = 1;

    pci_map_size[PCI_BAR0] = 4;
    pci_map_size[PCI_BAR1] = 4;
    pci_map_size[PCI_BAR2] = 4;
    pci_map_size[PCI_BAR3] = 4;
    pci_map_size[PCI_BAR4] = 4;
    pci_map_size[PCI_BAR5] = 4;

    return;
}

u32int pci_read(pci_dev_s device, u32int offset) {

    device.field_num = (offset & 0xFC) >> 2;
    device.enable = 1;
    outl(PCI_CONFIG_ADDR, device.bits);

    u32int size = pci_map_size[offset];
    if(size == 1) {
        u8int tmp = inb(PCI_CONFIG_DATA + (offset & 3));
        return tmp;
    }
    if(size == 2) {
        u16int tmp = inw(PCI_CONFIG_DATA + (offset & 2));
        return tmp;
    }
    if(size == 4) {
        u32int tmp = inl(PCI_CONFIG_DATA);
        return tmp;
    }

    return 0xFFFF;
}

void pci_write(pci_dev_s device, u32int offset, u32int value) {

    device.field_num = (offset & 0xFC) >> 2;
    device.enable = 1;
    outl(PCI_CONFIG_ADDR, device.bits);
    outl(PCI_CONFIG_DATA, value);

    return;
}

u32int get_dev_type(pci_dev_s dev) {

    u32int tmp = pci_read(dev, PCI_CLASS) << 8;
    tmp |= pci_read(dev, PCI_SUBCLASS);
    return tmp;
}

u32int pci_reach_end(pci_dev_s dev) {

    u32int tmp = pci_read(dev, PCI_HEADER_TYPE);    //PCI_HEADER_TYPE == 0 is end point
    return !tmp; 
}

u32int get_second_bus(pci_dev_s dev) {

    u32int tmp = pci_read(dev, PCI_SECONDARY_BUS);      //Get secondary bus from a PCI bridge device
    return tmp;
}
//------------------------------------------------------------------------------------------------------

pci_dev_s pci_scan_func(u16int vendor_id, u16int dev_id, u32int bus, u32int device, u32int function, s32int dev_type) {

    pci_dev_s dev = {0};
    dev.bus_num = bus;
    dev.device_num = device;
    dev.function_num = function;

    if(get_dev_type(dev) == PCI_TYPE_BRIDGE) {

        u32int sec_bus = get_second_bus(dev);
        pci_scan_bus(vendor_id, dev_id, sec_bus, dev_type);
    }
    if((dev_type == -1) || (dev_type == get_dev_type(dev)) ) {

        u32int vendor = pci_read(dev, PCI_VENDOR_ID);
        u32int devid = pci_read(dev, PCI_DEVICE_ID);
        if((devid == dev_id) && (vendor == vendor_id)) {
            return dev;
        }
    }
    return dev_null;
}

pci_dev_s pci_scan_bus(u16int vendor_id, u16int dev_id, u32int bus, s32int dev_type) {

    for(u32int device = 0; device < PER_DEVISE_BUS; device++) {
        pci_dev_s tmp = pci_scan_dev(vendor_id, dev_id, bus, device, dev_type);

        if(tmp.bits) {
            return tmp;
        }
    }
    return dev_null;
}

pci_dev_s pci_scan_dev(u16int vendor_id, u16int dev_id, u32int bus, u32int dev, s32int dev_type) {

    pci_dev_s device = {0};
    device.bus_num = bus;
    device.device_num = dev;

    if(pci_read(device, PCI_VENDOR_ID) == PCI_NONE) {
        return dev_null;
    }
    pci_dev_s tmp = pci_scan_func(vendor_id, dev_id, bus, dev, 0, dev_type);
    if(tmp.bits) {
        return tmp;
    }
    if(pci_reach_end(device)) {
        return dev_null;
    }
    for(u32int func = 1; func < PER_DEVISE_FUNC; func++) {
        
        if(pci_read(device, PCI_VENDOR_ID) != PCI_NONE) {
            tmp = pci_scan_func(vendor_id, dev_id, bus, dev, func, dev_type);
            
            if(tmp.bits) {
                return tmp;
            }
        }
    }
    return dev_null;
}

pci_dev_s pci_get_device(u16int vendor_id, u16int dev_id, s32int dev_type) {

    pci_dev_s tmp = pci_scan_bus(vendor_id, dev_id, 0, dev_type);

    if(tmp.bits) {
        return tmp;
    }
    if(pci_reach_end(dev_null)) {
        monitor_str_write("PCI get devise failed!!!\n");
    }
    for(u32int func = 1; func < PER_DEVISE_FUNC; func++) {
        pci_dev_s dev = {0};
        dev.function_num = func;

        if(pci_read(dev, PCI_VENDOR_ID) == PCI_NONE) {
            break;
        }
        tmp = pci_scan_bus(vendor_id, dev_id, func, dev_type);
        if(tmp.bits) {
            return tmp;
        }
    }
    return dev_null;
}