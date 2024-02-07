#pragma once
#include "Helper.h"

#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <queue>
#include <mutex>



class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:
	//things i need
	SOCKET _serverSocket;
	std::vector<std::string> _activeUsers;
	std::queue<std::string> _globalMessages;
	std::mutex _queueLock;


	void writeToFile();
	void accept();
	void clientHandler(SOCKET clientSocket);

	std::string returnUsers() const;
	std::string login(SOCKET clientSocket);
	std::string getFileName(std::string firstUsername, std::string secondUsername);
	std::string getFileContent(std::string firstUsername, std::string secondUsername);



};

