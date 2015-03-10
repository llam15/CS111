#include "schedos-kern.h"
#include "x86.h"
#include "lib.h"

/*****************************************************************************
 * schedos-kern
 *
 *   This is the schedos's kernel.
 *   It sets up process descriptors for the 4 applications, then runs
 *   them in some schedule.
 *
 *****************************************************************************/

// The program loader loads 4 processes, starting at PROC1_START, allocating
// 1 MB to each process.
// Each process's stack grows down from the top of its memory space.
// (But note that SchedOS processes, like MiniprocOS processes, are not fully
// isolated: any process could modify any part of memory.)

#define NPROCS		5
#define PROC1_START	0x200000
#define PROC_SIZE	0x100000

// +---------+-----------------------+--------+---------------------+---------/
// | Base    | Kernel         Kernel | Shared | App 0         App 0 | App 1
// | Memory  | Code + Data     Stack | Data   | Code + Data   Stack | Code ...
// +---------+-----------------------+--------+---------------------+---------/
// 0x0    0x100000               0x198000 0x200000              0x300000
//
// The program loader puts each application's starting instruction pointer
// at the very top of its stack.
//
// System-wide global variables shared among the kernel and the four
// applications are stored in memory from 0x198000 to 0x200000.  Currently
// there is just one variable there, 'cursorpos', which occupies the four
// bytes of memory 0x198000-0x198003.  You can add more variables by defining
// their addresses in schedos-symbols.ld; make sure they do not overlap!


// A process descriptor for each process.
// Note that proc_array[0] is never used.
// The first application process descriptor is proc_array[1].
static process_t proc_array[NPROCS];

// A pointer to the currently running process.
// This is kept up to date by the run() function, in mpos-x86.c.
process_t *current;

// The preferred scheduling algorithm.
int scheduling_algorithm;
int multi_level_queue;

/*****************************************************************************
 * start
 *
 *   Initialize the hardware and process descriptors, then run
 *   the first process.
 *
 *****************************************************************************/

void
start(void)
{
	int i;

	// Set up hardware (schedos-x86.c)
	segments_init();
	interrupt_controller_init(1);
	console_clear();

	// Initialize process descriptors as empty
	memset(proc_array, 0, sizeof(proc_array));
	for (i = 0; i < NPROCS; i++) {
		proc_array[i].p_pid = i;
		proc_array[i].p_state = P_EMPTY;
		proc_array[i].p_share_runs = 0;
	}

	// SETTING QUEUE NUMBERS FOR MULTI-LEVEL QUEUE SCHEDULING
	proc_array[1].p_queue = 0;
	proc_array[2].p_queue = 1;
	proc_array[3].p_queue = 0;
	proc_array[4].p_queue = 1;

	// Set up process descriptors (the proc_array[])
	for (i = 1; i < NPROCS; i++) {
		process_t *proc = &proc_array[i];
		uint32_t stack_ptr = PROC1_START + i * PROC_SIZE;

		// Initialize the process descriptor
		special_registers_init(proc);

		// Set ESP
		proc->p_registers.reg_esp = stack_ptr;

		// Load process and set EIP, based on ELF image
		program_loader(i - 1, &proc->p_registers.reg_eip);

		// Mark the process as runnable!
		proc->p_state = P_RUNNABLE;
	}
	
	// Initialize the cursor-position shared variable to point to the
	// console's first character (the upper left).
	cursorpos = (uint16_t *) 0xB8000;

	// Initialize the scheduling algorithm.
	scheduling_algorithm = 0;
	multi_level_queue = (scheduling_algorithm == 4) ? 1 : 0;
	// Switch to the first process.
	proc_array[1].p_share_runs++;
	run(&proc_array[1]);

	// Should never get here!
	while (1)
		/* do nothing */;
}



/*****************************************************************************
 * interrupt
 *
 *   This is the weensy interrupt and system call handler.
 *   The current handler handles 4 different system calls (two of which
 *   do nothing), plus the clock interrupt.
 *
 *   Note that we will never receive clock interrupts while in the kernel.
 *
 *****************************************************************************/

void
interrupt(registers_t *reg)
{
	// Save the current process's register state
	// into its process descriptor
	current->p_registers = *reg;

	switch (reg->reg_intno) {

	case INT_SYS_YIELD:
		// The 'sys_yield' system call asks the kernel to schedule
		// the next process.
		schedule();

	case INT_SYS_EXIT:
		// 'sys_exit' exits the current process: it is marked as
		// non-runnable.
		// The application stored its exit status in the %eax register
		// before calling the system call.  The %eax register has
		// changed by now, but we can read the application's value
		// out of the 'reg' argument.
		// (This shows you how to transfer arguments to system calls!)
		current->p_state = P_ZOMBIE;
		current->p_exit_status = reg->reg_eax;
		schedule();

	case INT_SYS_PRIORITY:
	        // Set process's priority to priority in %eax register
	        current->p_priority = reg->reg_eax;
		run(current);

	case INT_SYS_SHARE:
	        // Set process's share priority to share priority in %eax register
	        current->p_share = reg->reg_eax;
		run(current);

	case INT_SYS_WRITE:
	        // Write character and increment cursor position
		*cursorpos++ = reg->reg_eax;
		run(current);

	case INT_SYS_QUEUE:
	        // Set process's queue to number in %eax register
	        current->p_queue = reg->reg_eax;
	        run(current);

	case INT_CLOCK:
		// A clock interrupt occurred (so an application exhausted its
		// time quantum).
		// Switch to the next runnable process.
		schedule();

	default:
		while (1)
			/* do nothing */;
	}
}



