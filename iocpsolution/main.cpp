#include "IOCompletionPort.h"

const UINT16 MAX_CLIENTS = 100;
const UINT16 SERVER_PORT = 9000;


int main()
{
	IOCompletionPort iocp;
	iocp.InitSocket();

	iocp.BindAndListen(SERVER_PORT);
	iocp.StartServer(MAX_CLIENTS);

	printf("Server Start\n");
	getchar();

	iocp.DestroyThread();
	return 0;
}