/* The kernel call implemented in this file:
 *   m_type:	SYS_CLEAR
 *
 * The parameters for this kernel call are:
 *   m_lsys_krn_sys_clear.endpt		
 * (endpoint of process to clean up)
 */

#include "kernel/system.h"

#include <minix/endpoint.h>
#include "kernel/arch/i386/hproto.h"
#include "kernel/arch/i386/htype.h"

#if USE_CLEAR

/*=============================================*
 *				do_clear       *
 *=============================================*/
int do_clear(struct proc * caller, message * m_ptr)
{
/* Handle sys_clear. Only the PM can request 
 * other process slots to be cleared
 * when a process has exited.
 * The routine to clean up a process table 
 * slot cancels outstanding timers, 
 * possibly removes the process from the message 
 * queues, and resets certain 
 * process table fields to the default values.
 */
  struct proc *rc;
  int exit_p;
  int i;

  if(!isokendpt(
    m_ptr->m_lsys_krn_sys_clear.endpt, &exit_p)) {
      /* get exiting process */
      return EINVAL;
  }
  rc = proc_addr(exit_p);	/* clean up */

  release_address_space(rc);

  /* Don't clear if already cleared. */
  if(isemptyp(rc)) return OK;

  /* Check the table with IRQ hooks
   * to see if hooks should be released. */
  for (i=0; i < NR_IRQ_HOOKS; i++) {
      if (rc->p_endpoint == irq_hooks[i].proc_nr_e) {
        /* remove interrupt handler */
        rm_irq_handler(&irq_hooks[i]);
        /* mark hook as free */	
        irq_hooks[i].proc_nr_e = NONE;	
      } 
  }
 
   /* Add by EKA: free the PE working set list */
   free_pram_mem_blocks(rc, 1);
   handle_hsr_events(rc);
   free_hsrs(rc);
   reset_hardening_attri(rc);
   printf("### clearing proc: %s %d ticks: %d user: %d sys: %d####\n", 
         rc->p_name, rc->p_endpoint, rc->p_ticks, 
         rc->p_user_time, rc->p_sys_time);
  /**End Add by EKA**/
   /* Remove the process' ability 
   * to send and receive messages */
  clear_endpoint(rc);

  /* Turn off any alarm timers at the clock. */   
  reset_kernel_timer(&priv(rc)->s_alarm_timer);

 

  /* Make sure that the exiting process is no 
   * longer scheduled,
   * and mark slot as FREE. Also mark saved fpu 
   * contents as not significant.
   */
  RTS_SETFLAGS(rc, RTS_SLOT_FREE);
  /* release FPU */
  release_fpu(rc);
  rc->p_misc_flags &= ~MF_FPU_INITIALIZED;
  /* Release the process table slot. 
   * If this is a system process, also
   * release its privilege structure.  Further 
   * cleanup is not needed at
   * this point. All important fields are 
   * reinitialized when the 
   * slots are assigned to another, new process. 
   */
  if (priv(rc)->s_flags & SYS_PROC) 
    priv(rc)->s_proc_nr = NONE;
  return OK;
}

#endif /* USE_CLEAR */
