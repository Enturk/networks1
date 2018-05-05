/* udp_client.c */
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <time.h>

#define STRING_SIZE 112

int main(void) {

	int sock_client; /* Socket used by client */

	struct sockaddr_in client_addr; /* Internet address structure that
	 stores client address */
	unsigned short client_port; /* Port number used by client (local port) */

	struct sockaddr_in server_addr; /* Internet address structure that
	 stores server address */
	struct hostent * server_hp; /* Structure to store server's IP
	 address */
	char server_hostname[STRING_SIZE]; /* Server's hostname */
	unsigned short server_port; /* Port number used by server (remote port) */

	char sentence[STRING_SIZE]; /* send message */
	char receivedSentence[STRING_SIZE]; /* receive message */
	unsigned int msg_len; /* length of message */
	int bytes_sent, bytes_recd; /* number of bytes sent or received */

	int timeout;

	char fileName[];

	int expectedSequenceNumber = 1;

	struct packet {
		int sequenceNumber;
		int count;
		char data[];
	};

	struct packet recdPacket;
	recdPacket.count = -1; 		//in case default value is 0

	struct Ack {
		int ack;
	};

	struct Ack recdACK = 0;

	int timeoutHolder;
	printf("Please enter a timeout value between 1 and 10\n");
	scanf("%s", &timeoutHolder);
	timeout = pow(10, timeoutHolder);

	/* open a socket */

	if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("Client: can't open datagram socket\n");
		exit(1);
	}

	/* Note: there is no need to initialize local client address information
	 unless you want to specify a specific local port.
	 The local address initialization and binding is done automatically
	 when the sendto function is called later, if the socket has not
	 already been bound.
	 The code below illustrates how to initialize and bind to a
	 specific local port, if that is desired. */

	/* initialize client address information */

	client_port = 0; /* This allows choice of any available local port */

	/* Uncomment the lines below if you want to specify a particular
	 local port: */
	/*
	 printf("Enter port number for client: ");
	 scanf("%hu", &client_port);
	 */

	/* clear client address structure and initialize with client address */
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
	 any host interface, if more than one
	 are present */
	client_addr.sin_port = htons(client_port);

	/* bind the socket to the local client port */

	if (bind(sock_client, (struct sockaddr *) &client_addr, sizeof(client_addr))
			< 0) {
		perror("Client: can't bind to local address\n");
		close(sock_client);
		exit(1);
	}

	/* end of local address initialization and binding */

	/* initialize server address information */

	printf("Enter hostname of server: ");
	scanf("%s", server_hostname);
	if ((server_hp = gethostbyname(server_hostname)) == NULL) {
		perror("Client: invalid server hostname\n");
		close(sock_client);
		exit(1);
	}
	printf("Enter port number for server: ");
	scanf("%hu", &server_port);

	/* Clear server address structure and initialize with server address */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy((char *) &server_addr.sin_addr, server_hp->h_addr,
			server_hp->h_length);
	server_addr.sin_port = htons(server_port);

	/* Prepare file name packet */

	printf("Please enter the name of the file to be transferred: \n");
	scanf("%c", fileName);
	struct packet fileNamePacket;
	fileNamePacket.count = strlen(fileName);
	fileNamePacket.sequenceNumber = 0;
	fileNamePacket.data = fileName;

	/* Send file name packet */

	bytes_sent = sendto(sock_client, &fileNamePacket, sizeof(fileNamePacket), 0,
			(struct scokaddr *) &server_addr, sizeof(server_addr));

	if (bytes_sent == -1) {
		printf("check line 133");
	}

	/* receive ack for file name packet */

	int msec = 0;

	while (msec < timeout) {
		clock_t before = clock();
		do {
			bytes_recd = recvfrom(sock_client, &recdACK, sizeof(recdACK), 0,
					(struct sockaddr *) 0, (int *) 0);

			if (recdACK.ack == 0) {

				break;
			}

			clock_t difference = clock() - before;
			msec = difference * 1000 / CLOCKS_PER_SEC;
		} while (msec < timeout);

		if (msec >= timeout && recdACK.ack == 1) {
			msec = 0;
		}
	}

	/* Open file to write to */

	FILE *pFile;

	pFile = fopen("out.txt", "a");
	if (pFile == NULL) {
		perror("Error opening file.");
		close(sock_client);
		exit(1);
	}

	/*receive packets from server */

	while (recdPacket.count != 0) {

		bytes_recd = recvfrom(sock_client, &recdPacket, sizeof(recdPacket),
							0, (struct sockaddr *) 0, (int *) 0);

		if (recdPacket.sequenceNumber == expectedSequenceNumber) {
			recdPacket.count = (ntohs(recdPacket.count));
			recdPacket.sequenceNumber = ntohs(recdPacket.sequenceNumber);
			msg_len = recdPacket.count;
		}
		else {
			recdPacket = NULL;
			continue;
		}


		/* append data to file*/

		for(int i = 0; i < msg_len; i++) {
			fprintf(pFile,"%c", receivedSentence[i]);
		}

		/* end of transmission packet */

		if(recdPacket.count == 0) {
			printf("End of transmission");
		}

	}

		/* close file */

		fclose(pFile);


		/* close the socket */

		close(sock_client);
	}
