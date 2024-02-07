#pragma once

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
	SOCKET _serverSocket;
	std::vector<std::string> _activeUsers;


	std::queue<std::string> _globalMessages;
	std::mutex _queueLock;
	void clientHandler(SOCKET clientSocket);
	void writeToFile();
	void accept();
	

	std::string returnUsers() const;
	std::string loginPart(SOCKET clientSocket);
	std::string getFileName(std::string firstUsername, std::string secondUsername);
	std::string getFileConversationContent(std::string firstUsername, std::string secondUsername);



};
