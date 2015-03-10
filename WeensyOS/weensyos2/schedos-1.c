#include "schedos-app.h"
#include "x86sync.h"

/*****************************************************************************
 * schedos-1
 *
 *   This tiny application prints red "1"s to the console.
 *   It yields the CPU to the kernel after each "1" using the sys_yield()
 *   system call.  This lets the kernel (schedos-kern.c) pick another
 *   application to run, if it wants.
 *
 *   The other schedos-* processes simply #include this file after defining
 *   PRINTCHAR appropriately.
 *
 *****************************************************************************/

#ifndef PRINTCHAR
#define PRINTCHAR	('1' | 0x0C00)
#endif

#define LOCKED_WRITE

void
start(void)
{
  int i;

  for (i = 0; i < RUNCOUNT; i++) {
    // Write characters to the console, yielding after each one.

    // Default synchronization. Use system call
#ifndef LOCKED_WRITE
    sys_write(PRINTCHAR);
#endif

    // Secondary synchronization. Use atomic operations in x86sync.h
#ifdef LOCKED_WRITE
    // Spin while waiting for lock
    while (atomic_swap(&lock, 1) != 0)
      continue;

    *cursorpos++ = PRINTCHAR;

    // Unlock
    atomic_swap(&lock, 0);
#endif

    // Yield to schedule
    sys_yield();
  }
	
  sys_exit(0);
}
