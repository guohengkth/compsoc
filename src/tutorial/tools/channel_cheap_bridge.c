/**
 * Copyright 2019 Verintec Solutions B.V.
 */
#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include<string.h>
#include<sys/socket.h>
#include <time.h>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/mman.h> //mmap

#include<arpa/inet.h>
#include<unistd.h>

#include <cheap.h>
#include "cheapout.h"

// Setup to use normal malloc and free
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;

int main(int argc , char *argv[])
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
	volatile void* ocm = NULL;
	//volatile uint32_t* buf;
	int memf = open("/dev/mem" , O_RDWR | O_SYNC );
	if(memf < 0) {
		fprintf(stderr, "FAILED open %s\n", strerror ( errno ) );
		return EXIT_FAILURE;
	}
	ocm = mmap(NULL, OCM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memf,
			OCM_LOC[entry]);
	if(ocm == MAP_FAILED) {
		fprintf(stderr, "FAILED memmap: %s\n", strerror ( errno ) );
		close ( memf );
		return EXIT_FAILURE;
	}


	volatile cheap admin  = NULL;
	volatile cheap admin2 = NULL;

	volatile cheapout admin_v = (cheapout)(((uint32_t) ocm) );

	int vpid = 0;

	int cheapoff = 0;
	if(argc > 2){
		vpid =strtol(argv[2],NULL,10);
		cheapoff = vpid*CHEAP_MAX_VP_SIZE;
		admin_v = (cheapout)((uint32_t) ocm + cheapoff);
	}
	admin = (volatile cheap) &(admin_v->admin_stdout);
	admin2 = (volatile cheap) &(admin_v->admin_stdin);

	uint32_t port_number = 9876;

	/**
	 * If there is an extra argument specified, parse the first number as port number.
	 */
	if ( argc > 3 ) {
		port_number = (uint32_t)strtol(argv[3], NULL, 10);
	}

	/**
	 * Set socket to use re-use. This way you can directly restart it again, and do not have to wait for the kernel to
	 * release the port.
	 */
	int socket_desc = socket(AF_INET , SOCK_STREAM|SOCK_NONBLOCK , 0);
	if (socket_desc == -1)
	{
		fprintf(stderr, "Failed to create socket: %s\n" , strerror(errno));
		return EXIT_FAILURE;
	}
	int reuse = 1;
	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

#ifdef SO_REUSEPORT
	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
		perror("setsockopt(SO_REUSEPORT) failed");
