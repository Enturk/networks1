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

#define STRING_SIZE 112 //80+16+16 changed from 1 K

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 45054

//FIXME use count to determine length
void string2char(char* s, char* c, int length) {
   int i = 0;
   if (sizeof(s)>sizeof(c)) {
      perror("string2char error: end container too small\n");
      exit(1);
   }

   for (i; i<length; i++) {
      //if (c[i] == '\0') break;
      c[i] = s[i];
   }
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


int main(void) {

   int sock_server;  /* Socket on which server listens to clients */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   char sentence[STRING_SIZE];  /* receive message */
   char modifiedSentence[STRING_SIZE]; /* send message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   unsigned int i;  /* temporary loop variable */

   int debug = 1; //TODO change to 0 before final run
 
   FILE * fp;
   char * line = NULL;
   char file_name[0x100];
   size_t len = 0;

   short packet_count = 0;
   short data_start = 0;

   struct packetOLove{
      int sequenceNumber;
      int count;
      char data[80];
   };

   int totalPackets = 0;
   int totalCount = 0;
   int retransmits = 0;
   int retransdata = 0;
   int ACKcount = 0;
   int TOcount = 0;

   // ask user for the timeout value as n = 1-10, with the timeout = 10^n

   struct timeval tv;
   tv.tv_sec = 0;
   tv.tv_usec = 10;

   int intput;
   printf("Please enter a value between 1 and 10.\n");
   scanf("%d", &intput); 
   for (i=0; i<intput; i++) {
      tv.tv_usec *= 10;
   }

   if (setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      perror("Timout code Error");
      exit(1);
   }

   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Server: can't open datagram socket\n");
      exit(1);
   }

   /* initialize server address information */

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_UDP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address\n");
      close(sock_server);
      exit(1);
   }

   struct packetOLove getTheLoad;

   /* wait for incoming messages in an indefinite loop */

   printf("Waiting for incoming messages on port %hu\n\n", 
                           server_port);

   client_addr_len = sizeof (client_addr);

   // try getTheLoad instead of sentence
   bytes_recd = recvfrom(sock_server, &getTheLoad, sizeof(getTheLoad), 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);
   printf("Received Sentence is: %s\n     with length %d\n\n",
                         sentence, bytes_recd);

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

      if (debug == 1) {
         printf("Received file request sentence:\n");
         printf("%s", sentence);
         printf("\nwith length %d\n\n", bytes_recd);
      }
   }

   // TODO ACKit

   // open file
   fp = fopen(file_name, "r");
   if (fp == NULL) {
      perror("No file of requested name, hanging up on client.\n" );
      close(sock_server);
      exit(1);
   }

   // sending struct
   struct packetOLove payTheLoad;
   payTheLoad.sequenceNumber = 1;

   //  main transmission loop
   while (( getline(&line, &len, fp)) > 0) {

      /* prepare the message to send */
      payTheLoad.sequenceNumber = htons(1 - payTheLoad.sequenceNumber);
      payTheLoad.count = htons(totalCount+strlen(line));
      strncpy(payTheLoad.data, line, strlen(line));

      /* send message */ 
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
         printf("Packet %d transmitted ", totalPackets);
         len = sizeof(line);
         printf("with %d data bytes\n", len);
      }

      // wait for and get ACK
      bytes_recd = recvfrom(sock_server, &getTheLoad, sizeof(getTheLoad), 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);
      ACKcount++;

      // TODO implement packet or ACK loss

      // TODO on timeout
      //printf("Timeout expired for packet numbered %d\n", (totalPackets %2));
      //TOcount++;
      // on timeout or bad ACK, need to resend
      while (getTheLoad.sequenceNumber != payTheLoad.sequenceNumber) { // TODO add timeout condition
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


         bytes_recd = recvfrom(sock_server, &getTheLoad, sizeof(getTheLoad), 0,
               (struct sockaddr *) &client_addr, &client_addr_len);
         ACKcount++;
         // TODO need another timeout checker here?
       
      }

      printf("ACK %d received\n", (totalPackets % 2));
      ACKcount++;
      totalPackets++;
      totalCount += strlen(line);
   }

   // TODO send EOM packet
   // TODO prep payload
   payTheLoad.sequenceNumber = htons(totalPackets % 2);
   payTheLoad.count = htons(0);
   memset(payTheLoad.data, 0, sizeof(payTheLoad.data));
   len = sizeof(payTheLoad.data);

   // send it out...
   bytes_sent = sendto(sock_server, &payTheLoad, sizeof(payTheLoad), 0,
         (struct sockaddr*) &client_addr, client_addr_len);
   printf("End of Transmission Packet with sequence number %d ", totalPackets % 2);
   printf("transmitted with %d data bytes\n", len);

   // clean up a bit
   fclose(fp);
   close(sock_server);

   printf("Statistics:\n");
   printf("Number of data packets transmitted (initial transmission only): %d\n", totalPackets);
   printf("Total number of data bytes transmitted (this should be the sum of the count fields of all transmitted packets when transmitted for the first time only): %d\n", totalCount);
   printf("Total number of retransmissions: %d\n", retransmits);
   printf("Total number of data packets transmitted (initial transmissions plus retransmissions): %d\n", totalCount + retransdata);
   printf("Number of ACKs received: %d\n", ACKcount);
   printf("Count of how many times timeout expired: %d\n", TOcount);
}
/* udp_server.c */
