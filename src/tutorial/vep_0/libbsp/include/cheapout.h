#ifndef __CHEAPOUT_H__
#define __CHEAPOUT_H__

#include <platform.h>
#include <cheap.h>
#include <cheap_s.h>

/**
 * main memory map.
 */
typedef struct cheapout_sys_s
{
	volatile cheap_t admin_stdout;
	volatile char buffer_stdout[CHEAP_BUFFER_SIZE_SYS];
	volatile cheap_t admin_stdin;
	volatile char buffer_stdin[CHEAP_BUFFER_SIZE_SYS];
    // User Fifos
	volatile cheap_t admin_user_out;
	volatile char buffer_user_out[CHEAP_BUFFER_SIZE_SYS];
	volatile cheap_t admin_user_in;
	volatile char buffer_user_in[CHEAP_BUFFER_SIZE_SYS];
	uint8_t padding[896];
} cheapout_sys_t;
typedef struct cheapout_s
{
	volatile cheap_t admin_stdout;
	volatile char buffer_stdout[CHEAP_BUFFER_SIZE];
	volatile cheap_t admin_stdin;
	volatile char buffer_stdin[CHEAP_BUFFER_SIZE];
    // User Fifos
	volatile cheap_t admin_user_out;
	volatile char buffer_user_out[CHEAP_BUFFER_SIZE];
	volatile cheap_t admin_user_in;
	volatile char buffer_user_in[CHEAP_BUFFER_SIZE];
//	uint8_t padding[128];
    // Userspace (space is 896 big)
} cheapout_t;

typedef cheapout_sys_t* cheapout_sys;
typedef cheapout_t* cheapout;


typedef struct 
{
    cheapout_sys_t vp_kernel;
    cheapout_t  vps[8];
} shared_memory_map;


/**
 * Locations into the global memory map.
 */
static const unsigned int OCM_SIZE = 16*1024;
static const unsigned int OCM_LOC[3]  ={
       	0x80000000,
       	0x80004000,
       	0x80008000
};

#endif //__CHEAPOUT_H__
