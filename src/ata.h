#ifndef     ATA_H
#define     ATA_H

#include    "common.h"
#include    "help_func.h"
#include    "monitor.h"
#include    "pci.h"
#include    "isr.h"
#include    "vfs.h"

#define     ATA_VENDOR_ID           0x8086
#define     ATA_DEVICE_ID           0x7010
#define     ATA_SECTOR_SIZE         512
#define     MARK_END                0x8000

#define     CONTROL_STOP_INT        0x02
#define     CONTROL_SOFTWARE_RESET  0x04
#define     CONTROL_HIGH_ORDER_BYTE 0x80
#define     CONTROL_ZERO            0x00

#define     COMMAND_IDENTIFY        0xEC
#define     COMMAND_DMA_READ        0xC8
#define     COMMAND_DMA_WRITE       0xCA
#define     COMMAND_READ_PIO        0x20

#define     BM_COMMAND_DMA_STOP     0x00
#define     BM_COMMAND_DMA_START    0x01
#define     BM_COMMAND_READ         0x08
#define     BM_COMMAND_WRITE        0x00
#define     BM_STATUS_INT           0x04
#define     BM_STATUS_ERR           0x02

#define     STATUS_ERR              0x00
#define     STATUS_DRQ              0x08
#define     STATUS_SRV              0x10
#define     STATUS_DF               0x20
#define     STATUS_RDY              0x40
#define     STATUS_BSY              0x80

typedef struct prdt prdt_s;
typedef struct ata ata_s;

struct prdt {

    u32int  buff_phys;
    u16int  size;
    u16int  end;
};

struct ata {

    u16int      data;
    u16int      error;
    u16int      sector_cnt;

    union   {
        u16int  sector_num;
        u16int  lba_lo;
    };
    union   {
        u16int  cylindr_low;
        u16int  lba_mid;
    };
    union   {
        u16int  cylindr_high;
        u16int  lba_high;
    };
    union   {
        u16int  drive;
        u16int  head;
    };
    union   {
        u16int  command;
        u16int  status;
    };
    union   {
        u16int  control;
        u16int  alt_status;
    };

    u32int      bar4;
    u32int      bm_command;
    u32int      bm_status;
    u32int      bm_prdt;

    prdt_s      *prdt;
    u8int       *prdt_phys;
    u8int       *mem_buff;
    u8int       *mem_buff_phys;

    u32int      slave;
    u8int       mountpoint[32];
};

typedef struct {

    u16int      res1            : 1;
    u16int      retired         : 1;
    u16int      response_incompl: 1;
    u16int      retired2        : 3;
    u16int      fix_dev         : 1;
    u16int      remov_media     : 1;
    u16int      retired1        : 7;
    u16int      dev_type        : 1;
} gen_conf;

typedef struct {

    gen_conf    general_config;

    u16int      num_cylindres;
    u16int      spec_conf;
    u16int      num_heads;
    u16int      retired1[2];
    u16int      num_sect_track;
    u16int      vendor_unique1[3];
    u8int       serial_number[20];
    u16int      retired2[2];
    u16int      obsolete1;
    u8int       firm_ware_rev[8];
    u8int       model_num[40];
    u8int       max_block_trans;
    u8int       vendor_unique2;

    u8int       data[];
} ident_dev_data;


void ata_init(void);
void ata_close(vfs_node_s *node);
void ata_open(vfs_node_s *node, u32int flags);
void ata_device_init(ata_s *dev, u32int channel);
void ata_device_detect(ata_s *dev, u32int channel);
void ata_wait(ata_s *dev);
void ata_reset(ata_s *dev);
void ata_handler(registers_t reg);
void ata_sector_write(ata_s *dev, u32int lba, char *buf);
u8int *ata_sector_read(ata_s *dev, u32int lba);
u32int ata_write(vfs_node_s *node, u32int offset, u32int size, u8int *buff);
u32int ata_read(vfs_node_s *node, u32int offset, u32int size, u8int *buff);
vfs_node_s *ata_create_dev(ata_s *dev);

#endif