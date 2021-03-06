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

int         start2(char *); 
extern int  start3(char *);

void        spawn(sysargs *);
int         spawn_launch(char *);
int         spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority);
void        wait(sysargs *);
int         wait_real(int *);
void        terminate(sysargs *);      
int         terminate_real(int);
void        cputime(sysargs *);
void        gettimeofday(sysargs *);
void        getPID(sysargs *);
void        nullsys3(sysargs *);
void        check_kernel_mode();
void        set_user_mode();
int         insertChild(int, int);
int         removeChild(int);       

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

    check_kernel_mode();
    
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




/* wait */
void wait(sysargs *args_ptr)
{
    check_kernel_mode();
    int *status = args_ptr->arg2;
    int pid = wait_real(status);

    if (debugflag3) 
    {
        console("wait(): joined with child pid: %d, stauts: %d\n", pid, *status);
    }

    args_ptr->arg1 = (void *) ((long) pid);
    args_ptr->arg2 = (void *) ((long) *status);
    args_ptr->arg4 = (void *) ((long) 0);

    /* Terminate process if zapped. */
    if (is_zapped()) 
    {
        terminate_real(1);
    } else 
    {
        init_user_mode();
    }

} /* wait */


/* wait_real */
int wait_real(int *status)
{
    check_kernel_mode();
    if (debugflag3) 
    {
        console("wait_real(): reached wait_real\n");
        int pid = join(status);
        return pid;
    }
} /* wait_real */


/* terminate */
void terminate(sysargs *args_ptr)
{
    check_kernel_mode();
    int status = (int) ((long) args_ptr->arg1);
    terminate_real(status);
    set_user_mode();
} /* terminate */


/* terminate_real */
int terminate_real(int status)
{
    check_kernel_mode();
    int walker = getpid() % MAXPROC;
    int parent_location;
    int result;

    /* Set the parent location. */
    parent_location = ProcessTable3[walker].parent_ptr->pid % MAXPROC;
    
    /* If their is no child to terminate remove parent. */
    if (ProcessTable3[walker].num_children == 0)
    {
        if(ProcessTable3[parent_location].status == ITEM_WAITING)
        {

            removeChild(ProcessTable3[walker].parent_ptr->pid);
            result = MboxSend(ProcessTable3[parent_location].start_mbox, &status, sizeof(int));
        }
        else
        {
            removeChild(ProcessTable3[walker].parent_ptr->pid);
        }
        quit(0);
    }

    /* Zapping each child. */
    while (ProcessTable3[walker].num_children != 0)
    {
        result = zap(ProcessTable3[walker].child_ptr->pid);
        result = removeChild(walker);
    }

    /* User-level process for quit(0). */
    removeChild(ProcessTable3[parent_location].pid);
    result = MboxSend(ProcessTable3[parent_location].start_mbox, &status, sizeof(int));
    quit(0);

    return 0;

} /* terminate_real */


/* cputime */
static void cputime(sysargs *args_ptr)
{

    *((int*)(args_ptr->arg1)) = readtime();

} /* cputime */


/* gettimeofday */
void gettimeofday(sysargs *)
{

    *((int*)(args_ptr->arg1)) = sys_clock();

} /* gettimeofday */


/* getPID */
void getPID(sysargs *args_ptr)
{

    *((int*)(args_ptr->arg1)) = getpid();

} /* getPID */


/* nullsys3 */
void nullsys3(sysargs *args_ptr)
{
   printf("nullsys3(): Invalid syscall %d\n", args_ptr->number);
   printf("nullsys3(): process %d terminating\n", getpid());
   terminate_real(1);
} /* nullsys3 */


/* checkKernelMode */
void check_kernel_mode() 
{
    if ((PSR_CURRENT_MODE & psr_get()) == 0) {
        halt(1);
    }
} /* checkKernelMode */


/* set_user_mode */
void set_user_mode()
{
    psr_set(psr_get() & ~PSR_CURRENT_MODE);
} /* set_user_mode */


/* insertChild */
int insertChild(int child_location, int parent_location)
{

    /* Family tree (walker) if their is no empty space for child. Child set to 0. */
    int num_children = 0;
    pcb_ptr walker;

    /* Check for empty space. */
    if (ProcessTable3[parent_location].child_ptr == NULL)
    {
        ProcessTable3[parent_location].child_ptr = &ProcessTable3[child_location];
        num_children++;
    }
    /* If no empty space then their. */
    else
    {
        num_children = 1;

        /* Walker point to first child. */
        walker = ProcessTable3[parent_location].child_ptr;

        /* Looping around the sibling. */
        while (walker->sibling_ptr != NULL)
        {
            walker = walker->sibling_ptr;
            num_children++;
        }

        /* Set sibling to empty NULL from looping. */
        walker->sibling_ptr = &ProcessTable3[child_location];
        num_children++;
    }
    
    return num_children;
} /* insertChild */


/* removeChild */
int removeChild(int parent_location)
{
    pcb_ptr walker;

    /* Check for child if 0 then their is no child to remove. */
    if (ProcessTable3[parent_location].num_children == 0)
    {
        return ProcessTable3[parent_location].num_children;
    }
    else
    {
        walker = ProcessTable3[parent_location].child_ptr;

        /* Point to sibling. */
        ProcessTable3[parent_location].child_ptr = ProcessTable3[parent_location].child_ptr->sibling_ptr;

        /* Removing child. */
        ProcessTable3[parent_location].num_children--;
        walker->sibling_ptr = NULL;
    }
    
    return ProcessTable3[parent_location].num_children;
} /* removeChild */