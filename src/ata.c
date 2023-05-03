#include    "ata.h"

ata_s   first_master =  {.slave = 0};
ata_s   first_slave  =  {.slave = 1};
ata_s   second_master = {.slave = 0};
ata_s   second_slave  = {.slave = 1};

pci_dev_s   ata_dev;

void ata_handler(registers_t regs) {

    inb(first_master.status);
    inb(first_master.bm_status);
    outb(first_master.bm_command, BM_COMMAND_DMA_STOP);

    return;
}

void ata_wait(ata_s *dev) {

    inb(dev->alt_status);
    inb(dev->alt_status);
    inb(dev->alt_status);
    inb(dev->alt_status);

    return;
}

void ata_reset(ata_s *dev) {

    outb(dev->control, CONTROL_SOFTWARE_RESET);
    ata_wait(dev);
    outb(dev->control, CONTROL_ZERO);

    return;
}

void ata_init(void) {
    //Find pci device
    ata_dev = pci_get_device(ATA_VENDOR_ID, ATA_DEVICE_ID, -1);
    if(!ata_dev.bits) {
        monitor_str_write("!ATA NOT FOUNDED!\n");
    }
    //Install irq handler
    register_interrupt_handler(IRQ14, &ata_handler);

    ata_device_detect(&first_master, 1);
    //ata_device_detect(&first_slave, 1);
    //ata_device_detect(&second_master, 0);
    //ata_device_detect(&second_slave, 0);

    return;
}

void ata_device_detect (ata_s *dev, u32int channel) {

    ata_device_init(dev, channel);
    ata_reset(dev);
    ata_wait(dev);
// Select drive, send 0xA0 to master device, 0xB0 to slave device
    outb(dev->drive, (0x0A + dev->slave) << 4);
    outb(dev->sector_cnt, 0);
    outb(dev->lba_lo, 0);
    outb(dev->lba_mid, 0);
    outb(dev->lba_high, 0);
// Send identify command to command port
    outb(dev->command, COMMAND_IDENTIFY);

    if(!inb(dev->status)) {
        monitor_str_write("ata device isnt exist(1) :(\n");
        return;
    }

    u8int lba_lo = inb(dev->lba_lo);
    u8int lba_high = inb(dev->lba_high);
    
    if(lba_lo != 0 || lba_high != 0) {
        monitor_str_write("ata device isnt exist(2) );\n");
        return;
    }
    
    u8int err = 0;
    u8int drq = 0;
    u8int sts = 0;

    while(!err && !drq) {

        sts = inb(dev->status);
        err = sts & STATUS_ERR;
        drq = sts & STATUS_DRQ;
    }
    if(err) {
        monitor_str_write("ata ERROR polling\n");
        return;
    }

    u16int *buff_tt = kmalloc(512);
    u16int *buff_resv = buff_tt;
    for(int i = 0; i < 256; i++) {
        *buff_tt = inw(dev->data);
        buff_tt++;
    }
    kfree(buff_resv);

    u32int pci_comm_reg = pci_read(ata_dev, PCI_COMMAND);

    if(!(pci_comm_reg & (1 << 2))) {
        pci_comm_reg |= (1 << 2);
        pci_write(ata_dev, PCI_COMMAND, pci_comm_reg);
    }

    vfs_node_s *node = ata_create_dev(dev);
    vfs_mount(dev->mountpoint, node);
    monitor_str_write("ATA OK\n");

    return;
}

