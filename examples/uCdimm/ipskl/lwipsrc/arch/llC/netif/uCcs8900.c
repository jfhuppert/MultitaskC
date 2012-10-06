/*-----------------------------------------------------------------------------------*/
/*
	uCcs8900.c
*/
/*-----------------------------------------------------------------------------------*/

#include <netif/cs8900.h>
#include <netif/iofuncs.h>
#include <netif/bootstd.h>
#include <netif/io.h>

#define sti() __asm__ __volatile__ ("andiw #0xf8ff,%/sr": : : "memory")
#define cli() __asm__ __volatile__ ("oriw  #0x0700,%/sr": : : "memory")

extern void *_ramvec[];
extern void int_handler(void);
extern void int_handler5(void);

extern void *memcpy(void *dest, const void *src, int n);

_bsc1(unsigned char *, gethwaddr, int, a)

#define soutw(a,b)      outw(a,b)
#define soutl(a,b)      outl(a,b)
#define sinw(a)         inw(a)
#define sinl(a)         inl(a)

#define soutsw(a,b,l)       outsw(a,b,l)
#define sinsw(a,b,l)        insw(a,b,l)

unsigned long cs8900_base_addr = 0x10000300;

/* Information that need to be kept for each board. */
int chip_type;    /* one of: CS8900, CS8920, CS8920M */
char chip_revision;      /* revision letter of the chip ('A'...) */
int send_cmd;      /* the propercommand used to send a packet. */
int auto_neg_cnf;
int adapter_cnf;
int isa_config;
int irq_map;
int rx_mode;
int curr_rx_cfg;
int linectl;
int send_underrun;        /* keep track of how many underruns in a row we get */
int rev_type;
unsigned char dev_addr[6];

#include "lwip/debug.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/inet.h"

#include "netif/arp.h"

#define IFNAME0 'c'
#define IFNAME1 's'

static const struct eth_addr ethbroadcast = {{0xffff,0xffff,0xffff}};

void  uCcs8900_input(struct netif *netif);
err_t uCcs8900_output(struct netif *netif,struct pbuf *p,struct ip_addr *ipaddr);

