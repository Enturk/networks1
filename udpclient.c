/* udp_client.c */
/* Programmed by Adarsh Sethi, Timothy Louie, Nazim Karaca */
/* February 21, 2018 */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <time.h>
#include <math.h>

#define STRING_SIZE 112

int simulateLoss(double packetLossRate);
int simulateACKLoss(double ACKLossRate);

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

	char fileName[STRING_SIZE];

	int expectedSequenceNumber = 0;

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

	struct Ack recdACK;
	recdACK.ack = 0;

	struct Ack sentACK;
	
	struct fileNamePacket {
		char fileName[STRING_SIZE];
	}


	int receivedDataPackets = 0;
	int dataBytesReceived = 0;
	int duplicatePacketsReceived = 0;
	int dataPacketsDropped = 0;
	int totalDataPacketsReceived = 0;
	int ACKsTransmitted = 0;
	int ACKsGeneratedDropped = 0;
	int ACKsGenerated = 0;


	int timeoutHolder;
	printf("Please enter a timeout value between 1 and 10\n");
	scanf("%d", &timeoutHolder);
	timeout = pow(10, timeoutHolder);


	double packetLossRate;
	printf("Please enter a packet loss rate value between 0 and 1\n");
	scanf("%lf", &packetLossRate);

	double ACKLossRate;
	printf("Please enter a ACK loss rate value between 0 and 1\n");
	scanf("%lf", &ACKLossRate);


	int i; // for loops

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

	//printf("Enter hostname of server: ");
	//scanf("%s", server_hostname);
	server_hostname = "cisc450.cis.udel.edu"
	
	if ((server_hp = gethostbyname(server_hostname)) == NULL) {
		perror("Client: invalid server hostname\n");
		close(sock_client);
		exit(1);
	}
	//printf("Enter port number for server: ");
	//scanf("%hu", &server_port);

	server_port = 45054
	
	/* Clear server address structure and initialize with server address */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy((char *) &server_addr.sin_addr, server_hp->h_addr,
			server_hp->h_length);
	server_addr.sin_port = htons(server_port);

	/* Prepare file name packet */

	printf("Please enter the name of the file to be transferred: \n");
	scanf("%c", fileNamePacket.fileName[STRING_SIZE]);
	/* Send file name packet */
	// FIXME scodaddr is problem
	bytes_sent = sendto(sock_client, &fileNamePacket, sizeof(fileNamePacket), 0,
			(struct sockaddr *) &server_addr, sizeof(server_addr));

	if (bytes_sent == -1) {
		printf("check line 133");
	}



	/* receive ack for file name packet */

/*	int microsec = 0;
	while (microsec < timeout) {
		clock_t before = clock();
		do {
			bytes_recd = recv(sock_client, &recdACK, sizeof(recdACK), 0);
			if (ntohl(recdACK.ack) == 0) {
				break;
			}
			clock_t difference = clock() - before;
			microsec = difference * 1000000 / CLOCKS_PER_SEC;
		} while (microsec < timeout);
		if (microsec >= timeout && ntohl(recdACK.ack) == 1) {
			microsec = 0;
		}
	}
*/
	/* Open file to write to */

	FILE *pFile;

	pFile = fopen("out.txt", "a");
	if (pFile == NULL) {
		perror("Error opening file.");
		close(sock_client);
		exit(1);
	}

	/*receive packets from server */

	while (ntohl(recdPacket.count) != 0) {

		bytes_recd = recv(sock_client, &recdPacket, sizeof(recdPacket), 0);
		recdPacket.sequenceNumber = ntohl(recdPacket.sequenceNumber);
		recdPacket.count = ntohl(recdPacket.count);

		if (recdPacket.count != 0) {

			if (simulateLoss(packetLossRate) == 1) {
				printf("Packet %d lost", recdPacket.sequenceNumber);
				dataPacketsDropped++;
				continue;
			}

			else {

				if (recdPacket.sequenceNumber == expectedSequenceNumber) {

					recdPacket.count = (ntohs(recdPacket.count));
					recdPacket.sequenceNumber = ntohs(recdPacket.sequenceNumber);
					msg_len = recdPacket.count;

					receivedDataPackets++;
					totalDataPacketsReceived++;
					dataBytesReceived = dataBytesReceived + msg_len;

					printf("Packet %d received with %d data bytes", recdPacket.sequenceNumber,recdPacket.count);

					/* append data to file*/

					for (i = 0; i < msg_len; i++) {
						fprintf(pFile, "%c", receivedSentence[i]);
					}

					sentACK.ack = htonl(expectedSequenceNumber);
					ACKsGenerated++;

					if(simulateACKLoss(ACKLossRate) == 0) {
						ACKsTransmitted++;
						sendto(sock_client, &sentACK, sizeof(sentACK), 0,(struct sockaddr *) &server_addr, sizeof(server_addr));
						printf("Ack %d transmitted", sentACK.ack);
						expectedSequenceNumber = 1 - expectedSequenceNumber;
					}

					else {
						printf("ACK %d lost", expectedSequenceNumber);
						ACKsGeneratedDropped++;
						expectedSequenceNumber = 1 - expectedSequenceNumber;
					}

				}

				else {

					printf("Duplicate packet %d received with %d data bytes", recdPacket.sequenceNumber,recdPacket.count);

					totalDataPacketsReceived++;
					duplicatePacketsReceived++;

					sentACK.ack = htonl(1 - expectedSequenceNumber);
					ACKsGenerated++;

					if(simulateACKLoss(ACKLossRate) == 0) {
						sendto(sock_client, &sentACK, sizeof(sentACK), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));

						ACKsTransmitted++;
					}

					else {
						printf("ACK %d lost", 1- expectedSequenceNumber);
						ACKsGeneratedDropped++;
					}
					continue;
				}
			}
		}

		/* end of transmission packet */

		else {                                  //recdpacket.count == 0
			printf("End of transmission Packet with sequence number %d received with %d data bytes", recdPacket.sequenceNumber,recdPacket.count);
		}

	}

	/* close file */

	fclose(pFile);

	/* close the socket */

	close(sock_client);

	printf("Number of data packets received successfully: %d\n" , receivedDataPackets);
	printf("Total number of data bytes received which are delivered to user: %d\n",dataBytesReceived);
	printf("Total number of duplicate data packets received: %d\n", duplicatePacketsReceived);
	printf("Number of data packets received but dropped due to loss: %d\n", dataPacketsDropped);
	printf("Total number of data packets received: %d\n", totalDataPacketsReceived);
	printf("Number of ACKs transmitted without loss: %d\n", ACKsTransmitted);
	printf("Number of ACKs generated but dropped due to loss: %d\n", ACKsGeneratedDropped);
	printf("Total number of ACKs generated: %d\n", ACKsGenerated);

}

int simulateLoss(double packetLossRate) {
	srand(time(NULL));
	double randomNumber = rand() % 1;

	if(randomNumber < packetLossRate) {
		return 1;
	}
	else {
		return 0;
	}
}

int simulateACKLoss(double ACKLossRate) {
	srand(time(NULL));
	double randomNumber = rand() % 1;

	if(randomNumber < ACKLossRate) {
		return 1;
	}

	else {
		return 0;
	}
}
