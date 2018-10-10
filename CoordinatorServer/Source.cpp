#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <ctime>
#include <vector>
#include <thread>
#include <string>
#include <functional>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")


#define DEFAULT_RECV_BUFLEN 512
#define DEFAULT_SEND_BUFLEN 512

#define DEFAULT_TCP_PORT "13690"
#define DEFAULT_UDP_PORT 13691

class Client{
public:

	SOCKET ClientSocket;
	struct sockaddr_in ClientAddress;

	char recvbuf[DEFAULT_RECV_BUFLEN];
	int recvbuflen = DEFAULT_RECV_BUFLEN;

	Client();
	Client(SOCKET clientSocket, struct sockaddr_in clientAddress);

	~Client();

	int Close();
	int Read();
	int Read(char* buffer, int len);
	int Write(char* buffer, int len);

private:

};

Client::Client() {
}

Client::Client(SOCKET socket, struct sockaddr_in address) {
	ClientSocket = socket;
	ClientAddress = address;
}

Client::~Client(){	
	Close();
}

int Client::Close() {
	int iResult;
	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
}

int Client::Read(char* buffer, int len) {
	int iResult;

	iResult = recv(ClientSocket, buffer, len, 0);
	if (iResult > 0) {
		printf("%s\t%d\:\n", inet_ntoa(ClientAddress.sin_addr), (int)ntohs(ClientAddress.sin_port));
		printf("Received bytes: %.*s\n", iResult, buffer);
		return 1;
	}
	else if (iResult == 0) {
		printf("%s\t%d\ close connection.\n", inet_ntoa(ClientAddress.sin_addr), (int)ntohs(ClientAddress.sin_port));
		return 0;
	}		
	else {
		printf("recv failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}	
}

int Client::Read() {
	int iResult;
	do {
		iResult = recv(ClientSocket, recvbuf, DEFAULT_RECV_BUFLEN, 0);
		if (iResult > 0) {
			printf("%s\t%d\:\n", inet_ntoa(ClientAddress.sin_addr), (int)ntohs(ClientAddress.sin_port));
			printf("Received bytes: %.*s\n", iResult, recvbuf);
			//return 1;
		}
		else if (iResult == 0) {
			printf("%s\t%d\ close connection.\n", inet_ntoa(ClientAddress.sin_addr), (int)ntohs(ClientAddress.sin_port));
			//return 0;
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			//return -1;
		}
	} while (iResult > 0);
	return iResult;
}

int Client::Write(char* buffer, int len) {
	int iSendResult;

	iSendResult = send(ClientSocket, buffer, len, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	return 0;
}

//#define _CRT_SECURE_NO_WARNINGS
//
//#include <iostream>
//#include <string>
//#include <ctime>
//
//using namespace std;
//
//const string DAYS[] = { "Sunday","Monday","Tuesday",
//	"Wednesday","Thursday","Friday","Saturday" };
//
//const string MONTHS[] = { "January","February","March","April","May","June",
//	"July","August","September","October","November","December" };
//
//int main()
//{
//	int wday, mday, month, year;
//
//	time_t rawtime;
//	tm * timeinfo;
//
//	time(&rawtime);
//	timeinfo = localtime(&rawtime);
//
//	wday = timeinfo->tm_wday;
//	mday = timeinfo->tm_mday;
//	month = timeinfo->tm_mon;
//	year = timeinfo->tm_year;
//
//	cout << DAYS[wday] << ", ";
//	cout << MONTHS[month] << " " << mday << ", ";
//	cout << 1900 + year << endl;
//
//	cout << "\nhit enter to quit...";
//	cin.get();
//	return 0;
//}




int clientsCount = 0;

std::vector<Client*> clientList;
std::vector<std::thread*> clientThread;

int startTCPServer();
int startUDPServer();

int __cdecl main(void)
{
	std::thread t1(startTCPServer);
	std::cout << "TCP Server Successfully Created.\r\n";

	std::thread t2(startUDPServer);
	std::cout << "UDP Server Successfully Created.\r\n";

	while (true) {

	}

	t1.join();
	t2.join();

	system("pause");
	return 0;
}


int startTCPServer() {
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_TCP_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	while (true) {
		// Accept a client socket
		struct sockaddr_in client_addr;
		int clen = sizeof(client_addr);
		ClientSocket = accept(ListenSocket, (struct sockaddr *)&client_addr, &clen);

		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}


		printf("IP: %s\t", inet_ntoa(client_addr.sin_addr));
		int port = (int)ntohs(client_addr.sin_port);

		std::string s = std::to_string(port);
		char const *sendbuf = s.c_str();

		printf("port: %d", port);
		printf("\t Join!!!\n");
		iSendResult = send(ClientSocket, sendbuf, (int)strlen(sendbuf), 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			system("pause");
			return 1;
		}


		clientList.push_back(new Client(ClientSocket, client_addr));
		clientThread.push_back(new std::thread(std::mem_fun(&Client::Read), clientList[clientList.size() - 1]));
	}


	//for (int i = 0; i < clientsCount; i++) {
	//	clientThread[i].join();
	//}

	WSACleanup();
	return 0;
}


int startUDPServer() {
	/*
	Simple UDP Server
	*/
	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen, recv_len;
	char buf[DEFAULT_RECV_BUFLEN];
	WSADATA wsa;

	slen = sizeof(si_other);

	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}
	printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(DEFAULT_UDP_PORT);
	

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	puts("Bind done");

	//keep listening for data
	while (1)
	{
		printf("Waiting for data...\r\n");
		fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', DEFAULT_RECV_BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, DEFAULT_RECV_BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", buf);

		//now reply the client with the same data
		if (sendto(s, "echo: ", 7, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		//now reply the client with the same data
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

///To Do
/// 1- Complete ClientDataReceiver and ClientDataSender functions and test them
/// 2- AT Command execute function
/// 3- PING Client Function
///