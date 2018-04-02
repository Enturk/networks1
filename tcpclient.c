/* tcp_ client.c */ 
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */     

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, connect, send, and recv */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 80

int main(void) {

   int sock_client;  /* Socket used by client */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE] = "cisc450.cis.udel.edu"; /* Server's hostname */
   unsigned short server_port = 45054;  /* Port number used by server (remote port) */

   char sentence[STRING_SIZE];  /* send message */
   char receivedSentence[STRING_SIZE]; /* receive message */
   unsigned int msg_len;  /* length of message */                      
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
 
   char packetRecd[STRING_SIZE];	// recieved packet
 
   short debug = 1; // TODO change to 0 when done
   struct packetHeader { // to use with pointers when receiving headers
      int sequenceNumber;
      int count;
   };

   int dataSize = 0; // for end-of-transmission report
   int packetQty = 0; 

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
   scanf("%s", sentence);
   msg_len = strlen(sentence) + 1;

   // prep header
   struct packetHeader fileRequestHeader; 
   fileRequestHeader.sequenceNumber = 0;
   fileRequestHeader.count = sizeof(sentence);
   
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
      close(sock_servver);
      exit(1); 
   }

   while (catchFileHeader.sequenceNumber != 0){

	//TODO use net-to-host conversion?
      /* get header response from server */  
      bytes_recd = recv(sock_client, &catchFileHeader, sizeof(catchFileHeader), 0); 
      msglen=catchFileHeader.count;

	// error checking
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
            printf("Received header packet number field is: %d\n", catchFileHeader.sequence.Number);
            printf("Received header data count field is: %d\n\n", msg_len);
         }

	// receive data from server
      bytes_recd = recv(sock_client, receivedSentence, STRING_SIZE, 0)

	//TODO error checking

	// append data to file
      for (i=0; i<msglen; i++) {
        fprintf(pFile, "%s", receivedSentence[i]);
      }
      fprintf(pFile, "\n")

	// final report tracking
      packetQty++;
      dataSize+=msg_len;
   }

   /* close the things */
   fclose(pFile);
   close (sock_client);
}
