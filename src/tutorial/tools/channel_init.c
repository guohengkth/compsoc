#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/mman.h> //mmap
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cheapout.h"



// Setup to use normal malloc and free
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;

int main( int argc, char **argv )
{
	if ( argc < 2 ) {
		fprintf(stderr, "Usage: %s <tile id>\n" , argv[0]);
		return EXIT_FAILURE;
	}
	int entry = atoi ( argv[1]);

	if ( entry < 0 || entry > 2 ){
		fprintf(stderr, "Invalid tile id.\n" );
		return EXIT_FAILURE;
	}
	int err = 0;
	printf("mem: 16 KB @ 0x%p\n", (void *)OCM_LOC[entry]);

	volatile void* ocm = NULL;
	//volatile uint32_t* buf;
	int memf = open("/dev/mem"
			, O_RDWR | O_SYNC //do I want cacheing?
		       );
	if(memf < 0) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_;
	}
	ocm = mmap(NULL, OCM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memf,
			OCM_LOC[entry]);
	if(ocm == MAP_FAILED) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_close;
	}

	int i;
//    if (sizeof(cheapout_t) > CHEAP_MAX_VP_SIZE ) {
//        fprintf(stderr, "To much memory per virtual platform\r\n" );
//            abort();
//    }


	cheapout admin_v = (cheapout) ocm;
	shared_memory_map *map = (shared_memory_map*)ocm;
	{
		cheapout_sys admin_sys = (cheapout_sys) PLATFORM_OCM_MEMORY_LOC;
		cheap admin = cheap_init((void*)(admin_sys->buffer_stdout), CHEAP_BUFFER_SIZE_SYS, sizeof(uint8_t));
		cheap admin2 = cheap_init((void*)(admin_sys->buffer_stdin), CHEAP_BUFFER_SIZE_SYS, sizeof(uint8_t));
		cheap admin3 = cheap_init((void*)(admin_sys->buffer_user_out), CHEAP_BUFFER_SIZE_SYS, sizeof(uint8_t));
		cheap admin4 = cheap_init((void*)(admin_sys->buffer_user_in),  CHEAP_BUFFER_SIZE_SYS, sizeof(uint8_t));
		admin_sys = &(map->vp_kernel);
		memcpy((void *) &(admin_sys->admin_stdout), admin, sizeof(cheap_t));
		memcpy((void *) &(admin_sys->admin_stdin), admin2, sizeof(cheap_t));
		memcpy((void *) &(admin_sys->admin_user_out),admin3, sizeof(cheap_t));
		memcpy((void *) &(admin_sys->admin_user_in), admin4, sizeof(cheap_t));
	}


	cheapout admin_nov = (cheapout) PLATFORM_OCM_MEMORY_LOC;
	cheap admin = cheap_init((void*)(admin_nov->buffer_stdout), CHEAP_BUFFER_SIZE, sizeof(uint8_t));
	cheap admin2 = cheap_init((void*)(admin_nov->buffer_stdin), CHEAP_BUFFER_SIZE, sizeof(uint8_t));
	cheap admin3 = cheap_init((void*)(admin_nov->buffer_user_out), CHEAP_BUFFER_SIZE, sizeof(uint8_t));
	cheap admin4 = cheap_init((void*)(admin_nov->buffer_user_in),  CHEAP_BUFFER_SIZE, sizeof(uint8_t));
	for(i=0; i<NUM_VPS; i++){
		admin_v = &(map->vps[i]);//(cheapout)((uint32_t) ocm + i*CHEAP_MAX_VP_SIZE);

		memcpy((void *) &(admin_v->admin_stdout), admin, sizeof(cheap_t));
		memcpy((void *) &(admin_v->admin_stdin), admin2, sizeof(cheap_t));
		memcpy((void *) &(admin_v->admin_user_out),admin3, sizeof(cheap_t));
		memcpy((void *) &(admin_v->admin_user_in), admin4, sizeof(cheap_t));
	}

	cheap_destroy(admin);


	printf("CHeap stdout initialised\n");

	munmap((void*)ocm, OCM_SIZE);
err_close:
	if(memf > 0) close(memf);
err_:
	return err;
}