#endif


	printf("+Socket created\n" );

	//Prepare the sockaddr_in structure
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port_number );

	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
		close ( socket_desc );
		return EXIT_FAILURE;
	}
	printf("+bind on port: %d done\n", port_number);

	/**
	 * Listen for client, accept 1 connection.
	 */
	listen(socket_desc , 1);
	printf("+ Listen done\n");
	unsigned int quit = 0;
	while ( !quit )
	{
		fd_set readset;
		fd_set writeset;
		struct sockaddr_in client;

		printf("-Waiting for incoming connections.\n");
		socklen_t c = sizeof(struct sockaddr_in);
		//accept connection from an incoming client
		int client_sock = -1;
		while ( client_sock < 0){

			FD_ZERO ( &readset );

			FD_SET ( STDIN_FILENO, &readset);
			FD_SET ( socket_desc, &readset);

			int res = select ( socket_desc+1, &readset, NULL, NULL, NULL);
			if ( res < 0 ) {
				return EXIT_FAILURE;
			}

			if ( FD_ISSET ( STDIN_FILENO, &readset ) )
			{
				quit = 1;
				break;
			}
			if ( FD_ISSET ( socket_desc , &readset ) )
			{
				printf("- Accepting socket\n");
				client_sock = accept4(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c,SOCK_NONBLOCK);
				if (client_sock < 0)
				{
					fprintf(stderr,"* accept failed: %s\n", strerror(errno));
					continue;
				}
			}
		}
		if ( quit ) break;
		printf("+Connection accepted\n");

		size_t rtotal_size = 0;
		size_t wtotal_size = 0;
#if 1
		struct timespec start, stop;
		clock_gettime ( CLOCK_REALTIME, &start);
#endif
		unsigned int tok_size = admin->buffer_capacity;
		if ( tok_size< admin2->buffer_capacity ) {
			tok_size = admin2->buffer_capacity;
		}
		char *tok[tok_size];
		char client_message[tok_size];
		while ( client_sock >= 0 )
		{
			ssize_t read_size;
			FD_ZERO ( &readset );
			FD_ZERO ( &writeset );

			FD_SET ( STDIN_FILENO, &readset);
			FD_SET ( client_sock, &readset);
			FD_SET ( client_sock, &writeset);

			struct timeval time_d;
			time_d.tv_sec = 0;
			time_d.tv_usec = 100;

			int res = select ( client_sock+1, &readset, NULL, NULL, &time_d);
			if ( res < 0 ) {
				return EXIT_FAILURE;
			}

			{
				int windex = 0;
				int ct = 0;
				while( ( ct = cheap_claim_tokens(admin, (volatile void**) (&tok[0]), tok_size) ) > 0  )
				{
					for ( int i = 0; i < ct; i++ ) {
						uint32_t offset = ((uint32_t)tok[i])-0x000C0000+ cheapoff;
						client_message[windex++] = (((volatile char *)ocm)[offset]);
					}
					cheap_release_spaces(admin, ct);
				}
				ssize_t written =0;
				while ( client_sock >= 0 && written < windex ){
					ssize_t l= write ( client_sock , &(client_message[written]), windex-written);
					if ( l < 0 ){
						if ( errno != 11 ) {
							printf("-Connection write errored: %d %s\n",errno, strerror (errno) );
							close(client_sock);
							client_sock = -1;
						}
						continue;

					} else if (l == 0 ){
						usleep (10);
					}else {
						written+=l;
					}
					wtotal_size +=l;
				}
			}
			if ( FD_ISSET ( STDIN_FILENO, &readset ) )
			{
				close ( client_sock );
				client_sock = -1;
				quit = 1;
				continue;
			}
			if ( FD_ISSET ( client_sock, &readset ) )
			{
				unsigned int a = cheap_get_spaces_remaining ( admin2 );
				if ( a > sizeof(client_message) ) {
					a = sizeof(client_message);
				}
				if ( a >  0 )
				{
					//Receive a message from client
					read_size = read(client_sock , client_message , a/*sizeof(client_message)*/);
					if ( read_size == 0 ){
						printf("-Connection closed\n");
						close(client_sock);
						client_sock = -1;
						continue;
					} else if ( read_size < 0 ){
						printf("-Connection errored: %s\n", strerror (errno) );
						close(client_sock);
						client_sock = -1;
						continue;
					}
					rtotal_size += read_size;
					//Send the message back to client
					int index =0;
					while (read_size ){
						ssize_t m = 0;
						while ( (m = cheap_claim_spaces ( admin2, (volatile void **)&(tok[0]), read_size )) == 0 ) {}
						for(int i = 0; i < m ; i++ ) {
							int offset = ((uint32_t)tok[i])-0x000C0000+ cheapoff;
							(((volatile char *)ocm)[offset]) = client_message[index];
							index++;
							read_size -=1;
						}
						cheap_release_tokens (admin2, m);
					}
				}
			}
		}
#if 1
		clock_gettime ( CLOCK_REALTIME, &stop);
		int diff = stop.tv_sec-start.tv_sec;
		if ( start.tv_nsec > stop.tv_nsec ) diff--;
		printf("%lf\n", (rtotal_size/(double)diff)/(1024*1024));
		printf("%lf\n", (wtotal_size/(double)diff)/(1024*1024));
#endif
	}
	printf ("-Closing socket\n");
	close ( socket_desc );
	if ( ocm) {
		munmap ( (void *)ocm, OCM_SIZE );
	}
	close ( memf );
	return 0;
}
