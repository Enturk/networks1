/*
/* tcp_ client.c */
/* Programmed by Adarsh Sethi, Nazim Karaca, & Timothy Louie*/
/* February 21, 2018 & April 2, 2018*/

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, connect, send, and recv */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <time.h> 	    /* for timestamp in file */

#define STRING_SIZE 80

int main(void) {

   int sock_client;  /* Socket used by client */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE] = "cisc450.cis.udel.edu"; /* Server's hostname */
   unsigned short server_port = 45054;  /* Port number used by server (remote port) */

   char sentence[STRING_SIZE]="test1.txt";  /* send message TODO initialized only for output capture */ 
   char receivedSentence[STRING_SIZE]; /* receive message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */

   char packetRecd[STRING_SIZE];	// recieved packet

   short debug = 0; // TODO change to 0 when done
   struct packetHeader { // to use with pointers when receiving headers
      int sequenceNumber;
      int count;
   };

   int i; // for the for loop

   int dataSize = 0; // for end-of-transmission report
   int packetQty = 0;

   time_t rawtime; // for timestamp
   struct tm * timeinfo;
   time ( &rawtime );

   /* open a socket */

   if ((sock_client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("Client: can't open stream socket");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port
            (in which case, do it the same way as in udpclient.c).
            The local address initialization and binding is done automatically
            when the connect function is called later, if the socket has not
            already been bound. */

   /* initialize server address information */

/*   printf("Enter hostname of server: ");
   scanf("%s", server_hostname); */
   if ((server_hp = gethostbyname(server_hostname)) == NULL) {
      perror("Client: invalid server hostname");
      close(sock_client);
      exit(1);
   }

/*   printf("Enter port number for server: ");
   scanf("%hu", &server_port);
*/
   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(server_port);

    /* connect to the server */

   if (connect(sock_client, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Client: can't connect to server");
      close(sock_client);
      exit(1);
   }

   /* user interface */

   printf("Please type a filename (including the extension such as .txt or .c):\n");
   //scanf("%s", sentence); //TODO filname hardcoded above to produce output script
   printf("test1.txt"); // TODO either comment out this line or the prior one
   msg_len = strlen(sentence) + 1;

   // prep header
   struct packetHeader fileRequestHeader;
   fileRequestHeader.sequenceNumber = 0;
   fileRequestHeader.count = msg_len;

   /* send header */
   bytes_sent = send(sock_client, &fileRequestHeader, sizeof(fileRequestHeader), 0); // Sethi trick

   // error checking
   if (bytes_sent < 0) {
      perror("Send error, trying again... ");
      bytes_sent = send(sock_client, &fileRequestHeader, sizeof(fileRequestHeader), 0); // Sethi trick
      if (bytes_sent < 0)
         perror("Resend failed, giving up.\n");
      else {
         printf("Resend successful. Party on.\n");
      }
   } else if (debug == 1) {
      printf("Header packet for file name request transmitted ");
      printf("with %d data bytes\n", bytes_sent);
   }
   msg_len = fileRequestHeader.count;


   // send filename
   bytes_sent = send(sock_client, sentence, msg_len, 0);

   // error checking
   if (bytes_sent < 0) {
      perror("Send error, trying again... ");
      bytes_sent = send(sock_client, sentence, msg_len, 0);
      if (bytes_sent < 0)
         perror("Resend failed, giving up.\n");
      else {
         printf("Resend successful. Party on.\n");
      }
   } else if (debug == 1) {
      printf("Data packet for file request transmitted ");
      printf("with %d data bytes\n", bytes_sent);
   }

   // setup file receipt
   struct packetHeader catchFileHeader;
   catchFileHeader.sequenceNumber = 1;

   /* open file to write incoming crap */
   FILE *pFile;

   pFile=fopen("out.txt", "a");
   if(pFile==NULL) {
      perror("Error opening file.");
      close(sock_client);
      exit(1);
   }
   if (debug == 1) {
      timeinfo = localtime(&rawtime);
      fprintf(pFile,"\n\n\nOUTPUT OF FILE %s AT DATE & TIME: %s\n\n", sentence, asctime(timeinfo) );
   }

   if (debug == 1) printf("Reception header struct sequence is %d at packet number %d before while loop.\n", catchFileHeader.sequenceNumber, packetQty);

   while (catchFileHeader.sequenceNumber != 0){
     if (debug == 1) printf("In file reception while loop\n");
      /* get header response from server */
      bytes_recd = recv(sock_client, &catchFileHeader, sizeof(catchFileHeader), 0);
      catchFileHeader.count = ntohs(catchFileHeader.count);
      catchFileHeader.sequenceNumber = ntohs(catchFileHeader.sequenceNumber);
      msg_len=catchFileHeader.count;

      printf("packet %d is received with ", catchFileHeader.sequenceNumber );
      printf("%d bytes\n", catchFileHeader.count);


	// error checking
      if (bytes_recd < 0) {
         perror("Header recv error. Good luck with that...\n");
         close(sock_client);
         exit(1);

      } else if (bytes_recd == 0) {
         perror("Header recv got end-of-file return\n");
        // close(sock_client);
        //  exit(1);

      } else { // bytes_recd > 0
         if (debug == 1) {
            printf("Received header packet number field is: %d\n", catchFileHeader.sequenceNumber);
            printf("Received header data count field is: %d\n\n", msg_len);
         }
      }

	// receive data from server
      bytes_recd = recv(sock_client, receivedSentence, msg_len, 0);

	// error checking
      if (bytes_recd < 0) {
         perror("Data Recv error. Good luck with that...\n");
         close(sock_client);
         exit(1);

      } else if (bytes_recd == 0) {
         perror("Data recv got end-of-file return\n");

      } else { // bytes_recd > 0
         if (debug == 1) {
            printf("Received data with line: \n%s", receivedSentence);
         }
      }

	// append data to file
      if (debug == 1) // for debug
         printf("Appended this line to file:\n");
      for (i=0; i<msg_len; i++) {
        fprintf(pFile, "%c", receivedSentence[i]);
	// debug pring here
        if (debug == 1)
           printf("%c", receivedSentence[i]);

      }
	// add end of line UNNECESSARY
//      fprintf(pFile, "\n");

      if (debug == 1)
         printf("Appended this line to file:\n%s\n", receivedSentence);

      if(catchFileHeader.sequenceNumber == 0) {
    	  printf("End of Transmission Packet with sequence number %d ", packetQty);
    	  printf("received with %d data bytes\n", msg_len);
    	  packetQty--;                                      //while loop catches last packet
      }

	// final report tracking
      packetQty++;
      dataSize+=msg_len;
   }



   // final report
   printf("Number of data packets received: %d\n", packetQty);
   printf("Total number of data bytes received %d\n", dataSize);


   /* close the things */
   fclose(pFile);
   close (sock_client);
} 




