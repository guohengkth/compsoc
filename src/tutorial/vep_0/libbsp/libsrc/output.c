#include <stdlib.h>
#include "cheapout.h"
#include "cheap.h"
#include "xil_printf.h"

// Setup to use normal malloc and free    
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;  

volatile  cheapout myadmins = (cheapout) PLATFORM_OCM_MEMORY_LOC;

void outbyte(char c) {
  volatile uint8_t* buff[1];
  while (cheap_claim_spaces((cheap) &(myadmins->admin_stdout), (volatile void **)buff, 1) == 0);
  *((volatile uint8_t*) buff[0]) = c;
  cheap_release_tokens ((cheap) &(myadmins->admin_stdout), 1);
}

void *_sbrk(ptrdiff_t incr)
{
  register uint32_t var asm ("sp");
  extern unsigned char _end[];   // defined by linker
  static unsigned long heap_end;

  if (heap_end == 0) heap_end = (long)_end;

  heap_end += incr;
  /*
  if (heap_end > var) {
    xil_printf("FAILED to allocate extra memory, hitting stack.\r\n");
    while (1) ;
  }
  */
  return (void *)(heap_end - incr);
}

