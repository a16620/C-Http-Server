BOOL SecureSafeCheckUrl(const char* url)
{
	if ((*url == '?') || (*url == '\\'))
		return FALSE;
	url++;
	while (*url != '\0')
	{
		if ((*url == '?') || (*url == '\\') || ((*url == '/') && (*(url-1) == '.')))
			return FALSE;
		url++;
	}
	return TRUE;
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
	SendHeader(so, HGET, code, size, GetFileMimeIdx(".html"));
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
			if (!SendDocument(client, htm.dir))
			{
				SendErrorPage(client, HTTPCODE_404);
			}
		}
		else
		{
			SendDocument(client, "index.html");
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
