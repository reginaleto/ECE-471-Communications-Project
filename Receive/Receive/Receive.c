#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PACKETSIZE 1028

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <time.h>
#include <Windows.h>


void initWinsock()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("\n\n\t--Failed. Error Code %d--", WSAGetLastError());
	}
	else printf("\n\n\t--WINSOCK INITIALIZED--");
}

SOCKET createAndBind(struct sockaddr_in *server)
{
	SOCKET s;
	
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("\n\n\t--Could not Create Socket: %d--", WSAGetLastError());
	}
	else printf("\n\n\t--UDP SERVER SOCKET CREATED--");

	int noBlock = 0;
	server->sin_addr.s_addr = inet_addr("127.0.0.1");
	server->sin_family = AF_INET;
	server->sin_port = htons(80);

	if (bind(s, (struct sockaddr*)server, sizeof(*server)) == SOCKET_ERROR)
	{
		printf("\n\n\t--Bind Failed with Error Code %d--", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	else printf("\n\t--SERVER SOCKET BIND SUCCESS--");

	return (s);
}

void receiveData(char* filename, SOCKET s, struct sockaddr_in server)
{
	char* packet = NULL; 
	char* image = NULL; 
	
	
	printf("\n\n\t--Waiting for Data--");

	FILE* writefp;;
	
	writefp = fopen(filename, "wb");
	printf("\n\n\t--Opening File...--");
	if (writefp == NULL)
	{
		printf("\n\n\t--Error Opening Image to Write--");
		fclose(writefp);
		exit(-1);
	}
	else printf("\n\n\t--File Opened for Writing--");


	int recv_len = 0;
	int packetSeqNum = 0;
	int seqNum = 0;
	int total = 0;
	
	packet = (char*)malloc(PACKETSIZE + sizeof(int));
	image = (char*)malloc(PACKETSIZE);


	while (1)
	{
		fflush(stdout);

		struct sockaddr_in si_other;
		int slen = sizeof(si_other);

		if ((recv_len = recvfrom(s, packet, PACKETSIZE + sizeof(int), 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR)
		{
			printf("\n\n\t--recvfrom() Failed with Error Code: %d--", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("\n\n\t--Server Received Packet IPaddr %s Port %d, SEQ# = %d--", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), seqNum);

			memcpy(&packetSeqNum, packet + PACKETSIZE, sizeof(int));
			if (packetSeqNum == seqNum)
			{
				memcpy(image, packet, PACKETSIZE);
				fwrite(image, 1, PACKETSIZE, writefp);

				sendto(s, "ACK", 3, 0, (struct sockaddr*)&si_other, slen);
				printf("\n\t--ACK Sent to Client for Sequence #%d--", packetSeqNum);
				seqNum++;
				total = total + (recv_len - sizeof(int));
			}
			else
			{
				printf("\n\t--Unable to Send ACK to Client--");
				seqNum = seqNum;
			}
			printf("\nBytes Received: %d", total);
		}

		if (total / PACKETSIZE > 9)
		{
			break;
		}
	}

	fclose(writefp);
	closesocket(s);
	WSACleanup();

	printf("\n\n\t--Imaged Saved - Press Any Key--");
}

int main()
{
	initWinsock();

	char filename[] = "received_image.jpg";
	SOCKET s;
	struct sockaddr_in server;
	int slen;

	s = createAndBind(&server);

	receiveData(filename, s, server);
}