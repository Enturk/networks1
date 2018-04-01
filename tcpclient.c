#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <netdb.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <winsock.h>

#define STRING_SIZE 1024

using namespace std;

int main(void) {

	int sock_client;   	//Socket used by client

	struct sockaddr_in server_addr;    // Internet address structure that stores server address

	struct hostent * server_hp;        // structure to store server's ip  address

	char server_hostname[STRING_SIZE] = "cisc450.cis.udel.edu";		//server host name

	unsigned short server_port = 45054;				//port number used by server (remote port)

	//char modifiedSentence[STRING_SIZE];		// send message


	unsigned int name_len;					//length of file name

	int bytes_sent, bytes_recd;				//number of bytes sent or received from packet header

	char fileTransferred[STRING_SIZE];		//name of file being transferred

	int totalPacketsRecd = 0;					// total number of packets received

	int totalDataBytesReceived = 0;				// all counts summed up

	struct packetHeader {
		int sequenceNumber;
		int count;
		//char dataBytes[STRING_SIZE];
	};

	char dataBytes[STRING_SIZE];
	packetHeader packetHeaderRecd;

	int headerBytesRecd;
	int dataBytesRecd;

	//string dataBytes;

	cout << "Enter name of file to be transfered" << endl;
	cin >> fileTransferred;

	name_len = strlen(fileTransferred) + 1;


	//open a socket

	if((sock_client= socket(PF_INET, SOCK_STREAM, IPPROTO_UDP)) < 0) {
		perror("Client: cant open stream socket");
		exit(1);
	}

	/*//initialize server address information

	cout << "Enter hostname of server: " << endl;
	cin >> server_hostname;


	if((server_hp = gethostbyname(server_hostname)) == NULL) {
		perror("Client: invalid server hostname");
		close(sock_client);
		exit(1);
	}*/

	/*cout << "Enter port number for server: " << endl;
	cin >> &server_port;*/



	// clear server address structure and initalize with server address

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy((char *)&server_addr.sin_addr,server_hp->h_addr,server_hp->h_length);
	server_addr.sin_port = htons(server_port);

	//connect to the server

	if (connect(sock_client, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("Client: can't connect to server");
		close(sock_client);
		exit(1);
	}

	//send message

	bytes_sent = send(sock_client, fileTransferred, name_len, 0);

	// get response from server

	while(packetHeaderRecd.count != 0) {
	dataBytesRecd = recv(sock_client, dataBytes, STRING_SIZE, 0);
	headerBytesRecd = recv(sock_client, &packetHeaderRecd, sizeof(packetHeader), 0);

	cout <<  "Packet " << packetHeaderRecd.sequenceNumber <<
			"received with" << packetHeaderRecd.count << "data bytes";


	ofstream out ("out.txt");
	out.open("out.txt", ios::ate);
	out << dataBytes << endl;
	out.close();
	//cin >> packetRecd.dataBytes;

	cout << "The response from server is:" << endl;
	cout << dataBytesRecd;

	totalPacketsRecd++;
	totalDataBytesReceived = totalDataBytesReceived + packetHeaderRecd.count;
	}

	totalPacketsRecd = totalPacketsRecd + 1; 	// for termination packet

	cout << "End of Transmission Packet with sequence number " << packetHeaderRecd.sequenceNumber
			<< "received with " << packetHeaderRecd.count << "data bytes";



	//close the socket

	close(sock_client);



}
