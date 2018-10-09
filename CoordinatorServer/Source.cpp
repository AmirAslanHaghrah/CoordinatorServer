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

#define DEFAULT_PORT "13690"

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

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	//char recvbuf[DEFAULT_RECV_BUFLEN];
	//int recvbuflen = DEFAULT_RECV_BUFLEN;

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
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
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
	system("pause");
	return 0;
}




///To Do
/// 1- Complete ClientDataReceiver and ClientDataSender functions and test them
/// 2- AT Command execute function
/// 3- PING Client Function
///