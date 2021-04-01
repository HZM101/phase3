/* ------------------------------------------------------------------------
   phase3.c

   University of Arizona South
   Computer Science 452

   @authors: Erik Ibarra Hurtado, Hassan Martinez, Victor Alvarez

   ------------------------------------------------------------------------ */

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <libuser.h>
#include "sems.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>

/* ------------------------- Prototypes ----------------------------------- */

int     start2(char *); 
int     spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority);
int     wait_real(int *status);

/* -------------------------- Globals ------------------------------------- */

int debugflag3 = 0;
pcb_ptr ProcessTable3[MAXPROC];
pcb_ptr empty_pcb = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - start2
   Purpose - Initializes start3 process in user mode.
             Start the phase3 test process.
   ----------------------------------------------------------------------- */
int start2(char *arg)
{
    int		pid;
    int		status;

    /*
     * Check kernel mode here.
     */

    if ((psr_get() & PSR_CURRENT_MODE) == 0)
    {
        console("start2(): Not in kernel mode\n");
        halt(1);
    }
    
    /*
     * Data structure initialization as needed.
     */

    /* Setting process table. */ 
    for (int i = 0; i < MAXPROC; i++)
    {
        ProcessTable3[i] = empty_pcb;
    }

    /* Setting system call handlers. */
    for (int i = 0; i < MAXSYSCALLS; i++)
    {
        sys_vec[i] = nullsys3;
    }

    /* Setting handlers functions. */
    sys_vec[SYS_SPAWN] = spawn; 
    sys_vec[SYS_WAIT] = wait; 
    sys_vec[SYS_TERMINATE] = terminate; 
    sys_vec[SYS_GETTIMEOFDAY] = getTimeOfDay;
    sys_vec[SYS_CPUTIME] = cpuTime;
    sys_vec[SYS_GETPID] = getPID;

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscall_handler; spawn_real is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes usyscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawn_real().
     *
     * Here, we only call spawn_real(), since we are already in kernel mode.
     *
     * spawn_real() will create the process by using a call to fork1 to
     * create a process executing the code in spawn_launch().  spawn_real()
     * and spawn_launch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawn_real() will
     * return to the original caller of Spawn, while spawn_launch() will
     * begin executing the function passed to Spawn. spawn_launch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawn_real() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */

    pid = spawn_real("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);

    pid = wait_real(&status);

    if (DEBUG2 && debugflag3)
    {
        console ("start2(): Calling quit(0)\n");
    }
    quit(0);
} /* start2 */