void ata_device_init(ata_s *dev, u32int channel) {

    //dev->prdt = (void *)kmalloc_align(sizeof(prdt_s));
    dev->prdt = (void *)kmalloc_aphys(sizeof(prdt_s), (physaddr *)&dev->prdt_phys);
    memset(dev->prdt, 0, sizeof(prdt_s));
    //dev->prdt_phys = (u32int)virt2phys(kern_page_dir, dev->prdt);
    //dev->mem_buff = (void *)kmalloc_align(4096);
    dev->mem_buff = (void *)kmalloc_aphys(4096, (physaddr *)&dev->mem_buff_phys);
    memset(dev->mem_buff, 0, 4096);

    dev->prdt[0].buff_phys = (u32int)dev->mem_buff_phys;      //(u32int)virt2phys(kern_page_dir, dev->mem_buff);
    dev->prdt[0].size = ATA_SECTOR_SIZE;
    dev->prdt[0].end = MARK_END;

    u16int baddr =  channel ? (0x01F0) : (0x0170);
    u16int a_stat = channel ? (0x03F6) : (0x0376);

    dev->data = baddr;
    dev->error = baddr + 1;
    dev->sector_cnt = baddr + 2;
    dev->lba_lo = baddr + 3;
    dev->lba_mid = baddr + 4;
    dev->lba_high = baddr + 5;
    dev->drive = baddr + 6;
    dev->command = baddr + 7;
    dev->alt_status = a_stat;

    dev->bar4 = pci_read(ata_dev, PCI_BAR4);
    if(dev->bar4 & 0x01) {
        dev->bar4 &= 0xFFFFFFFC;
    }

    dev->bm_command = dev->bar4;
    dev->bm_status = dev->bar4 + 2;
    dev->bm_prdt = dev->bar4 + 4;

    memset(dev->mountpoint, 0, 32);
    strcpy(dev->mountpoint, "/dev/hd"); 
    // Primary master(hda, 00), primary slave(hdb, 01), secondary master(hdc, 10), secondary slave(hdd, 11)
    dev->mountpoint[strlen(dev->mountpoint)] = 'a' + (((!channel) << 1) | dev->slave);

    return;
}

void ata_sector_write(ata_s *dev, u32int lba, char *buf) {

    u8int dma_stat;
    u8int stat;

    dev->prdt[0].buff_phys = (u32int)dev->mem_buff_phys;
    dev->prdt[0].size = ATA_SECTOR_SIZE;
    dev->prdt[0].end = MARK_END;

    memcpy(dev->mem_buff, buf, ATA_SECTOR_SIZE);
    
    outb(dev->drive, 0xE0 | dev->slave << 4 | (lba & 0x0F000000) >> 24);
     while(1) {                          // Wait BSY=0 and DRDY=1
    
        u8int tp = inb(dev->status);
        if((!(tp & 0x80)) && (tp & 0x40)) {      
            break;
        }
    }

    outb(dev->sector_cnt, 1);
    outb(dev->lba_lo, lba & 0x000000FF);
    outb(dev->lba_mid, (lba & 0x0000FF00) >> 8);
    outb(dev->lba_high, (lba & 0x00FF0000) >> 16);

    //outb(dev->bm_command, 0);
    outl(dev->bm_prdt, (u32int)dev->prdt_phys);
    outb(dev->bm_command, 0);                   //from/out memory

    outb(dev->command, COMMAND_DMA_WRITE);              // Write the WRITE_DMA to the command register (0xCA)
    outb(dev->bm_command, BM_COMMAND_DMA_START);       // Start DMA Writing

    while(1) {
        dma_stat = inb(dev->bm_status);
        stat = inb(dev->status);

        if((!(dma_stat & 0x04)) && (dma_stat & 0x01)) {
            continue;
        }
        if(!(stat & 0x80)) {
            break;
        }
    }
    outb(dev->bm_status, 0x04);
    outb(dev->bm_command, 0x00);

    return;
}

