/*------------------------------------------------------------------------*/
/*
	sys_arch.c : llC implementation -> single threaded.
*/
/*------------------------------------------------------------------------*/

#include "lwip/debug.h"
#include "lwip/sys.h"
#include "lwip/opt.h"

struct sys_mbox {
  char dummy;
};

struct sys_mbox mboxes[1];

struct sys_sem {
  char dummy;
};

struct sys_sem sems[1];

struct sys_thread {
  struct sys_timeouts timeouts;
};

static struct sys_thread threads[1]={ { { 0 } }, };

/*-----------------------------------------------------------------------------------*/
static struct sys_thread *current_thread(void)
{
  return &threads[0];
}
/*-----------------------------------------------------------------------------------*/
void sys_thread_new(void (* function)(void *arg), void *arg)
{
}
/*-----------------------------------------------------------------------------------*/
struct sys_mbox *sys_mbox_new()
{
  return &mboxes[0];
}
/*-----------------------------------------------------------------------------------*/
void sys_mbox_free(struct sys_mbox *mbox)
{
}
/*-----------------------------------------------------------------------------------*/
void sys_mbox_post(struct sys_mbox *mbox, void *msg)
{
}
/*-----------------------------------------------------------------------------------*/
u16_t sys_arch_mbox_fetch(struct sys_mbox *mbox, void **msg, u16_t timeout)
{
  return 0;
}
/*-----------------------------------------------------------------------------------*/
struct sys_sem *sys_sem_new(u8_t count)
{
  return &sems[0];
}
/*-----------------------------------------------------------------------------------*/
u16_t sys_arch_sem_wait(struct sys_sem *sem, u16_t timeout)
{
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void sys_sem_signal(struct sys_sem *sem)
{
}
/*-----------------------------------------------------------------------------------*/
void sys_sem_free(struct sys_sem *sem)
{
}
/*-----------------------------------------------------------------------------------*/
void sys_init()
{
}
/*-----------------------------------------------------------------------------------*/
struct sys_timeouts *sys_arch_timeouts(void)
{
  struct sys_thread *thread;
  thread=current_thread();
  return &thread->timeouts;  
}
/*-----------------------------------------------------------------------------------*/
