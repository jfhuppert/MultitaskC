/*-----------------------------------------------------------------------------------*/
/*
	main.c
*/
/*-----------------------------------------------------------------------------------*/

#include "lwip/debug.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"

#include "lwip/stats.h"
#include "lwip/tcpip.h"

#include "netif/ethskl.h"

#include "lwip/ip_addr.h"

/*-----------------------------------------------------------------------------------*/
int main(int argc, char*argv[])
{
struct ip_addr
  ipaddr,
  netmask,
  gw;

struct netif
  *ethsklif;

volatile unsigned long
  slow_time,
  fast_time;
    
  sys_init();
  mem_init();
  memp_init();
  pbuf_init();
  ip_init();
  udp_init();
  tcp_init();

  IP4_ADDR(&gw,192,168,1,1);
  IP4_ADDR(&ipaddr,192,168,1,100);
  IP4_ADDR(&netmask,255,255,255,0);

  ethsklif=netif_add(&ipaddr,&netmask,&gw,ethskl_init,ip_input);
  netif_set_default(ethsklif);

  slow_time=0;
  fast_time=0;

  for(;;) {

	if(fast_time>20000) { tcp_fasttmr(); fast_time=0; }
	if(slow_time>50000) { tcp_slowtmr(); slow_time=0; }
	fast_time++;
	slow_time++;

	ethskl_input(ethsklif);

  }

}
/*-----------------------------------------------------------------------------------*/
