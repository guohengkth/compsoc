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
		fprintf(stderr, "Usage: %s <tile id> <vp id>\n" , argv[0]);
		return EXIT_FAILURE;
	}
	int entry = atoi ( argv[1]);

	if ( entry < 0 || entry > 2 ){
		fprintf(stderr, "Invalid tile id.\n" );
		return EXIT_FAILURE;
	}
	int err = 0;
	printf("mem: 16 KB @ 0x%p\n", (void *)OCM_LOC[entry]);

	volatile void* ocm = NULL, *ocm_s = NULL;
	//volatile uint32_t* buf;
	int memf = open("/dev/mem" , O_RDWR | O_SYNC );

	if(memf < 0) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_;
	}
	ocm_s = ocm = mmap(NULL, OCM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memf,
			OCM_LOC[entry]);
	if(ocm == MAP_FAILED) {
		err = errno;
		fprintf(stderr, "FAILED open %d\n", err);
		goto err_close;
	}

    //if (sizeof(cheapout_t) > CHEAP_MAX_VP_SIZE ) {
    //    fprintf(stderr, "To much memory per virtual platform\r\n" );
    //        abort();
    //}

    int vp = 0;
    shared_memory_map *map = (shared_memory_map*)ocm;
    if ( argc >= 3 ) {
	    vp = atoi(argv[2]);
	    if ( vp < 0 || vp >= NUM_VPS) {
		    vp = 0;
	    }
	    printf("Selecting VP: %d\r\n", vp);
//	    ocm = (void *)(((unsigned int)ocm) + CHEAP_MAX_VP_SIZE*vp);

    }

	cheapout admin_v = (cheapout) &(map->vps[vp]);//ocm;


	printf("==== stdout ====\r\n");
	printf("token_size:      %08X\r\n", admin_v->admin_stdout.token_size); 
	printf("buffer_capacity: %08X\r\n", admin_v->admin_stdout.buffer_capacity); 
	printf("buffer:	 	 %08X\r\n", (unsigned int)(admin_v->admin_stdout.buffer)); 
	printf("malloced_buffer: %08X\r\n", admin_v->admin_stdout.malloced_buffer); 
	printf("writec: 	 %08X\r\n", admin_v->admin_stdout.writec); 
	printf("readc: 		 %08X\r\n", admin_v->admin_stdout.readc); 
	printf("claimed_spaces:  %08X\r\n", admin_v->admin_stdout.claimed_spaces); 
	printf("claimed_tokens:  %08X\r\n", admin_v->admin_stdout.claimed_tokens); 

	printf("==== stdin ====\r\n");
	printf("token_size:      %08X\r\n", admin_v->admin_stdin.token_size); 
	printf("buffer_capacity: %08X\r\n", admin_v->admin_stdin.buffer_capacity); 
	printf("buffer:	 	 %08X\r\n", (unsigned int )(admin_v->admin_stdin.buffer)); 
	printf("malloced_buffer: %08X\r\n", admin_v->admin_stdin.malloced_buffer); 
	printf("writec: 	 %08X\r\n", admin_v->admin_stdin.writec); 
	printf("readc: 		 %08X\r\n", admin_v->admin_stdin.readc); 
	printf("claimed_spaces:  %08X\r\n", admin_v->admin_stdin.claimed_spaces); 
	printf("claimed_tokens:  %08X\r\n", admin_v->admin_stdin.claimed_tokens); 

	printf("==== user_out ====\r\n");
	printf("token_size:      %08X\r\n", admin_v->admin_user_out.token_size); 
	printf("buffer_capacity: %08X\r\n", admin_v->admin_user_out.buffer_capacity); 
	printf("buffer:	 	 %08X\r\n", (unsigned int)(admin_v->admin_user_out.buffer)); 
	printf("malloced_buffer: %08X\r\n", admin_v->admin_user_out.malloced_buffer); 
	printf("writec: 	 %08X\r\n", admin_v->admin_user_out.writec); 
	printf("readc: 		 %08X\r\n", admin_v->admin_user_out.readc); 
	printf("claimed_spaces:  %08X\r\n", admin_v->admin_user_out.claimed_spaces); 
	printf("claimed_tokens:  %08X\r\n", admin_v->admin_user_out.claimed_tokens); 

	printf("==== user_in ====\r\n");
	printf("token_size:      %08X\r\n", admin_v->admin_user_in.token_size); 
	printf("buffer_capacity: %08X\r\n", admin_v->admin_user_in.buffer_capacity); 
	printf("buffer:	 	 %08X\r\n", (unsigned int )(admin_v->admin_user_in.buffer)); 
	printf("malloced_buffer: %08X\r\n", admin_v->admin_user_in.malloced_buffer); 
	printf("writec: 	 %08X\r\n", admin_v->admin_user_in.writec); 
	printf("readc: 		 %08X\r\n", admin_v->admin_user_in.readc); 
	printf("claimed_spaces:  %08X\r\n", admin_v->admin_user_in.claimed_spaces); 
	printf("claimed_tokens:  %08X\r\n", admin_v->admin_user_in.claimed_tokens); 


	munmap((void*)ocm_s, OCM_SIZE);
err_close:
	if(memf > 0) close(memf);
err_:
	return err;
}

