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

void openFile(char* fileName, char *packet)
{
	SOCKET s; 
	struct sockaddr_in si_other;

	struct timeval to;
	fd_set read;
	int ready;

	to.tv_sec = 5;
	to.tv_usec = 20;
	
	FILE* fp;
	int fileLen; 
	int packetNum = 0;
	int seqNum = 0;

	char ackBuff[3];
	int ackSize;
	int total = 0;
	int bytesIn = 0;

	LARGE_INTEGER frequency, start, end;
	long long int elapsed_time;

	QueryPerformanceFrequency(&frequency);

	fp = fopen(fileName, "rb");

	if (fp == NULL)
	{
		printf("\n\n\t--Error Opening Image to Read--");
		fclose(fp);
		exit(-1);
	}

	fseek(fp, 0, SEEK_END);
	fileLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	packet = (char*)malloc(PACKETSIZE + sizeof(int));

	if (!packet)
	{
		printf("\n\n\t--Memory Error Allocating Packet--");
		fclose(fp);
	}

	packetNum = fileLen / PACKETSIZE;
	if (fileLen % PACKETSIZE != 0)
	{
		packetNum++;
	}
	
	s = createSocket(&si_other);
	int slen = sizeof(si_other);

	FD_ZERO(&read);
	FD_SET(s, &read);

	while (seqNum < packetNum)
	{
		bytesIn = fread(packet, 1, PACKETSIZE, fp);
		memcpy(packet + PACKETSIZE, &seqNum, sizeof(int));

		QueryPerformanceCounter(&start);
		if (sendto(s, packet, PACKETSIZE + sizeof(int), 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
		{
			printf("\n\n\t--sendto() Failed with Error Code %d--", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else printf("\n\n\t--PACKET #%d SENT--", seqNum);
		QueryPerformanceCounter(&end);

		ready = select(s, &read, NULL, NULL, &to);

		if (ready == 0)
		{
			printf("\n\t--Timeout Occurred while Waiting for ACK--");
			printf("\n\t--Retransmitting Packet#%d--", seqNum);
			seqNum = seqNum;
			continue;
		}
		else if (ready == -1)
		{
			printf("\n\t--Error Occurred while Waiting for ACK--");
			exit(-1);
		}
		else
		{
			ackSize = recvfrom(s, ackBuff, 3, 0, NULL, NULL);
			if (ackSize == 3 && strncmp(ackBuff, "ACK", 3) == 0)
			{
				printf("\n\t--ACK Received for Packet#%d--", seqNum);
				seqNum++;
			}
			else
			{
				continue;
			}
		}
		total = total + bytesIn;

		elapsed_time = (end.QuadPart - start.QuadPart) * 1000000LL / frequency.QuadPart;
		printf("\n\t--Elapsed time: %lld microseconds--\n\n", elapsed_time);

	}

	fclose(fp);
	free(packet);
}

SOCKET createSocket(struct sockaddr_in *si_other)
{
	SOCKET s;
	int noBlock = 1;
	int slen = sizeof(si_other);
	
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("\n\n\t--Could Not Create Socket: %d--", WSAGetLastError());
	}
	else printf("\n\n\t--UDP CLIENT SOCKET CREATED--");

	noBlock = 1;
	ioctlsocket(s, FIONBIO, &noBlock);
	si_other->sin_family = AF_INET;
	si_other->sin_port = htons(80);
	inet_pton(AF_INET, "127.0.0.1", &si_other->sin_addr);

	connect(s, (struct sockaddr*)&si_other, slen);

	if (noBlock != 1)
	{
		printf("\n\n\t--CONNECTION ERROR--");
	}
	else printf("\n\n\t--Initializing Connection to Receive Side--");

	return (s);
}

int main()
{
	initWinsock();
	char filename[] = "image/test.jpg";
	char* packet = malloc(PACKETSIZE + sizeof(int));

	openFile(filename, packet);
}

