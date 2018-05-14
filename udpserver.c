/* udp_server.c */
/* Programmed by Adarsh Sethi, Nazim Karaca, & Timothy Louie */
/* February 21, 2018 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#include <math.h>           /* for power */
#include <string.h>         /* for the love of poetry... */
#include <time.h> 	    // for timeout
#define STRING_SIZE 112 //80+16+16 changed from 1 K
/* SERV_UDP_PORT is the port number on which the server listens for
 incoming messages from clients. You should change this to a different
 number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 45054

//FIXME use count to determine length
void string2char(char* s, char* c, int length) {
	int i = 0;
	if (sizeof(s) > sizeof(c)) {
		perror("string2char error: end container too small\n");
		exit(1);
	}

	for (i; i < length; i++) {
		//if (c[i] == '\0') break;
		c[i] = s[i];
	}
}

int simulateLoss(double packetLossRate) {
	srand(time(NULL));
	double randomNumber = rand() % 1;

	if (randomNumber < packetLossRate) {
		return 1;
	} else {
		return 0;
	}
}

int simulateACKLoss(double ACKLossRate) {
	srand(time(NULL));
	double randomNumber = rand() % 1;

	if (randomNumber < ACKLossRate) {
		return 1;
	}

	else {
		return 0;
	}
}

int main(void) {

	int sock_server; /* Socket on which server listens to clients */

	struct sockaddr_in server_addr; /* Internet address structure that
	 stores server address */
	unsigned short server_port; /* Port number used by server (local port) */

	struct sockaddr_in client_addr; /* Internet address structure that
	 stores client address */
	unsigned int client_addr_len; /* Length of client address structure */

	char sentence[STRING_SIZE]; /* receive message */
	char modifiedSentence[STRING_SIZE]; /* send message */
	unsigned int msg_len; /* length of message */
	int bytes_sent, bytes_recd; /* number of bytes sent or received */
	unsigned int i; /* temporary loop variable */

	int debug = 1; //TODO change to 0 before final run

	FILE * fp;
	char * line = NULL;
	char file_name[0x100];
	size_t len = 0;

	short packet_count = 0;
	short data_start = 0;

	struct packetOLove {
		short int sequenceNumber;
		short int count;
		char data[80];
	};

	struct ACK {
		short int ack;
	};

	struct ACK recdACK;

	int expectedSequenceNumber = 0;

	int microsec = 0;
	clock_t before = clock();

	int totalPackets = 0;
	int totalCount = 0;
	int retransmits = 0;
	int retransdata = 0;
	int ACKcount = 0;
	int TOcount = 0;

	// ask user for the timeout value as n = 1-10, with the timeout = 10^n
	printf(
			"Please enter a value between 1 and 10 for the power of the timeout\n");
	int intput;
	scanf("%d", &intput);
	int timeout = pow(10, intput);

	// open a socket
	if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("Server: can't open datagram socket\n");
		exit(1);
	}

	/* initialize server address information */

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
	 any host interface, if more than one
	 are present */
	server_port = SERV_UDP_PORT; /* Server will listen on this port */
	server_addr.sin_port = htons(server_port);

	/* bind the socket to the local server port */

	if (bind(sock_server, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		perror("Server: can't bind to local address\n");
		close(sock_server);
		exit(1);
	}

	struct packetOLove getTheLoad;

	/* wait for incoming messages in an indefinite loop */

	printf("Waiting for incoming messages on port %hu\n\n", server_port);

	client_addr_len = sizeof(client_addr);

	// try getTheLoad instead of sentence
	bytes_recd = recvfrom(sock_server, &getTheLoad, sizeof(getTheLoad), 0,
			(struct sockaddr *) &client_addr, &client_addr_len);

	if (debug == 1) {
		printf("Received Sentence is: %s\n", getTheLoad.data);
		printf("with length %d\n\n", ntohs(getTheLoad.count)); // this works, apparently

	}
	/*   struct timeval tv;
	 tv.tv_sec = 0;
	 tv.tv_usec = 1000; // one millisecond = 1000 microseconds!
	 if (debug == 1) printf("You entered %d as the timeout power, and tv_usec is now %d\n", intput, tv.tv_usec);
	 for (i=0; i<intput; i++) {
	 tv.tv_usec *= 10;
	 if (debug == 1) printf("tv_usec is now %d\n", tv.tv_usec);
	 }
	 if (setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	 printf("Timout expired for packet number zero\n");
	 }*/

	// error checking always...
	if (bytes_recd < 0) {
		perror("Filename data recv error. Ooops?");
		close(sock_server);
		exit(1);

	} else if (bytes_recd == 0) {
		perror("Filename data recv got end-of-file return\n");
		close(sock_server);
		exit(1);

	} else { // bytes_recd > 0
		// get the goods!
		/* this is probably unnecessary
		 getTheLoad.sequenceNumber = ntohs(sentence[0]) << 8;
		 getTheLoad.sequenceNumber += ntohs(sentence[1]);
		 getTheLoad.count = ntohs(sentence[2]) << 8;
		 getTheLoad.count += ntohs(sentence[3]);
		 for(i=0; i<payTheLoad.count; i++) {
		 payTheLoad.data[i]= sentence[i+4];
		 filename[i] = sentence[i+4];
		 } */
		getTheLoad.sequenceNumber = ntohs(getTheLoad.sequenceNumber);
		getTheLoad.count = ntohs(getTheLoad.count);
		strncpy(file_name, getTheLoad.data, getTheLoad.count);

		// or, instead of the above for loop:
		// strncpy(file_name, rec_message, 100);
		// or:
		/* get filename */
		// string2char(&(payTheLoad.data), filename, paytheLoad.count);
	}

	// open file
	fp = fopen(getTheLoad.data, "r");
	if (fp == NULL) {
		perror("No file of requested name, hanging up on client.\n");
		close(sock_server);
		exit(1);
	}

	struct packetOLove payTheLoad;

	//  main transmission loop
	while ((getline(&line, &len, fp)) > 0) {

		memset(&payTheLoad.data, 0, sizeof(payTheLoad.data));

		/* prepare the message to send */
		payTheLoad.sequenceNumber = htons(expectedSequenceNumber);
		// payTheLoad.count = htons(totalCount+strlen(line));
		strncpy(payTheLoad.data, line, strlen(line));
		payTheLoad.count = htons(strlen(payTheLoad.data));

		retransmits--; //to undo the ++ at the end of this loop

		// unACKed send loop
		//     while (ntohs(payTheLoad.sequenceNumber) != expectedSequenceNumber) {

		/* send message */

		//setting timeout

		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;

		setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO,
				(const void *) &timeout, sizeof(timeout));

		bytes_sent = sendto(sock_server, &payTheLoad, sizeof(payTheLoad), 0,
				(struct sockaddr*) &client_addr, client_addr_len);

		// error checking
		if (bytes_sent <= 0) {
			perror("Send error");
			retransmits++;
			retransdata += sizeof(line);
			TOcount++;
			continue;
			/* bytes_sent = sendto(sock_server, &payTheLoad, sizeof(payTheLoad), 0,           wait for timeout
			 (struct sockaddr*) &client_addr, client_addr_len);

			 if (bytes_sent < 0)
			 perror("Resend failed, giving up.\n");
			 else {
			 printf("Resend successful. Party on.\n");
			 }*/
		}

		else {
			printf("Packet %d transmitted ", ntohs(payTheLoad.sequenceNumber));
			// len = sizeof(line);
			printf("with %d data bytes\n", ntohs(payTheLoad.count));
		}

		bytes_recd = recvfrom(sock_server, &recdACK, sizeof(getTheLoad), 0,
				(struct sockaddr *) &client_addr, &client_addr_len);

		if (bytes_recd >= 0 && ntohs(recdACK.ack) == expectedSequenceNumber) {
			expectedSequenceNumber = 1 - expectedSequenceNumber;
			continue;
		}

		else {
			continue;
		}
	}

	/*
	 // wait for and get ACK with timeout
	 bytes_recd = recvfrom(sock_server, &getTheLoad, sizeof(getTheLoad), 0,
	 (struct sockaddr *) &client_addr, &client_addr_len);
	 if (bytes_recd > 0 && ntohs(getTheLoad.sequenceNumber) == ntohs(payTheLoad.sequenceNumber)) {
	 ACKcount++;
	 printf("ACK %d received\n", ntohs(getTheLoad.sequenceNumber));
	 break;
	 }
	 clock_t difference = clock() - before;
	 microsec = difference * 1000000 / CLOCKS_PER_SEC;
	 if (microsec >= timeout && ntohl(recdACK.ack) == 1) {
	 microsec = 0;
	 }*/

	/*if (setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	 printf("Timeout expired for packet numbered %d\n", ntohs(payTheLoad.sequenceNumber));
	 TOcount++;
	 }*/

	// resend due to pad ack or timeout
	/* while (getTheLoad.sequenceNumber != payTheLoad.sequenceNumber) { // no timeout check needed
	 retransmits++;
	 retransdata += sizeof(line);
	 bytes_sent = sendto(sock_server, &payTheLoad, sizeof(payTheLoad), 0,
	 (struct sockaddr*) &client_addr, client_addr_len);
	 // error checking
	 if (bytes_sent < 0) {
	 perror("Send error, trying again... ");
	 retransmits++;
	 retransdata += sizeof(line);
	 bytes_sent = sendto(sock_server, &payTheLoad, sizeof(payTheLoad), 0,
	 (struct sockaddr*) &client_addr, client_addr_len);
	 if (bytes_sent < 0)
	 perror("Resend failed, giving up.\n");
	 else {
	 printf("Resend successful. Party on.\n");
	 }
	 } else if (debug == 1) {
	 printf("Packet %d retransmitted ", (totalPackets % 2));
	 len = sizeof(line);
	 printf("with %d data bytes\n", len);
	 }
	 // wait for and get ACK
	 bytes_recd = recvfrom(sock_server, &getTheLoad, sizeof(getTheLoad), 0,
	 (struct sockaddr *) &client_addr, &client_addr_len);
	 if (bytes_recd > 0 && ntohs(getTheLoad.sequenceNumber) == ntohs(payTheLoad.sequenceNumber)) {
	 ACKcount++;
	 printf("ACK %d received\n", ntohs(getTheLoad.sequenceNumber));
	 }
	 // timeout FIXME if sock_server doesn't work
	 if (setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	 printf("Timeout expired for packet numbered %d\n", ntohs(payTheLoad.sequenceNumber));
	 TOcount++;
	 }
	 }
	 totalPackets++;
	 totalCount += strlen(line);
	 bytes_recd = 0;
	 }
	 */
	// send EOM packet
	// prep payload
	payTheLoad.sequenceNumber = htons(totalPackets % 2);
	payTheLoad.count = htons(0);
	memset(payTheLoad.data, 0, strlen(payTheLoad.data));
	len = strlen(payTheLoad.data);

	// send it out...
	bytes_sent = sendto(sock_server, &payTheLoad, sizeof(payTheLoad), 0,
			(struct sockaddr*) &client_addr, client_addr_len);
	printf("End of Transmission Packet with sequence number %d ",
			totalPackets % 2);
	printf("transmitted with %zu data bytes\n", len);

	// clean up a bit
	fclose(fp);
	close(sock_server);

	printf("Statistics:\n"); // yeah, those.
	printf(
			"Number of data packets transmitted (initial transmission only): %d\n",
			totalPackets);
	printf(
			"Total number of data bytes transmitted (this should be the sum of the count fields of all transmitted packets when transmitted for the first time only): %d\n",
			totalCount);
	printf("Total number of retransmissions: %d\n", retransmits);
	printf(
			"Total number of data packets transmitted (initial transmissions plus retransmissions): %d\n",
			totalCount + retransdata);
	printf("Number of ACKs received: %d\n", ACKcount);
	printf("Count of how many times timeout expired: %d\n", TOcount);
}
/* udp_server.c */