u8int *ata_sector_read(ata_s *dev, u32int lba) {

    u8int dma_stat;
    u8int stat;
    u8int *buf = kmalloc(ATA_SECTOR_SIZE);

    dev->prdt[0].buff_phys = (u32int)dev->mem_buff_phys;
    dev->prdt[0].size = ATA_SECTOR_SIZE;
    dev->prdt[0].end = MARK_END;
/*
    while(1) {
        if(!(inb(dev->status) & 0x80)) {
            break;
        }
    }
*/
    outb(dev->drive, 0xE0 | dev->slave << 4 | (lba & 0x0F000000) >> 24);
    while(1) {                          // Wait BSY=0 and DRDY=1
    
        u8int tp = inb(dev->status);
        if((!(tp & 0x80)) && (tp & 0x40)) {      
            break;
        }
    }

    outb(dev->sector_cnt, 1);
    outb(dev->lba_lo, lba & 0x000000FF);
    outb(dev->lba_mid, (lba & 0x0000FF00) >> 8);
    outb(dev->lba_high, (lba & 0x00FF0000) >> 16);

    //outb(dev->bm_command, 0);
    outl(dev->bm_prdt, (u32int)dev->prdt_phys);     
    outb(dev->bm_command, 0x08);

    outb(dev->command, COMMAND_DMA_READ);                        // Write the READ_DMA to the command register (0xC8)
    outb(dev->bm_command, BM_COMMAND_READ | BM_COMMAND_DMA_START);  // Start DMA reading

    while(1) {
        dma_stat = inb(dev->bm_status);
        stat = inb(dev->status);

        if((!(dma_stat & 0x04)) && (dma_stat & 0x01)) {
            continue;
        }
        if(!(stat & 0x80)) {
            break;
        }
    }
    outb(dev->bm_status, 0x04);
    outb(dev->bm_command, 0x00);
    memcpy(buf, dev->mem_buff, ATA_SECTOR_SIZE);

/*
    u32int *m_b = (u32int *)dev->mem_buff;
    monitor_str_write("bm_command:");
    write_hex(dev->bm_command);
    monitor_str_write(" | first u32int: ");
    write_dec(m_b[0]);
    monitor_str_write(" | prdt_phys:");
    write_hex(dev->prdt_phys);
    monitor_str_write(" | bff_phs:");
    write_hex(dev->prdt[0].buff_phys);
    monitor_char_put('\n');
*/
    return buf;
}

u32int ata_write(vfs_node_s *node, u32int offset, u32int size, u8int *buff) {

    u8int *curr_buf = buff;

    u32int start_sect = offset / ATA_SECTOR_SIZE;
    u32int start_offset = offset % ATA_SECTOR_SIZE;
    u32int end_sect = (offset + size - 1) / ATA_SECTOR_SIZE;
    u32int end_offset = (offset + size - 1) % ATA_SECTOR_SIZE;

    u32int total = 0;
    u32int cnter = start_sect;
    u32int size_write;
    u32int off;
    u8int  *ret;

    while(cnter <= end_sect) {

        off = 0;
        size_write = ATA_SECTOR_SIZE;
        ret = ata_sector_read((ata_s *)node->device, cnter);

        if(cnter == start_sect) {
            off = start_offset;
            size_write = ATA_SECTOR_SIZE - off;
        }
        if(cnter == end_sect) {
            size_write = end_offset - off + 1;
        }
        memcpy(ret + off, curr_buf, size_write);
        ata_sector_write((ata_s *)node->device, cnter, ret);
        curr_buf += size_write;
        total += size_write;
        cnter++;

        kfree(ret);
    }
    return total;
}

u32int ata_read(vfs_node_s *node, u32int offset, u32int size, u8int *buff) {

    u8int *curr_buf = buff;

    u32int start_sect = offset / ATA_SECTOR_SIZE;
    u32int start_offset = offset % ATA_SECTOR_SIZE;
    u32int end_sect = (offset + size - 1) / ATA_SECTOR_SIZE;
    u32int end_offset = (offset + size - 1) % ATA_SECTOR_SIZE;

    u32int total = 0;
    u32int cnter = start_sect;
    u32int size_read;
    u32int off;
    u8int  *ret;

    while(cnter <= end_sect) {

        off = 0;
        size_read = ATA_SECTOR_SIZE;

        ret = ata_sector_read((ata_s *)node->device, cnter);

        if(cnter == start_sect) {
            off = start_offset;
            size_read = ATA_SECTOR_SIZE - off;
        }
        if(cnter == end_sect) {
            size_read = end_offset - off + 1;
        }
        memcpy(curr_buf, ret + off, size_read);
        curr_buf += size_read;
        total += size_read;
        cnter++;

        kfree(ret);
    }
    return total;
}

