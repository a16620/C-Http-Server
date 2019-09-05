//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#define THIS_DOMAIN ("localhost:8080")
#define PATH_ERRORPAGE ("error_html")
#define PATH_HTMLPATH ("public_html")
#define PATH_H_LEN 11
#define SEND_BUFFER_SIZE 128

const char* HTTPCODEMSG[50] = {
"100 Continue",
"101 Switching Protocols",
"103 Early Hints",
"200 OK",
"201 Created",
"202 Accepted",
"203 Non-Authoritative Information",
"204 No Content",
"205 Reset Content",
"206 Partial Content",
"300 Multiple Choices",
"301 Moved Permanently",
"302 Found",
"303 See Other",
"304 Not Modified",
"307 Temporary Redirect",
"308 Permanent Redirect",
"400 Bad Request",
"401 Unauthorized",
"402 Payment Required",
"403 Forbidden",
"404 Not Found",
"405 Method Not Allowed",
"406 Not Acceptable",
"407 Proxy Authentication Required",
"408 Request Timeout",
"409 Conflict",
"410 Gone",
"411 Length Required",
"412 Precondition Failed",
"413 Payload Too Large",
"414 URI Too Long",
"415 Unsupported Media Type",
"416 Range Not Satisfiable",
"417 Expectation Failed",
"418 I'm a teapot",
"422 Unprocessable Entity",
"425 Too Early",
"426 Upgrade Required",
"428 Precondition Required",
"429 Too Many Requests",
"431 Request Header Fields Too Large",
"451 Unavailable For Legal Reasons",
"500 Internal Server Error",
"501 Not Implemented",
"502 Bad Gateway",
"503 Service Unavailable",
"504 Gateway Timeout",
"505 HTTP Version Not Supported",
"511 Network Authentication Required"
};
typedef enum HTTPCODEINDEX
{
	HTTPCODE_100 = 0,
	HTTPCODE_101 = 1,
	HTTPCODE_103 = 2,
	HTTPCODE_200 = 3,
	HTTPCODE_201 = 4,
	HTTPCODE_202 = 5,
	HTTPCODE_203 = 6,
	HTTPCODE_204 = 7,
	HTTPCODE_205 = 8,
	HTTPCODE_206 = 9,
	HTTPCODE_300 = 10,
	HTTPCODE_301 = 11,
	HTTPCODE_302 = 12,
	HTTPCODE_303 = 13,
	HTTPCODE_304 = 14,
	HTTPCODE_307 = 15,
	HTTPCODE_308 = 16,
	HTTPCODE_400 = 17,
	HTTPCODE_401 = 18,
	HTTPCODE_402 = 19,
	HTTPCODE_403 = 20,
	HTTPCODE_404 = 21,
	HTTPCODE_405 = 22,
	HTTPCODE_406 = 23,
	HTTPCODE_407 = 24,
	HTTPCODE_408 = 25,
	HTTPCODE_409 = 26,
	HTTPCODE_410 = 27,
	HTTPCODE_411 = 28,
	HTTPCODE_412 = 29,
	HTTPCODE_413 = 30,
	HTTPCODE_414 = 31,
	HTTPCODE_415 = 32,
	HTTPCODE_416 = 33,
	HTTPCODE_417 = 34,
	HTTPCODE_418 = 35,
	HTTPCODE_422 = 36,
	HTTPCODE_425 = 37,
	HTTPCODE_426 = 38,
	HTTPCODE_428 = 39,
	HTTPCODE_429 = 40,
	HTTPCODE_431 = 41,
	HTTPCODE_451 = 42,
	HTTPCODE_500 = 43,
	HTTPCODE_501 = 44,
	HTTPCODE_502 = 45,
	HTTPCODE_503 = 46,
	HTTPCODE_504 = 47,
	HTTPCODE_505 = 48,
	HTTPCODE_511 = 49
}HTTPCODEINDEX;
#define HTTPCODE(cdx) (HTTPCODEMSG[cdx])
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
	char* dir;
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

	char* flnd2 = strchr(++flnd, ' ');
	if (flnd == NULL)
		return FALSE;

	len = flnd2 - flnd;
	s = malloc(len+1);
	memcpy(s, flnd, len);
	s[len] = '\0';
	htm->dir = s;

	return TRUE;
}

