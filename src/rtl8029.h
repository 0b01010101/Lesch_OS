#ifndef     RTL8029_H
#define     RTL8029_H

#include    "common.h"
#include    "pci.h"
#include    "memory.h"
#include    "net.h"

#define     PKTSIZE_ALIGN       1536
#define     PKTSIZE             1518
#define     PKTALIGN            32

#define     RTL8029_VENDOR_ID   0x10EC
#define     RTL8029_DEVICE_ID   0x8029

#define     N2K_COMMAND			        0x00
#define		N2K_PAGESTART		        0x01
#define		N2K_PAGESTOP		        0x02
#define		N2K_BOUNDARY		        0x03
#define		N2K_TRANSMITSTATUS		    0x04
#define		N2K_TRANSMITPAGE		    0x04
#define		N2K_TRANSMITBYTECOUNT0	    0x05
#define		N2K_NCR 			        0x05
#define		N2K_TRANSMITBYTECOUNT1 	    0x06
#define		N2K_INTERRUPTSTATUS		    0x07
#define		N2K_CURRENT 		        0x07
#define		N2K_REMOTESTARTADDRESS0     0x08
#define		N2K_CRDMA0  		        0x08
#define		N2K_REMOTESTARTADDRESS1     0x09
#define		N2K_CRDMA1 			        0x09
#define		N2K_REMOTEBYTECOUNT0	    0x0A
#define		N2K_REMOTEBYTECOUNT1	    0x0B
#define		N2K_RECEIVESTATUS		    0x0C
#define		N2K_RECEIVECONFIGURATION    0x0C
#define		N2K_TRANSMITCONFIGURATION   0x0D
#define		N2K_FAE_TALLY 		        0x0D
#define		N2K_DATACONFIGURATION	    0x0E
#define		N2K_CRC_TALLY 		        0x0E
#define		N2K_INTERRUPTMASK		    0x0F
#define		N2K_MISS_PKT_TALLY		    0x0F
#define		N2K_PHYSICALADDRESS0	    0x01
#define 	N2K_PHYSICALADDRESS1	    0x02
#define		N2K_PHYSICALADDRESS2	    0x03
#define		N2K_PHYSICALADDRESS3	    0x04
#define		N2K_PHYSICALADDRESS4	    0x05
#define		N2K_PHYSICALADDRESS5	    0x06
#define		N2K_MULTIADDRESS0		    0x08
#define		N2K_MULTIADDRESS1		    0x09
#define		N2K_MULTIADDRESS2	        0x0A
#define		N2K_MULTIADDRESS3		    0x0B
#define		N2K_MULTIADDRESS4	        0x0C
#define		N2K_MULTIADDRESS5		    0x0D
#define		N2K_MULTIADDRESS6		    0x0E
#define		N2K_MULTIADDRESS7		    0x0F
#define		N2K_DMA_DATA		        0x10
#define		N2K_RESET			        0x1F

#define 	N2K_PAGE0               	0x22
#define   	N2K_PAGE1               	0x62
#define   	N2K_PAGE0DMAWRITE       	0x12
#define   	N2K_PAGE2DMAWRITE       	0x92
#define   	N2K_REMOTEDMAWR         	0x12
#define   	N2K_REMOTEDMARD         	0x0A
#define   	N2K_ABORTDMAWR          	0x32
#define   	N2K_ABORTDMARD          	0x2A
#define   	N2K_PAGE0STOP           	0x21
#define   	N2K_PAGE1STOP           	0x61
#define   	N2K_TRANSMIT            	0x26
#define   	N2K_TXINPROGRESS        	0x04
#define   	N2K_SEND		    	    0x1A

#define		N2K_PSTART			0x4c
#define		N2K_PSTOP			0x80
#define		N2K_TPSTART			0x40

struct net_dev {
    u8int   bar_type;
    u16int  io_base;
    u32int  mem_base;
    u32int  eeprom_exist;
    u8int   mac_addr[6];
    char    *rx_buffer;
    u32int  tx_cur;

    u32int  (*send) (void *pack, u32int length);
    u32int  (*recv) (void);
    void    (*halt) (void);
};

typedef struct net_dev net_dev_s;

void  rtl8029_init(void);
void  rtl8029_halt(void);
void  n2k_mac_addr(net_dev_s *dev);
void get_mac_addr(net_dev_s *dev, u8int *src);
u8int n2k_to_pc(void);
u32int rtl8029_recv(void);
u32int rtl8029_send(void *, u32int);

extern void Ethernet_Recv(ether_s *pack, u32int len);
extern u8int Ip_Send(u8int *dst_ip, u8int proto, void *data, u32int len);
extern void Arp_Send(u8int *dst_mac, u8int *dst_ip);

#endif