void ata_open(vfs_node_s *node, u32int flags) {
    return;
}

void ata_close(vfs_node_s *node) {

    kfree(node);
    return;
}

vfs_node_s *ata_create_dev(ata_s *dev) {

    vfs_node_s *f = kmalloc(sizeof(vfs_node_s));
    strcpy(f->name, "ata device ");

    f->name[strlen(f->name)] = dev->mountpoint[strlen(dev->mountpoint) - 1];
    f->device = dev;
    f->flags = FS_BLOCKDEV;
    f->read = ata_read;
    f->write = ata_write;
    f->open = ata_open;
    f->close = ata_close;

    return f;
}

void ata_test(ata_s *dev) {

    ident_dev_data *ata = kmalloc(sizeof(ident_dev_data));
    ident_dev_data *buff = kmalloc(sizeof(ident_dev_data));
    buff->num_cylindres = 89;
    buff->num_heads = 28;
    buff->num_sect_track = 1568;

    vfs_node_s *node = ata_create_dev(dev);
    ata_write(node, 512, sizeof(ident_dev_data), (u8int *)buff);
    ata_read(node, 512, sizeof(ident_dev_data), (u8int *)ata);

    monitor_str_write(node->name);
    monitor_char_put('\n');
    monitor_str_write(dev->mountpoint);
    monitor_char_put('\n');

    write_dec(dev->mountpoint[strlen(dev->mountpoint) - 1]);
    monitor_char_put('\n');

    u16int tmp = (u16int)ata->num_heads;
    monitor_str_write("headers: ");
    write_dec((u32int)tmp);
    tmp = (u16int)ata->num_cylindres;
    monitor_str_write("\ncylinders: ");
    write_dec((u32int)tmp);
    tmp = (u16int)ata->num_sect_track;
    monitor_str_write("\nsectors per track: ");
    write_dec((u32int)tmp);
    monitor_str_write("\n");

    kfree(ata);
    kfree(buff);

    return;
}

void ata_test2(ata_s *dev) {

    ext2_supblock_s *sb = kcalloc(sizeof(ext2_supblock_s), 1);
    vfs_node_s *node = ata_create_dev(dev);
    ata_read(node, 1024, sizeof(ext2_supblock_s), (u8int *)sb);

    monitor_clear();
    monitor_str_write("total_inodes: ");
    write_dec(sb->total_inodes);
    monitor_str_write(" | total_blocks: ");
    write_dec(sb->total_blocks);
    monitor_str_write("| ext2_magic: ");
    write_hex(sb->ext2_magic);
    monitor_str_write(" | block_size: ");
    write_dec(1024 << sb->log2block_size);
    monitor_char_put('\n');

    monitor_str_write("inodes_per_group:");
    write_dec(sb->inodes_per_group);
    monitor_str_write(" | blocks_per_group: ");
    write_dec(sb->blocks_per_group);
    monitor_str_write(" | supblock_idx: ");
    write_dec(sb->supblock_idx);
    monitor_char_put('\n');

    monitor_str_write("free_inodes: ");
    write_dec(sb->free_inodes);
    monitor_str_write(" | free_blocks: ");
    write_dec(sb->free_blocks);
    monitor_str_write(" | fs_state: ");
    write_dec(sb->fs_state);
    monitor_str_write(" | frag_size: ");
    write_dec(sb->log2frag_size);
    monitor_char_put('\n');

    monitor_str_write("mtime: ");
    write_dec(sb->mtime);
    monitor_str_write(" | wtime: ");
    write_dec(sb->wtime);
    monitor_char_put('\n');

    kfree(sb);
    return;
}
