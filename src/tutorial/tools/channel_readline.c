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

// Setup to use normal malloc and free
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;

int main(int argc, char ** argv) 
{
	int err = 0;
	if ( argc < 2 ) {
		fprintf(stderr, "Usage: %s <tile id> <vp id>\n" , argv[0]);
		return EXIT_FAILURE;
	}
	int entry = atoi ( argv[1]);

	if ( entry < 0 || entry > 2 ){
		fprintf(stderr, "Invalid tile id.\n" );
		return EXIT_FAILURE;
	}

	volatile void* ocm = NULL;
	//volatile uint32_t* buf;
	int memf = open("/dev/mem" , O_RDWR | O_SYNC );
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

	int cheapoff = 0;

	volatile cheap admin = (volatile cheap) ocm;
	int vpid = 0;
	if(argc > 2){
		vpid =strtol(argv[2],NULL,10);
		cheapoff = vpid*CHEAP_MAX_VP_SIZE;
		volatile cheapout admin_v = (cheapout)((uint32_t) ocm + cheapoff);
		admin = (volatile cheap) &(admin_v->admin_stdout);
	}

	uint32_t buffer_capacity = admin->buffer_capacity;
	uint32_t mid_buffer = buffer_capacity / 2;
	uint32_t claimed_tokens = 0;

	volatile char** tok = (volatile char**) malloc(buffer_capacity*sizeof(char*));
	uint32_t offset;
	char c;
	int line_len = 0;

	if((claimed_tokens = cheap_claim_tokens(admin, (volatile void**) tok, buffer_capacity))){
		for(unsigned int i=0; i<claimed_tokens; i++) {
			offset = ((uint32_t)tok[i])-PLATFORM_OCM_MEMORY_LOC + cheapoff;
			//printf("offset is %08x\n",offset);
			c = ((volatile char *)ocm)[offset];
			if (c == '\n') {
				line_len = i+1;
				break;
			}
		}

		if(line_len == 0 && claimed_tokens >= mid_buffer){
			line_len = mid_buffer;
		}

		if (line_len > 0){
			printf("%02d %02d: ", entry, vpid);
			for(int i=0; i<line_len; i++) {
				offset = ((uint32_t)tok[i])/*-OCM_LOC[entry]*/ -PLATFORM_OCM_MEMORY_LOC + cheapoff;
				//printf("offset is %08x\n",offset);
				c = ((volatile char *)ocm)[offset];
				putchar(c);
			}

			fflush(stdout);
			cheap_release_spaces(admin, line_len);
		}

		cheap_release_all_claimed_tokens(admin);
	}

	free(tok);

	munmap((void*)ocm, OCM_SIZE);
err_close:
	if(memf > 0) close(memf);
err_:
	return err;
}

