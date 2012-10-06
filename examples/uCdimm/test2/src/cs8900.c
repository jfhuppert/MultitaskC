/*------------------------------------------------------------------------*/
/*
	Basic cs8900 driver.
*/
/*------------------------------------------------------------------------*/

#include <cs8900.h>
#include <iofuncs.h>
#include <bootstd.h>
#include <io.h>

#define sti() __asm__ __volatile__ ("andiw #0xf8ff,%/sr": : : "memory")
#define cli() __asm__ __volatile__ ("oriw  #0x0700,%/sr": : : "memory")

extern void *_ramvec[];
extern void int_handler(void);
extern void int_handler5(void);

_bsc1(unsigned char *, gethwaddr, int, a)

#define soutw(a,b)      outw(a,b)
#define soutl(a,b)      outl(a,b)
#define sinw(a)         inw(a)
#define sinl(a)         inl(a)

#define soutsw(a,b,l)       outsw(a,b,l)
#define sinsw(a,b,l)        insw(a,b,l)

unsigned long cs8900_base_addr = 0x10000300;

/* Information that need to be kept for each board. */
int chip_type;	  /* one of: CS8900, CS8920, CS8920M */
char chip_revision;	 /* revision letter of the chip ('A'...) */
int send_cmd;	   /* the propercommand used to send a packet. */
int auto_neg_cnf;
int adapter_cnf;
int isa_config;
int irq_map;
int rx_mode;
int curr_rx_cfg;
int linectl;
int send_underrun;	  /* keep track of how many underruns in a row we get */
int rev_type;
unsigned char dev_addr[6];
volatile unsigned long in_tx;

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
typedef unsigned short u16_t;
/*------------------------------------------------------------------------*/
int cs8900_probe(void)
{
int
	i;

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
	putstr(" rev ");
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

	putstr("sizeof(u16_t)==0x");
	puthex8(sizeof(u16_t));
	putstr("\r\n");

	return(0);
}
/*------------------------------------------------------------------------*/
static int detect_tp(void)
{
volatile int
	timenow;

	putstr("Attempting TP\r\n");
	writereg(PP_LineCTL,linectl&~AUI_ONLY);
	for(timenow=0x10000;timenow>0;timenow--);
	if((readreg(PP_LineST)&LINK_OK)==0) return 0;
	return A_CNF_MEDIA_10B_T;
}
/*------------------------------------------------------------------------*/
static void set_mac_address(void *addr)
{
	int i;
	putstr("Setting MAC address to ");
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

int
	i;

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
		if(!result) putstr("10Base-T (RJ-45) has no cable\r\n");
		if(auto_neg_cnf&IMM_BIT) result=A_CNF_MEDIA_10B_T; /* Yes! I don't care if I see a link pulse */
		break;
	case A_CNF_MEDIA_AUI:
		putstr("AUI media detected\r\n");
		break;
	case A_CNF_MEDIA_10B_2:
		putstr("10Base-2 (Coaxel) detected\r\n");
		break;
	case A_CNF_MEDIA_AUTO:
		writereg(PP_LineCTL,linectl|AUTO_AUI_10BASET);
		if(adapter_cnf&A_CNF_10B_T)
			if((result=detect_tp())!=0)
				break;

		putstr("no media detected\r\n");
		goto release_irq;
	}

	switch(result) {
	case 0: putstr("no network cable attached to configured media\r\n"); goto release_irq;
	case A_CNF_MEDIA_10B_T: putstr("using 10Base-T (RJ-45)\r\n"); break;
	case A_CNF_MEDIA_AUI:   putstr("using 10Base-5 (AUI)\r\n"); break;
	case A_CNF_MEDIA_10B_2: putstr("using 10Base-2 (BNC)\r\n"); break;
	default: print("unexpected result was ",result); goto release_irq;
	}

	/* Turn on both receive and transmit operations */
	writereg(PP_LineCTL,readreg(PP_LineCTL)|SERIAL_RX_ON|SERIAL_TX_ON);

	/* Receive only error free packets addressed to this card */
	rx_mode=0;
	writereg(PP_RxCTL,DEF_RX_ACCEPT);

	curr_rx_cfg=RX_OK_ENBL|RX_CRC_ERROR_ENBL;
	writereg(PP_RxCFG,curr_rx_cfg);

	writereg(PP_TxCFG,TX_LOST_CRS_ENBL|TX_SQE_ERROR_ENBL|TX_OK_ENBL|
		   TX_LATE_COL_ENBL|TX_JBR_ENBL|TX_ANY_COL_ENBL|TX_16_COL_ENBL);

	writereg(PP_BufCFG,READY_FOR_TX_ENBL|RX_MISS_COUNT_OVRFLOW_ENBL|
		 TX_COL_COUNT_OVRFLOW_ENBL|TX_UNDERRUN_ENBL);

	/* now that we've got our act together, enable everything */
	// writereg(PP_BusCTL,ENABLE_IRQ);

	in_tx=0;

	/*#######################*/

#if 0
	*(volatile unsigned long *)0xfffff304&=~(1<<20);
	sti();