/*------------------------------------------------------------------------*/
int inline readreg(int portno)
{
	soutw(portno,cs8900_base_addr+ADD_PORT);
	return(sinw(cs8900_base_addr+DATA_PORT));
}
/*------------------------------------------------------------------------*/
void inline writereg(int portno,int value)
{
	soutw(portno,cs8900_base_addr+ADD_PORT);
	soutw(value,cs8900_base_addr+DATA_PORT);
}
/*------------------------------------------------------------------------*/
int inline readword(int portno)
{
	return(sinw(cs8900_base_addr+portno));
}
/*------------------------------------------------------------------------*/
void inline writeword(int portno,int value)
{
	soutw(value,cs8900_base_addr+portno);
}
/*------------------------------------------------------------------------*/
void reset_chip(void)
{
volatile int
	reset_start_time;

	putstr("cs8900: reseting ...");
	writereg(PP_SelfCTL,readreg(PP_SelfCTL)|POWER_ON_RESET);
	reset_start_time=0x10000;
	while((readreg(PP_SelfST)&INIT_DONE)==0 && reset_start_time-->0);
	if(reset_start_time>0) putstr(" done");
	else putstr(" failed");
	putstr("\r\n");
}
/*------------------------------------------------------------------------*/
int cs8900_probe(void)
{
	*(volatile unsigned  char *)0xfffff42b |= 0x01; /* output /sleep */
	*(volatile unsigned short *)0xfffff428 |= 0x0101; /* not sleeping */
	*(volatile unsigned  char *)0xfffff42b &= ~0x02; /* input irq5 */
	*(volatile unsigned short *)0xfffff428 &= ~0x0202; /* irq5 fcn on */
	*(volatile unsigned short *)0xfffff102 = 0x8000; /* 0x04000000 */
	*(volatile unsigned short *)0xfffff112 = 0x01e1; /* 128k, 2ws, FLASH, en */

	if(readreg(PP_ChipID)!=CHIP_EISA_ID_SIG) {
		putstr("cs8900: No CrystalLan device found.\r\n");
		return(-1);
	}

	/* get the chip type */
	rev_type=readreg(PRODUCT_ID_ADD);
	chip_type=rev_type&~REVISON_BITS;
	chip_revision=((rev_type&REVISON_BITS)>>8)+'A';
	/* Check the chip type and revision in order to set the correct send command
	CS8920 revision C and CS8900 revision F can use the faster send. */
	send_cmd=TX_AFTER_ALL;

	putstr("cs89");
	putstr(chip_type==CS8900?"0":"2");
	putstr("0");
	putstr(chip_type==CS8920M?"M":"");
	putstr(": rev ");
	putchr(chip_revision);
	putstr(" found at 0x");
	puthex(cs8900_base_addr);
	putstr(" ");
	putstr(readreg(PP_SelfST)&ACTIVE_33V?"3.3Volts":"5Volts");

	/* Fill this in, we don't have an EEPROM */
	adapter_cnf=A_CNF_10B_T | A_CNF_MEDIA_10B_T;
	auto_neg_cnf=EE_AUTO_NEG_ENABLE;

	putstr(" media ");
	putstr(adapter_cnf&A_CNF_10B_T?"RJ-45,":"");
	putstr(adapter_cnf&A_CNF_AUI?"AUI,":"");
	putstr(adapter_cnf&A_CNF_10B_2?"BNC,":"");

	putstr("\r\n");

	memcpy(dev_addr,gethwaddr(0),6);

	reset_chip();

	return(0);
}
/*------------------------------------------------------------------------*/
static int detect_tp(void)
{
volatile int
	i,
	timenow;

	putstr("cs8900: attempting TP\r\n");

	for(i=0;i<10;i++) {
		writereg(PP_LineCTL,linectl&~AUI_ONLY);
		for(timenow=0x10000;timenow>0;timenow--);
		if((readreg(PP_LineST)&LINK_OK)!=0) break;
	}

	if(i<10) return A_CNF_MEDIA_10B_T;
	else return 0;
}
/*------------------------------------------------------------------------*/
static void set_mac_address(void *addr)
{
	int i;
	putstr("cs8900: setting MAC address to ");
	for(i=0;i<5;i++) { puthex8(((unsigned char *)addr)[i]); putchr(':'); }
	puthex8(((unsigned char *)addr)[i]);
	putstr("\r\n");

	/* set the Ethernet address */
	for(i=0;i<3;i++) writereg(PP_IA+i*2,((unsigned char *)addr)[i*2]|(((unsigned char *)addr)[i*2+1]<<8));
}
/*------------------------------------------------------------------------*/
int cs8900_open(void)
{
int
	result=0;

	writereg(PP_CS8900_ISAINT,0);
	writereg(PP_BusCTL,0); /* ints off! */

	/*#######################*/

	_ramvec[64]=int_handler;
	_ramvec[65]=int_handler;
	_ramvec[66]=int_handler;
	_ramvec[67]=int_handler;
	_ramvec[68]=int_handler;
	_ramvec[69]=int_handler5;
	_ramvec[70]=int_handler;
	_ramvec[71]=int_handler;

	*(volatile unsigned char *)0xfffff300=0x40; /* Set DragonBall IVR (interrupt base) to 64 */
	*(volatile unsigned short *)0xfffff302|=0x0080; /* +ve pol irq */
	*(volatile unsigned long *)0xfffff304=~0;

	/*#######################*/

	/* set the Ethernet address */
	set_mac_address(dev_addr);

	/* Set the LineCTL */
	linectl=0;

	/* check to make sure that they have the "right" hardware available */
	switch(adapter_cnf&A_CNF_MEDIA_TYPE) {
	case A_CNF_MEDIA_10B_T:
		result=adapter_cnf&A_CNF_10B_T;
		break;
	case A_CNF_MEDIA_AUI:
		result=adapter_cnf&A_CNF_AUI;
		break;
	case A_CNF_MEDIA_10B_2:
		result=adapter_cnf&A_CNF_10B_2;
		break;
	default:
		result=adapter_cnf&(A_CNF_10B_T|A_CNF_AUI|A_CNF_10B_2);
		break;
	}

	if (!result) {
		putstr("EEPROM is configured for unavailable media\r\n");
	release_irq:
		writereg(PP_LineCTL,readreg(PP_LineCTL) & ~(SERIAL_TX_ON|SERIAL_RX_ON));
		return(-1);
	}

	/* set the hardware to the configured choice */
	switch(adapter_cnf&A_CNF_MEDIA_TYPE) {
	case A_CNF_MEDIA_10B_T:
		result=detect_tp();
		if(!result) putstr("cs8900: 10Base-T (RJ-45) has no cable\r\n");
		if(auto_neg_cnf&IMM_BIT) result=A_CNF_MEDIA_10B_T; /* Yes! I don't care if I see a link pulse */
		break;
	case A_CNF_MEDIA_AUI:
		putstr("cs8900: AUI media detected\r\n");
		break;
	case A_CNF_MEDIA_10B_2:
		putstr("cs8900: 10Base-2 (Coaxel) detected\r\n");
		break;
	case A_CNF_MEDIA_AUTO:
		writereg(PP_LineCTL,linectl|AUTO_AUI_10BASET);
		if(adapter_cnf&A_CNF_10B_T)
			if((result=detect_tp())!=0)
				break;

		putstr("cs8900: no media detected\r\n");
		goto release_irq;
	}

	switch(result) {
	case 0: putstr("cs8900: no network cable attached to configured media\r\n"); goto release_irq;
	case A_CNF_MEDIA_10B_T: putstr("cs8900: using 10Base-T (RJ-45)\r\n"); break;
	case A_CNF_MEDIA_AUI:   putstr("cs8900: using 10Base-5 (AUI)\r\n"); break;
	case A_CNF_MEDIA_10B_2: putstr("cs8900: using 10Base-2 (BNC)\r\n"); break;
	default: print("cs8900: unexpected result was ",result); goto release_irq;
	}

	/* Turn on both receive and transmit operations */
	writereg(PP_LineCTL,readreg(PP_LineCTL)|SERIAL_RX_ON|SERIAL_TX_ON);

	/* Receive only error free packets addressed to this card */
	rx_mode=0;
	writereg(PP_RxCTL,DEF_RX_ACCEPT);

	curr_rx_cfg=RX_OK_ENBL|RX_CRC_ERROR_ENBL;
	writereg(PP_RxCFG,curr_rx_cfg);

	writereg(PP_TxCFG,TX_LOST_CRS_ENBL|TX_SQE_ERROR_ENBL|TX_OK_ENBL|TX_LATE_COL_ENBL|TX_JBR_ENBL|TX_ANY_COL_ENBL|TX_16_COL_ENBL);
	writereg(PP_BufCFG,READY_FOR_TX_ENBL|RX_MISS_COUNT_OVRFLOW_ENBL|TX_COL_COUNT_OVRFLOW_ENBL|TX_UNDERRUN_ENBL);

	return 0;
}
/*------------------------------------------------------------------------*/
unsigned char *get_dev_addr(void)
{
	return(dev_addr);
}
/*-----------------------------------------------------------------------------------*/
static void low_level_init(struct netif *netif)
{
  cs8900_probe();
  cs8900_open();

  // putstr("before memcpy()\r\n");
  memcpy(netif->hwaddr,dev_addr,6);
  // putstr("after memcpy()\r\n");
}
/*-----------------------------------------------------------------------------------*/
static err_t low_level_output(struct pbuf *p)
{
volatile long
  timenow;

unsigned long
  n;

unsigned char
  *ptr;

struct pbuf
  *q;

u16_t
  tmp0;

  soutw(TX_AFTER_ALL,cs8900_base_addr+TX_CMD_PORT);
  tmp0=p->tot_len;
  soutw(tmp0,cs8900_base_addr+TX_LEN_PORT);

  /* Test to see if the chip has allocated memory for the packet */
  timenow=0x8000;
  while(timenow-->0) if(readreg(PP_BusST)&READY_FOR_TX_NOW) break;
  if(timenow<=0) { 
    putstr("no mem for tx\r\n");
    return(ERR_MEM); /* this shouldn't happen */
  }

  dbg_putstr("low_level_output 0\r\n");
  for(q=p;q!=NULL;q=q->next) {
    ptr=q->payload;
    n=(q->len+1)>>1;
    while(n>0) {
      tmp0=(u16_t)(((*ptr)&0xff)<<8)|((u16_t)(*(ptr+1)&0xff));
      *(volatile unsigned short *)(cs8900_base_addr+TX_FRAME_PORT)=tmp0;
      ptr+=2;
      n--;
    }
  }
  
  dbg_putstr("low_level_output 1\r\n");
  timenow=0x8000;
  while(timenow-->0) if(((readreg(PP_TxEvent))&TX_SEND_OK_BITS)==TX_OK) break;
  if(timenow<=0) { 
    putstr("tx timeout\r\n");
    return(ERR_MEM); /* this shouldn't happen */
  }

  dbg_putstr("low_level_output 2\r\n");
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static struct pbuf *low_level_input(void)
{
struct pbuf
  *p,
  *q;

unsigned short
  event,
  len;

unsigned long
  n;

unsigned char
  *ptr;

u16_t
  tmp0;

  event=readreg(PP_RxEvent);
  if((event&(RX_OK|RX_BROADCAST))==0) return(NULL);

  dbg_putstr("low_level_input 0\r\n");
  event=sinw(cs8900_base_addr+RX_FRAME_PORT);
  len=sinw(cs8900_base_addr+RX_FRAME_PORT);

#if 0
  if(event!=0x104 && event!=0x504) {
	putstr("rx_errors\r\n");
	if(event&RX_RUNT) putstr("rx_runt_errors\r\n");
	if(event&RX_EXTRA_DATA) putstr("rx_length_errors\r\n");
	if(event&RX_CRC_ERROR) putstr("rx_crc_errors\r\n");
	if(event&RX_DRIBBLE) putstr("rx_dribble_errors\r\n");
	return(0);
  }
#endif

  p=pbuf_alloc(PBUF_LINK,len,PBUF_POOL);

  if(p!=NULL) {
    for(q=p;q!=NULL;q=q->next) {
      ptr=q->payload;
      n=(q->len)>>1;
      while(n>0) {
        tmp0=*(volatile unsigned short *)(cs8900_base_addr+RX_FRAME_PORT);
        *ptr++=(unsigned char)((tmp0&0xff00)>>8);
        *ptr++=(unsigned char)(tmp0&0x00ff);
        n--;
      }
      if(q->len&1) {
        tmp0=*(volatile unsigned short *)(cs8900_base_addr+RX_FRAME_PORT);
        *ptr++=(unsigned char)((tmp0&0xff00)>>8);
      }
    }
  }
  else {
    writereg(PP_RxCTL,readreg(PP_RxCTL)|0x40);
  }

  return p;  
}
/*-----------------------------------------------------------------------------------*/
err_t uCcs8900_output(struct netif *netif,struct pbuf *p,struct ip_addr *ipaddr)
{
  struct pbuf *q;
  struct eth_hdr *ethhdr;
  struct eth_addr *dest, mcastaddr;
  struct ip_addr *queryaddr;
  err_t err;
  u8_t i;
  
  /* Make room for Ethernet header. */
  if(pbuf_header(p, 14) != 0) {
    /* The pbuf_header() call shouldn't fail, but we allocate an extra pbuf just in case. */
    q=pbuf_alloc(PBUF_LINK, 14, PBUF_RAM);
    if(q==NULL) { return ERR_MEM; }
    pbuf_chain(q,p);
    p=q;
  }

  /* Construct Ethernet header. Start with looking up deciding which
     MAC address to use as a destination address. Broadcasts and
     multicasts are special, all other addresses are looked up in the
     ARP table. */
  queryaddr = ipaddr;
  if(ip_addr_isany(ipaddr) ||
     ip_addr_isbroadcast(ipaddr, &(netif->netmask))) {
    dest = (struct eth_addr *)&ethbroadcast;
  } else if(ip_addr_ismulticast(ipaddr)) {
    /* Hash IP multicast address to MAC address. */
    mcastaddr.addr[0] = HTONS(0x01 << 8);
    mcastaddr.addr[1] = HTONS((0x5e << 8) | (ip4_addr2(ipaddr) & 0x7f));
    mcastaddr.addr[2] = HTONS((ip4_addr3(ipaddr) << 8) | ip4_addr4(ipaddr));
    dest = &mcastaddr;
  } else {
    if(ip_addr_maskcmp(ipaddr, &(netif->ip_addr), &(netif->netmask))) {
      /* Use destination IP address if the destination is on the same subnet as we are. */
      queryaddr = ipaddr;
    } else {
      /* Otherwise we use the default router as the address to send the Ethernet frame to. */
      queryaddr = &(netif->gw);
    }
    dest = arp_lookup(queryaddr);
  }

  /* If the arp_lookup() didn't find an address, we send out an ARP query for the IP address. */
  if(dest==NULL) {
    q=arp_query(netif,(struct eth_addr *)dev_addr,queryaddr);
    if(q!=NULL) {
      err=low_level_output(q);
      pbuf_free(q);
      return err;
    }
    return ERR_MEM;
  }

  ethhdr=p->payload;

  for(i=0;i<3;i++) {
    ethhdr->dest.addr[i]=dest->addr[i];
    ethhdr->src.addr[i]=(dev_addr[i*2]|(dev_addr[(i*2)+1]<<8));
  }
  
  ethhdr->type=htons(ETHTYPE_IP);
  
  return low_level_output(p);
}
/*-----------------------------------------------------------------------------------*/
void uCcs8900_input(struct netif *netif)
{
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  p=low_level_input();

  if(p!=NULL) {
    ethhdr=p->payload;
    switch(htons(ethhdr->type)) {
    case ETHTYPE_IP:
      arp_ip_input(netif,p);
      pbuf_header(p,-14);
      dbg_putstr("calling ip_input()\r\n");
      netif->input(p,netif);
      break;
    case ETHTYPE_ARP:
      p=arp_arp_input(netif,(struct eth_addr *)dev_addr,p);
      if(p!=NULL) {
	low_level_output(p);
	pbuf_free(p);
      }
      break;
    default:
      pbuf_free(p);
      break;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
void uCcs8900_init(struct netif *netif)
{
  netif->state = NULL;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->output = uCcs8900_output;
  
  // putstr("before low_level_init()\r\n");
  low_level_init(netif);
  // putstr("after low_level_init()\r\n");
  arp_init();  
  // putstr("after arp_init()\r\n");
}
/*-----------------------------------------------------------------------------------*/