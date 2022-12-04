#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/mman.h> //mmap
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cheapout.h"
#include <signal.h>


/**
 * READS from 3 platforms, VP 1,2,3
 */

#define VP_START 0
#define NUM_PLATFORMS 3

// Setup to use normal malloc and free
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;

int run= 1;

void my_handler(int s){
	printf("Quitting %d\n",s);
	run = 0;
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char ** argv) 
{
	int err = 0;

	volatile void* ocm = NULL;
	//volatile uint32_t* buf;
	int memf = open("/dev/mem" , O_RDWR | O_SYNC );
	if(memf < 0) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_;
	}
	ocm = mmap(NULL, NUM_PLATFORMS*OCM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memf,
			OCM_LOC[0]);
	if(ocm == MAP_FAILED) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_close;
	}

	int cheapoff = 0;

	volatile cheap admin = (volatile cheap) ocm;

	// Assume buffer is same for all tiles.
	uint32_t buffer_capacity = admin->buffer_capacity;
	uint32_t mid_buffer = buffer_capacity / 2;

	volatile char** tok = (volatile char**) malloc(buffer_capacity*sizeof(char*));

	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);


	printf("start reading\n");
	while(run)
	{
		for ( int platform = 0; platform < NUM_PLATFORMS ; platform++) {
			for  ( int i = VP_START; i < NUM_VPS; i++) {
				uint32_t offset;
				char c;
				int line_len = 0;

				/**
				 * Lookup administration.
				 */

				shared_memory_map *map = (shared_memory_map*)((uint32_t)ocm+platform*OCM_SIZE);
				//cheapoff = i*CHEAP_MAX_VP_SIZE+platform*OCM_SIZE;
				volatile cheapout admin_v =  &(map->vps[i]);//(cheapout)((uint32_t) ocm + cheapoff);
				cheapoff = ((uint32_t)(admin_v) - (uint32_t)map) +( (uint32_t)platform*OCM_SIZE);
				//printf("offset: %08X\r\n", cheapoff);
				admin = (volatile cheap) &(admin_v->admin_stdout);

				uint32_t claimed_tokens = 0;
				if((claimed_tokens = cheap_claim_tokens(admin, (volatile void**) tok, buffer_capacity))){
					for(unsigned int x=0; x<claimed_tokens; x++) {
						offset = ((uint32_t)tok[x])-PLATFORM_OCM_MEMORY_LOC + cheapoff;
						c = ((volatile char *)ocm)[offset];
						if (c == '\n') {
							line_len = x+1;
							break;
						}
					}

					if(line_len == 0 && claimed_tokens >= mid_buffer){
						line_len = mid_buffer;
					}

					if (line_len > 0){
						printf("%02d %02d: ", platform, i+1);
						for(int i=0; i<line_len; i++) {
							offset = ((uint32_t)tok[i])/*-OCM_LOC[entry]*/ -PLATFORM_OCM_MEMORY_LOC + cheapoff;
//							printf("offset2 is %08x\n",offset);
							c = ((volatile char *)ocm)[offset];
							putchar(c);
							//				printf("%02X", c);
						}

						fflush(stdout);
						cheap_release_spaces(admin, line_len);
					}

					cheap_release_all_claimed_tokens(admin);
				}
			}
		}
	}

	free(tok);

	munmap((void*)ocm, NUM_PLATFORMS*OCM_SIZE);
err_close:
	if(memf > 0) close(memf);
err_:
	return err;
}

