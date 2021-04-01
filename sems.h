/*sems.h heater file*/

#define DEBUG2 1
#define ITEM_IN_USE 1
#define ITEM_WAITING 2

#define CHECKMODE { \
	if (psr_get() & PSR_CURRENT_MODE) { \
    		console("Trying to invoke syscall from kernel, halt(1)\n"); \
    		halt(1); \
	} \
}

/* Process Control Block */
struct pcb_ptr {
    int pid;
    char *name;
    int status;
    int num_of_children;
    pcb_ptr parent_ptr;
    pcb_ptr child_ptr;
    pcb_ptr sibling_ptr;
    int start_mbox;
    char *start_arg;
    int (* start_func) (char *);
};
