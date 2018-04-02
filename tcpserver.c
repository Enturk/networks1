/* tcpserver.c */
/* Programmed by Adarsh Sethi & Nazim Karaca*/
/* February 21, 2018 & April 1, 2018 */    

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, bind, listen, accept */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024   

/* SERV_TCP_PORT is the port number on which the server listens for
   incoming requests from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_TCP_PORT 45054
#define _GNU_SOURCE

int main(void) {

   int sock_server;  /* Socket on which server listens to clients */
   int sock_connection;  /* Socket on which server exchanges data with client */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned int server_addr_len;  /* Length of server address structure */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   char rec_message[STRING_SIZE];  /* receive message */
   short rec_packet_num; // first received header containing packet sequence numebr
   short rec_data_size; // second received header containing packet data size
   char send_message[STRING_SIZE]; /* send message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   unsigned int i;  /* temporary loop variable */
   short net_number; // for use with htons
   long THIRTYTWO_ZEROS = 0x00000000; // for end-of-transmission packet header legibility
   unsigned int total_data = 0; // to produce the end-of-run statistic

   FILE * fp;
   char * line = NULL;
   size_t len = 0;
   ssize_t read;
   char file_name[0x100];
   short packet_count = 1;
   short data_start = 4;
   
   char debug = 1; // debugging variable, TODO set to 0 when done

   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("Server: can't open stream socket");
      exit(1);                                                
   }

   /* initialize server address information */
    
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */ 
   server_port = SERV_TCP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address");
      close(sock_server);
      exit(1);
   }                     

   /* listen for incoming requests from clients */

   if (listen(sock_server, 50) < 0) {    /* 50 is the max number of pending */
      perror("Server: error on listen"); /* requests that will be queued */
      close(sock_server);
      exit(1);
   }
   printf("I am here to listen ... on port %hu\n\n", server_port);
  
   client_addr_len = sizeof (client_addr);

   /* wait for incoming connection requests in an indefinite loop */

   for (;;) {

      sock_connection = accept(sock_server, (struct sockaddr *) &client_addr, 
                                         &client_addr_len);
                     /* The accept function blocks the server until a
                        connection request comes from a client */
      if (sock_connection < 0) {
         perror("Server: accept() error\n"); 
//         close(sock_server);
//         exit(1);
      }
 
      /* receive the message */

      bytes_recd = recv(sock_connection, rec_message, STRING_SIZE, 0);

      // error checking as recommended by Adarsh Sethi
      if (bytes_recd < 0) {
         perror("Recv error. Good luck with that...\n");
         close(sock_server);
         exit(1);

      } else if (bytes_recd == 0) {
         perror("Recv got end-of-file return\n");
         close(sock_server);
         exit(1);

      } else { // bytes_recd > 0
         if (debug == 1) {
            printf("Received Sentence is:\n");
            printf("%s", rec_message);
            printf("\nwith length %d\n\n", bytes_recd);
         }

        /* get filename */
        for (i=data_start; i<sizeof(rec_message); i++)
           file_name[i-data_start] = rec_message[i];
        i=0; // reset i to be environmentally friendly
        if (debug == 1)
           printf("Requsted file is %s\n", file_name);

        /* prepare the message to send */
         fp = fopen(file_name, "r");
         if (fp == NULL) {
            perror("No file of requested name, hanging up on client.\n" );
            close(sock_server);
            exit(1);
         }

        /* make header */
         while ((read = getline(&line, &len, fp)) != -1) {
             //maybe break line into multiple messages if line is too long
             if (strlen(line)>STRING_SIZE) {
                perror("Next line is too long and will be skipped.\n");
                continue;
             }
 	     msg_len = strlen(line);                 

	// header 1: packet sequence number
            net_number = htons(packet_count);
            send_message[0] = net_number >> 8; //Most significant byte
            send_message[1] = net_number & 0x00FF; //Least significant byte

	// header 2: data size
            net_number = htons(strlen(line));
            send_message[2] = net_number >> 8; //Most significant byte
            send_message[3] = net_number & 0x00FF; //Least significant pyte

	// send headers
            bytes_sent = send(sock_connection, send_message, msg_len, 0);

	// error checking
            if (bytes_sent < 0) {
               perror("Send error, trying again... ");
               bytes_sent = send(sock_connection, send_message, msg_len, 0);
               if (bytes_sent < 0)
                  perror("Resend failed, giving up.\n");
               else {
                  printf("Resend successful. Party on.\n");
               }
            } else if (debug == 1) {
               printf("Header packet for %d transmitted ", packet_count);
               printf("with %d data bytes\n", bytes_sent);
            }

	// housekeeping: clean up send_message
            memset(send_message, 0, sizeof(send_message));

	// packet data
            for (i=0; i<msg_len; i++)
               send_message[i] = line[i];
            msg_len = sizeof(send_message);

         /* send data */ 
            bytes_sent = send(sock_connection, send_message, msg_len, 0);

        // error checking
            if (bytes_sent < 0) {
               perror("Send error, trying again... ");
               bytes_sent = send(sock_connection, send_message, msg_len, 0);
               if (bytes_sent < 0)
                  perror("Resend failed, giving up.\n");
               else {
                  printf("Resend successful. Party on.\n");
                  printf("Packet %d transmitted ", packet_count);
                  printf("with %d data bytes\n", bytes_sent);
                  total_data += bytes_sent;
               }
            } else {
               printf("Packet %d transmitted ", packet_count);
               printf("with %d data bytes\n", bytes_sent);
               total_data += bytes_sent;
            }

	// housekeeping: clean up send_message
            memset(send_message, 0, sizeof(send_message));
            packet_count++;
         }

	 // close file and free up line memory (probably unnecessary, but man says best do it)
         fclose(fp);
         if (line)
            free(line);

	// send end-of-transmission packet: 4 bytes of all zeros
         bytes_sent = send(sock_connection, htonl(THIRTYTWO_ZEROS), sizeof(THIRTYTWO_ZEROS), 0);

        // error checking
         if (bytes_sent < 0) {
            perror("Send error, trying again... ");
            bytes_sent = send(sock_connection, htonl(THIRTYTWO_ZEROS), sizeof(THIRTYTWO_ZEROS), 0);
            if (bytes_sent < 0)
               perror("Resend failed, giving up.\n");
            else {
               printf("Resend successful. Party on.\n");
               printf("End of Transmission Packet with sequence number %d transmitted ", THIRTYTWO_ZEROS);
               printf("with 0 data bytes\n\n");
            }
         } else {
            printf("End of Transmission Packet with sequence number %d transmitted ", THIRTYTWO_ZEROS);
            printf("with 0 data bytes\n\n");
         }

      }

      /* close the socket */
      close(sock_connection);

   } 

   // end of program statistics
   printf("Number of data packets transmitted: %d\n", packet_count);
   printf("Total number of data bytes transmitted: %d\n", total_data);

}
