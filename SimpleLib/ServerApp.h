#pragma once

class ServerApp
{
public:
	ServerApp();
	~ServerApp();

	virtual void Run();
	virtual void Stop();
};