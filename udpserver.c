/* udp_server.c */
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 112 //80+16+16 changed from 1 K

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 45054

// from https://stackoverflow.com/questions/7863499/conversion-of-char-to-binary-in-c/
int printstringasbinary(char* s, int position )
{
    // A small 9 characters buffer we use to perform the conversion
    char output[9];

    // for the loop...
    int i = position;

    // Until the first character pointed by s is not a null character
    // that indicates end of string...
    for (i; i<position+2; i++) 
//    while (*s)
    {
        // Convert the first character of the string to binary using itoa.
        // Characters in c are just 8 bit integers, at least, in noawdays computers.
        itoa(*s, output, 2);

        // print out our string and let's write a new line.
        puts(output);

        // we advance our string by one character,
        // If our original string was "ABC" now we are pointing at "BC".
        ++s;
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
      char * data;
   };

   // ask user for the timeout value as n = 1-10, with the timeout = 10^n

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

   struct timeval tv;
   tv.tv_sec = 0;
   tv.tv_usec = 100000;
   if (setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      perror("Timout code Error");
   }


   /* wait for incoming messages in an indefinite loop */

   printf("Waiting for incoming messages on port %hu\n\n", 
                           server_port);

   client_addr_len = sizeof (client_addr);

   bytes_recd = recvfrom(sock_server, &sentence, STRING_SIZE, 0,
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
      if (debug == 1) {
         printf("Received file request sentence:\n");
         printf("%s", sentence);
         printf("\nwith length %d\n\n", bytes_recd);
      }
   }

   struct packetOLove payTheLoad;
   // get the goods!
   // TODO convert from internet numbers to my numbers
   payTheLoad.sequenceNumber = sentence[0] << 8;
   payTheLoad.sequenceNumber += sentence[1];
   payTheLoad.count = sentence[2] << 8;
   payTheLoad.count += sentence[3];
   for(i=4; i<(sizeof(sentence)-1); i++) {
      payTheLoad.data[i-4]= sentence[i];
   }

   /* get filename */
//   strncpy(file_name, rec_message, 100); // do we need this?
   file_name = payTheLoad.data;

   // open file
   fp = fopen(file_name, "r");
   if (fp == NULL) {
      perror("No file of requested name, hanging up on client.\n" );
      close(sock_server);
      exit(1);
   }

   //  main transmission loop
   while ((read = getline(&line, &len, fp)) > 0) {

      // TODO sequence number = 1-sequence number
      /* prepare the message to send */

      msg_len = bytes_recd;
      for (i=0; i<msg_len; i++)
         modifiedSentence[i] = toupper (sentence[i]);

      /* send message */ 
      bytes_sent = sendto(sock_server, modifiedSentence, msg_len, 0,
               (struct sockaddr*) &client_addr, client_addr_len);

      // TODO wait for and get ACK
      bytes_recd = recvfrom(sock_server, &sentence, STRING_SIZE, 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);
      printf("Received Sentence is: %s\n     with length %d\n\n",
                         sentence, bytes_recd);

      // TODO on timeout or bad ACK, need to resend
   }
}
/* udp_server.c */
