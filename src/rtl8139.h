#ifndef     RTL8139_H
#define     RTL8139_H

#include    "common.h"
#include    "pci.h"
#include    "memory.h"

#define     RX_BUF_SIZE             8192
#define     RTL8139_VENDOR_ID       0x10EC
#define     RTL8139_DEVICE_ID       0x8029      //8139
#define     CAPR                    0x38

#define     RX_READ_POINTER_MASK    (~3)
#define     TOK                     (1<<2)
#define     ROK                     (1<<0)
#define     TER                     (1<<3)
#define     RER                     (1<<1)
#define     TX_OK                   (1<<15)

enum RTL8139_registers {
    
  MAG0             = 0x00,       // Ethernet hardware address
  MAR0             = 0x08,       // Multicast filter
  TxStatus0        = 0x10,       // Transmit status (Four 32bit registers)
  TxAddr0          = 0x20,       // Tx descriptors (also four 32bit)
  RxBuf            = 0x30,
  RxEarlyCnt       = 0x34,
  RxEarlyStatus    = 0x36,
  ChipCmd          = 0x37,
  RxBufPtr         = 0x38,
  RxBufAddr        = 0x3A,
  IntrMask         = 0x3C,
  IntrStatus       = 0x3E,
  TxConfig         = 0x40,
  RxConfig         = 0x44,
  Timer            = 0x48,        // A general-purpose counter
  RxMissed         = 0x4C,        // 24 bits valid, write clears
  Cfg9346          = 0x50,
  Config0          = 0x51,
  Config1          = 0x52,
  FlashReg         = 0x54,
  GPPinData        = 0x58,
  GPPinDir         = 0x59,
  MII_SMI          = 0x5A,
  HltClk           = 0x5B,
  MultiIntr        = 0x5C,
  TxSummary        = 0x60,
  MII_BMCR         = 0x62,
  MII_BMSR         = 0x64,
  NWayAdvert       = 0x66,
  NWayLPAR         = 0x68,
  NWayExpansion    = 0x6A,
  // Undocumented registers, but required for proper operation
  FIFOTMS          = 0x70,        // FIFO Control and test
  CSCR             = 0x74,        // Chip Status and Configuration Register
  PARA78           = 0x78,
  PARA7c           = 0x7c,        // Magic transceiver parameter register
};

struct rtl8139_dev {

    u8int       bar_type;
    u16int      io_base;
    u32int      mem_base;
    u32int      eeprom_exist;
    u8int       mac_addr[6];
    char        *rx_buffer;
    u32int      tx_cur;
};

struct tx_desc {

    u32int      phys_addr;
    u32int      packet_size;
};

typedef struct rtl8139_dev rtl8139_dev_s;
typedef struct tx_desc tx_desc_s;

void rtl8139_handler(registers_t *regs);
void rtl8139_init(void);
void read_mac_addr(void);
void get_mac_addr(u32int *src_addr);
void rtl8139_send_packet(void *data, u32int len);
void rtl8139_recv_packet(void);

#endif