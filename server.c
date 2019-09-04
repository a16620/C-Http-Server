//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#define NEW_SEGMENT (PHTTPRequestSegment)malloc(sizeof(HTTPRequestSegment))
#define HTTPSEGMENT_BUFFER_SIZE 1024
typedef struct HTTPRequestSegment {
	char buffer[HTTPSEGMENT_BUFFER_SIZE];
	struct HTTPRequestSegment* next;
} HTTPRequestSegment, *PHTTPRequestSegment;

PHTTPRequestSegment _beginQueue = NULL, _endQueue = NULL;
void PushQueue(PHTTPRequestSegment const _src)
{
	if (_beginQueue == NULL)
	{
		_beginQueue = NEW_SEGMENT;
		_endQueue = _beginQueue;
		memcpy(_endQueue->buffer, _src->buffer, HTTPSEGMENT_BUFFER_SIZE);
		_endQueue->next = NULL;
	}
	else
	{
		PHTTPRequestSegment curr = NEW_SEGMENT;
		memcpy(curr->buffer, _src->buffer, HTTPSEGMENT_BUFFER_SIZE);
		curr->next = _endQueue;
		_endQueue = curr;
	}
}

BOOL PopQueue(PHTTPRequestSegment _dest)
{
	if (_beginQueue == NULL)
		return FALSE;

	memcpy(_dest->buffer, _beginQueue->buffer, HTTPSEGMENT_BUFFER_SIZE);
	PHTTPRequestSegment curr = _beginQueue;
	_beginQueue = _beginQueue->next;
	free(curr);
	return TRUE;
}

void ClearQueue()
{
	if (_beginQueue == NULL)
		return;
	HTTPRequestSegment tmp;
	while (PopQueue(&tmp));
}

//BOOL ParseHttpPacket(const char* pk)
//{
//
//}

typedef enum HTTPMETHOD
{
	HGET,
	HPOST,
	HPUT,
	HHEAD,
	HCONNECT,
	HOPTIONS,
	HDELETE,
	HTRACE,
	HERROR
} HTTPMETHOD;

typedef struct HTTPMethod
{
	const char* dir;
	enum HTTPMETHOD method;
} HTTPMethod, *PHTTPMethod;

BOOL GetDirectoryFromHttpRequestPacket(const char* pk, PHTTPMethod htm)
{
	if (pk == NULL)
		return FALSE;
	const char* flnd = strchr(pk, ' ');
	if (flnd == NULL)
		return FALSE;
	int len = flnd - pk;
	char* s = malloc(len + 1);

	memcpy(s, pk, len);
	s[len] = '\0';
	if (!strcmp(s, "GET"))
	{
		htm->method = HGET;
	}
	else if (!strcmp(s, "POST"))
	{
		htm->method = HPOST;
	}
	else
	{
		htm->method = HERROR;
	}
	free(s);

	char* flnd2 = strchr(flnd+1, ' ');
	if (flnd == NULL)
		return FALSE;

	len = flnd2 - flnd;
	s = malloc(len+1);
	memcpy(s, flnd, len);
	s[len] = '\0';
	htm->dir = s;

	return TRUE;
}

int main(int argc, char* args[])
{
	//소켓 초기화
	struct WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in sAddr;
	sAddr.sin_family = AF_INET;
	sAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	sAddr.sin_port = htons(8080);
	//바인딩
	bind(listener, (struct sockaddr*)&sAddr, sizeof(struct sockaddr_in));
	listen(listener, SOMAXCONN);

	while (1)
	{
	//클라이언트 처리
	struct sockaddr_in cAddr;
	int szAddr = sizeof(struct sockaddr_in);
	SOCKET client;
	do {
		client = accept(listener, (struct sockaddr*)& cAddr, &szAddr);
	} while (client <= 0);

	int ltlen = 0;
	int segcnt = 0;
	while (1)
	{
		//for arduino
		/*register int i = 0;
		for (; i < HTTPSEGMENT_BUFFER_SIZE; ++i)
		{
			recv(client,)
		}*/
		//for windows
		HTTPRequestSegment _seg;
		int len = recv(client, _seg.buffer, HTTPSEGMENT_BUFFER_SIZE, 0);
		if (len == -1)
		{
			ltlen = -1;
			break;
		}
		if (len == 0)
		{
			ltlen = HTTPSEGMENT_BUFFER_SIZE;
			break;
		}
		else if (len < HTTPSEGMENT_BUFFER_SIZE)
		{
			PushQueue(&_seg);
			ltlen = len;
			segcnt++;
			break;
		}
		else
		{
			PushQueue(&_seg);
			segcnt++;
		}
	}

	if (ltlen == -1)
	{
		//오류 처리
		int a = 1; //중단점용 임시 식
	}
	
	char* buffer = malloc(segcnt*HTTPSEGMENT_BUFFER_SIZE+ltlen+1);
	HTTPRequestSegment _seg;
	for (int i = 0; i < segcnt - 1; i++)
	{
		PopQueue(&_seg);
		memcpy(buffer + (i*HTTPSEGMENT_BUFFER_SIZE), _seg.buffer, HTTPSEGMENT_BUFFER_SIZE);
	}
	PopQueue(&_seg);
	memcpy(buffer + ((segcnt-1)*HTTPSEGMENT_BUFFER_SIZE), _seg.buffer, ltlen);
	buffer[(segcnt - 1)*HTTPSEGMENT_BUFFER_SIZE + ltlen] = '\0';
	ClearQueue();

	
	free(buffer);


	const char* respond = "HTTP/1.1 200 OK\r\nHost : localhost:8080\r\nConnection : close\r\nDate : Sat, 13 Jul 2019 00:00:00 GMT\r\nContent-Length : 4\r\nContent-Type : text/html\r\n\r\nTest";
	send(client, respond, 120, 0);
	send(client, respond + 120, 26, 0);
	closesocket(client);
	//연결해제 & 처리 끝
	}
	WSACleanup();
	return 0;
}