#endif

	/*#######################*/

	return 0;
}
/*------------------------------------------------------------------------*/
unsigned char *get_dev_addr(void)
{
	return(dev_addr);
}
/*------------------------------------------------------------------------*/
void pkt_dump(unsigned char *pkt,unsigned short length)
{
	int i,print;

	if(length==0) return;

	putstr("Packet dest   addr is ");
	for(i=0;i<5;i++) { puthex8(((unsigned char *)pkt)[i]); putchr(':'); }
	puthex8(((unsigned char *)pkt)[i]);
	putstr("\r\n");
	putstr("Packet source addr is ");
	for(i=0;i<5;i++) { puthex8(((unsigned char *)pkt+6)[i]); putchr(':'); }
	puthex8(((unsigned char *)pkt+6)[i]);
	putstr("\r\n");

	print=0;
	for(i=0;i<(length-12);i++) {
		puthex8(((unsigned char *)pkt+12)[i]);
		print++;
		if((print&0x1f)==0) { putstr("\r\n"); print=0; }
	}

	if((print&0x1f)!=0) { putstr("\r\n"); }
}
/*------------------------------------------------------------------------*/
void int_handler_c_dummy(void)
{
	putstr("\r\ndummy int\r\n");
}
/*------------------------------------------------------------------------*/
void int_handler_c_5(void)
{
#if 0
unsigned short
	status;

unsigned short
	len;

	while((status=readword(ISQ_PORT))!=0) {
		switch(status&ISQ_EVENT_MASK) {

		case ISQ_RECEIVER_EVENT:
			putstr("ISQ_RECEIVER_EVENT\r\n");
			break;

		case ISQ_TRANSMITTER_EVENT:
			putstr("ISQ_TRANSMITTER_EVENT\r\n");
			if((status&TX_OK)==0) putstr("tx_errors\r\n");
			if(status&TX_LOST_CRS) putstr("tx_carrier_errors\r\n");
			if(status&TX_SQE_ERROR) putstr("tx_heartbeat_errors\r\n");
			if(status&TX_LATE_COL) putstr("tx_window_errors\r\n");
			if(status&TX_16_COL) putstr("tx_aborted_errors\r\n");
			break;

		case ISQ_BUFFER_EVENT:
			putstr("ISQ_BUFFER_EVENT\r\n");
			if(status&READY_FOR_TX) { putstr("out_of_buffers\r\n"); }
			if(status&TX_UNDERRUN) { putstr("send_underrun\r\n"); }
			break;

		case ISQ_RX_MISS_EVENT:
			putstr("ISQ_RX_MISS_EVENT\r\n");
			break;

		case ISQ_TX_COL_EVENT:
			putstr("ISQ_TX_COL_EVENT\r\n");
			break;

		default:
            print("Unknown event",status);
            break;


		}
	}
#endif
}
/*------------------------------------------------------------------------*/
int nb_getpkt(unsigned char *pkt,unsigned short *length)
{
unsigned short
	event,
	len;

	if(((event=readreg(PP_RxEvent))&RX_OK)!=RX_OK) return(0);
	else {

		event=sinw(cs8900_base_addr+RX_FRAME_PORT);
		// print("Status is ",event);

		len=sinw(cs8900_base_addr+RX_FRAME_PORT);
		// print("Length is ",len);

		if(event!=0x104 && event!=0x504) {
			putstr("rx_errors\r\n");
			if(event&RX_RUNT) putstr("rx_runt_errors\r\n");
			if(event&RX_EXTRA_DATA) putstr("rx_length_errors\r\n");
			if(event&RX_CRC_ERROR) putstr("rx_crc_errors\r\n");
			if(event&RX_DRIBBLE) putstr("rx_dribble_errors\r\n");
			return(0);
		}

		*length=len;

		sinsw((void *)(cs8900_base_addr+RX_FRAME_PORT),pkt,len>>1);
		if(len&1) pkt[len-1]=sinw(cs8900_base_addr+RX_FRAME_PORT);

		return(1);
	}
}
/*------------------------------------------------------------------------*/
int nb_putpkt(unsigned char *pkt,unsigned short length)
{
volatile long
	timenow;

unsigned short
	event;

	if(in_tx) {
		if(((readreg(PP_TxEvent))&TX_SEND_OK_BITS)!=TX_OK) return(0);
		else { in_tx=0; return(1); }
	}

	memcpy(pkt+ETH_ALEN,dev_addr,ETH_ALEN);
	if(length<ETH_ZLEN) length=ETH_ZLEN;
	soutw(TX_AFTER_ALL,cs8900_base_addr+TX_CMD_PORT);
	soutw(length,cs8900_base_addr+TX_LEN_PORT);

	/* Test to see if the chip has allocated memory for the packet */
	timenow=0x8000;
	while(timenow-->0) if(readreg(PP_BusST)&READY_FOR_TX_NOW) break;
	if(timenow<=0) { 
		putstr("no mem for tx\r\n");
		return(0);	/* this shouldn't happen */
	}

	/* Write the contents of the packet */
	soutsw((void *)(cs8900_base_addr+TX_FRAME_PORT),pkt,(length+1)>>1);

	in_tx=1;

	return(0);
}
/*------------------------------------------------------------------------*/