void SendHeader(SOCKET so, HTTPMETHOD method, HTTPCODEINDEX code, unsigned int contentLength)
{
	char* headers[5] = {
		"HTTP/1.1 %s\r\n",
		"Host: %s\r\nConnection: close\r\n",
		"Date : Sat, 13 Jul 2019 00:00:00 GMT\r\n",
		"Content-Length : %u\r\n",
		"Content-Type : text/html\r\n\r\n"};
	char msg[300];
	snprintf(msg, 300, headers[0], HTTPCODE(code));
	send(so, msg, strlen(msg), 0);
	snprintf(msg, 300, headers[1], THIS_DOMAIN);
	send(so, msg, strlen(msg), 0);
	snprintf(msg, 300, headers[2]);
	send(so, msg, strlen(msg), 0);
	snprintf(msg, 300,headers[3], contentLength);
	send(so, msg, strlen(msg), 0);
	snprintf(msg, 300, headers[4]);
	send(so, msg, strlen(msg), 0);
}

BOOL OpenFileFromDirectory(FILE** fp, const char* path, size_t* szLen)
{
	FILE* f = fopen(path, "r");
	if (f == NULL)
		return FALSE;
	fseek(f, 0, SEEK_END);
	*szLen = ftell(f);
	fseek(f, 0, SEEK_SET);
	*fp = f;
	return TRUE;
}

VOID SendFile(SOCKET so, FILE* const fp, size_t szBuffer)
{
	char* buffer = malloc(szBuffer);
	int rl = 0;
	while (!feof(fp))
	{
		rl = fread(buffer, 1, szBuffer, fp);
		send(so, buffer, rl, 0);
	}

	free(buffer);
}

BOOL SendHtml(SOCKET so, const char* src)
{
	FILE* fp = 0;
	size_t size = 0;
	char path[300] = PATH_HTMLPATH;
	path[PATH_H_LEN] = '\\';
	path[PATH_H_LEN + 1] = '\0';
	strcat(path, src);
	if (!OpenFileFromDirectory(&fp, path, &size))
		return FALSE;
	
	SendHeader(so, HGET, HTTPCODE_200, size);
	SendFile(so, fp, SEND_BUFFER_SIZE);
	fclose(fp);
	return TRUE;
}


BOOL SecureSafeCheckUrl(const char* url)
{
	if (strchr(url, '?') == NULL)
		return TRUE;
	return FALSE;
}

BOOL SendErrorPage(SOCKET so, HTTPCODEINDEX code)
{
	char path[256] = PATH_ERRORPAGE;
	char epath[10] = "\\   .html";
	epath[1] = HTTPCODEMSG[code][0];
	epath[2] = HTTPCODEMSG[code][1];
	epath[3] = HTTPCODEMSG[code][2];
	strcat(path, epath);
	FILE* fp = 0;
	size_t size = 0;
	if (!OpenFileFromDirectory(&fp, path, &size))
		return FALSE;
	SendHeader(so, HGET, code, size);
	SendFile(so, fp, SEND_BUFFER_SIZE);
	fclose(fp);
	return TRUE;
}

VOID ParseUriToPath(char* str)
{
	while (*str != '\0')
	{
		if (*str == '/')
			*str = '\\';
		str++;
	}
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

	HTTPMethod htm;
	if (GetDirectoryFromHttpRequestPacket(buffer, &htm))
	{
		printf("%s Dir:%s\n", (htm.method == HGET ? "GET" : "UND"), htm.dir);
		if (!SecureSafeCheckUrl(htm.dir))
			SendErrorPage(client, HTTPCODE_403);
		if (strcmp(htm.dir, "/"))
		{
			ParseUriToPath(htm.dir);
			if (!SendHtml(client, htm.dir))
			{
				SendErrorPage(client, HTTPCODE_404);
			}
		}
		else
		{
			SendHtml(client, "index.html");
		}
		free(htm.dir);
	}
	else
	{
		SendErrorPage(client, HTTPCODE_502);
	}
	free(buffer);

	closesocket(client);
	//연결해제 & 처리 끝
	}
	WSACleanup();
	return 0;
}