/*****************************************************************************
 * schedule
 *
 *   This is the weensy process scheduler.
 *   It picks a runnable process, then context-switches to that process.
 *   If there are no runnable processes, it spins forever.
 *
 *   This function implements multiple scheduling algorithms, depending on
 *   the value of 'scheduling_algorithm'.  We've provided one; in the problem
 *   set you will provide at least one more.
 *
 *****************************************************************************/

void
schedule(void)
{
  pid_t pid = current->p_pid;

  // Round Robin
  if (scheduling_algorithm == 0) {
    while (1) {
      pid = (pid + 1) % NPROCS;

      // Run the selected process, but skip
      // non-runnable processes.
      // Note that the 'run' function does not return.
      if (proc_array[pid].p_state == P_RUNNABLE) {
	// If was redirected from multi-level queue scheduling,
	// change scheduling algorithm back to 4
	if (multi_level_queue)
	  scheduling_algorithm = 4;

	run(&proc_array[pid]);
      }
    }
  }

  // Priority Scheduling (based on PID)
  else if (scheduling_algorithm == 1) {
    // Find runnable process with highest priority and run it
    while (1) {
      for (pid = 1; pid < NPROCS; pid++) {
	if (proc_array[pid].p_state == P_RUNNABLE) {
	  // If was redirected from multi-level queue scheduling,
	  // change scheduling algorithm back to 4
	  if (multi_level_queue)
	    scheduling_algorithm = 4;

	  run(&proc_array[pid]);
	}
      }
    }
  }

  // Priority Scheduling (set by processes)
  else if (scheduling_algorithm == 2) {
    // Find highest priority in list, because starting process
    // will not necessarily start at the highest priority.
    pid_t i = pid;
    // Do not include current process if it has exited
    while (proc_array[pid].p_state == P_ZOMBIE ||
	   proc_array[pid].p_state == P_EMPTY)

      i = (i+1) % NPROCS;

    int smallest = proc_array[i].p_priority;

    for (i = 1; i < NPROCS; i++) {
      if (proc_array[i].p_state == P_RUNNABLE && 
	  proc_array[i].p_priority < smallest)
	smallest = proc_array[i].p_priority;
    }      

    // Find next process with highest priority and run it
    while (1) {
      pid = (pid + 1) % NPROCS;

      if (proc_array[pid].p_state == P_RUNNABLE && 
	  proc_array[pid].p_priority <= smallest) {
	run(&proc_array[pid]);
      }
    }
  }

  // Proportional Share Scheduling
  else if (scheduling_algorithm == 3) {
    while (1) {
      if (proc_array[pid].p_state == P_RUNNABLE) {
	// If have run your entire share, then reset and go to next process
	if (proc_array[pid].p_share_runs >= proc_array[pid].p_share)
	  proc_array[pid].p_share_runs = 0;

	// Else run and increment number of times run
	else {
	  proc_array[pid].p_share_runs++;
	  run(&proc_array[pid]);
	}
      }

      pid = (pid + 1) % NPROCS;
    }    
  }

  else if (scheduling_algorithm == 4) {
    // Do not include current process if it has exited
    while (proc_array[pid].p_state == P_ZOMBIE ||
	   proc_array[pid].p_state == P_EMPTY)
      pid = (pid + 1) % NPROCS;
      
    int smallest = proc_array[pid].p_queue;

    // Find highest priority queue number
    pid_t i;
    for (i = 1; i < NPROCS; i++) {
      if (proc_array[i].p_state == P_BLOCKED)
	proc_array[i].p_state = P_RUNNABLE;

      if (proc_array[i].p_state == P_RUNNABLE && 
	  proc_array[i].p_queue < smallest)
	smallest = proc_array[i].p_queue;
    }

    // Block processes not in current queue number
    for (i = 1; i < NPROCS; i++) {
      if (proc_array[i].p_state == P_RUNNABLE && 
	  proc_array[i].p_queue != smallest)
	proc_array[i].p_state = P_BLOCKED;
    }

    // Only valid queue numbers are 0 and 1.
    if (smallest == 0 || smallest == 1) {
      // Scheduling algorithm 0 for queue level 0, and 1 for queue level 1
      scheduling_algorithm = smallest;
      schedule();
    }

    // If scheduling algorithm is 4 and we get here, must have invalid queue number
    cursorpos = console_printf(cursorpos, 0x100, "\nInvalid queue number: %d\n", smallest);
  }

  else {
    // If we get here, we are running an unknown scheduling algorithm.
    cursorpos = console_printf(cursorpos, 0x100, "\nUnknown scheduling algorithm %d\n", scheduling_algorithm);
  }

  while (1)
    /* do nothing */;
}
