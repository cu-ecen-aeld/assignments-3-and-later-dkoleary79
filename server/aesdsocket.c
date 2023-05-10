#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> //for getaddrinfo/freeaddrinfo

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <signal.h>
#include <syslog.h>

volatile int gFinishedRunning = 0;
void intHandler( int dummy ) {
	gFinishedRunning = 1;
	return;
}

int main( int argc, char*argv[] )
{
	int retVal = 0;
	int runInDaemonMode = 0;
	const char* tmpFilePath = "/var/tmp/aesdsocketdata";

	if( argc > 1 )
	{//validate command line args
		if( strcmp(argv[1], "-d") == 0 )
			runInDaemonMode = 1;
	}

    openlog( "aesdsocket", 0, LOG_USER );

    int serverSock = socket( PF_INET, SOCK_STREAM, 0 );
	if( serverSock < 0 )
	{
		printf("!ERR: failed to open server socket\r\n");
		retVal = -1;
	}
	else
	{
		int reuseAddr = 1;
		if( setsockopt( serverSock, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr) ) != 0 )
		{
			printf("!ERR: failed to set SO_REUSEADDR on socket\r\n");
			retVal = -1;
		}
	}
	struct sigaction sigAct;
	memset( &sigAct, 0, sizeof(sigAct) );
	sigAct.sa_handler = intHandler;

	sigaction( SIGINT, &sigAct, NULL );
	sigaction( SIGTERM, &sigAct, NULL );

	if( retVal == 0 )
	{//bind socket
		struct addrinfo hints;
		struct addrinfo *result;
		memset( &hints, 0, sizeof(hints) );
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = 0;//meaning 'any'
		if( getaddrinfo( NULL, "9000", &hints, &result ) != 0 )
		{
			printf("!ERR: failed to use getaddrinfo\r\n");
			retVal = -1;
		}
		else
		{
			if( bind( serverSock, result->ai_addr, result->ai_addrlen ) != 0 )
			{
				printf("!ERR: failed to use bind\r\n");
				retVal = -1;
			}
			freeaddrinfo( result );
		}
	}

	if( retVal == 0 )
	{
		if( runInDaemonMode )
		{
			pid_t childPid = fork();
			if( childPid != 0 )
			{
				printf("Running in deamon mode\r\n");
				return 0;
			}
		}
	}

	if( retVal == 0 )
	{//listen
		if( listen( serverSock, 1 ) != 0 )//backlog of 1
		{
			printf("!ERR: failed to use listen\r\n");
			retVal = -1;
		}
	}

	if( retVal == 0 )
	while( !gFinishedRunning )
	{
		struct sockaddr clientAddr;
		socklen_t clientAddrLen = sizeof(clientAddr);
		int clientSock = accept( serverSock, &clientAddr, &clientAddrLen );
		if( clientSock >= 0 )
		{
			char clientAddrBuffer[INET6_ADDRSTRLEN];
			{//First get peer address name and print it out
				if( getnameinfo((struct sockaddr*)&clientAddr, clientAddrLen, clientAddrBuffer, sizeof(clientAddrBuffer), 0, 0, NI_NUMERICHOST) != 0 )
					sprintf(clientAddrBuffer, "UNKNOWN");
				//printf("Accepted connection from %s\r\n", buffer);
			}

			syslog(LOG_ERR, "Accepted connection from %s", clientAddrBuffer);

			int gFinishedPacket = 0;
			size_t myBuffOffset = 0;
			char* myBuff = malloc(myBuffOffset+2);
			while( (!gFinishedRunning) && (!gFinishedPacket) )
			{
				int charsRx = recv( clientSock, &(myBuff[myBuffOffset]), 1, 0 );
				if( charsRx < 0 )
				{
					if( errno != EINTR )
						printf("!ERR: in recv\r\n");
				}
				else if( charsRx > 0 )
				{
					if( myBuff[myBuffOffset] == '\n' )
					{//reached end of line/packet
						FILE * f = fopen(tmpFilePath, "a+");//NOTE: a+ means writes are always appended to end of file
						if( f == NULL )
						{
							printf("!ERR: failed to open file %s\r\n", tmpFilePath);
						}
						else
						{
							if( fwrite( myBuff, sizeof(char), (myBuffOffset+1/*+1 for the '\n' at the end*/), f ) != (myBuffOffset+1) )
								printf("!ERR: in fwrite\r\n");
							fflush( f );

							{//read the entire file and forward back to the client
								fseek( f, 0, SEEK_SET );
								size_t charsRead;

								do
								{
									charsRead = fread( myBuff, sizeof(char), myBuffOffset, f );
									myBuff[charsRead] = '\0';
									//syslog(LOG_ERR, "sending blob: %s", myBuff);
									size_t charsTx = send( clientSock, myBuff, charsRead, 0 );
									if( charsTx != charsRead )
									{
										if( errno != EINTR )
											printf("!ERR: in send\r\n");
									}
								} while( charsRead > 0 );
							}

							fclose( f );
						}
						myBuffOffset = 0;
						gFinishedPacket = 1;
					}
					else
					{//not yet reached end of line/packet
						++myBuffOffset;
						myBuff = realloc( myBuff, myBuffOffset+1 );
						if( myBuff == NULL )
						{
							printf("!ERR: in realloc\r\n");
							myBuff = realloc( myBuff, 2 );
							myBuffOffset = 1;
						}
					}
				}
			}

			if( myBuff != NULL )
			{
				free( myBuff );
				myBuff = NULL;
			}


			syslog(LOG_ERR, "Closed connection from %s", clientAddrBuffer);

			close( clientSock );
		}
	}
	syslog(LOG_ERR, "Caught signal, exiting");

	remove( tmpFilePath );

	close( serverSock );

	return retVal;
}

