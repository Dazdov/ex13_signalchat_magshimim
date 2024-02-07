
#include "Server.h"

#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>



Server::Server()
{

	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ " - creating socket failed");
}


Server::~Server()
{
	try
	{
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);
	}
	catch (...) {}
}

void Server::serve(int port)
{

	struct sockaddr_in sa = { 0 };

	sa.sin_port = htons(port); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");

	// Start listening
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "Listening on port " << port << std::endl;

	std::thread writeToFileThread(&Server::writeToFile, this);
	writeToFileThread.detach();
	while (true)
	{
		std::cout << "Waiting for client connection request" << std::endl;
		accept();
	}
}
void Server::accept()
{
	SOCKET client_socket = ::accept(this->_serverSocket, NULL, NULL);

	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);

	std::cout << "Client accepted. Server and client can speak" << std::endl;

	
	std::thread td(&Server::clientHandler, this, client_socket);
	td.detach();
}

void Server::clientHandler(SOCKET clientSocket)
{
	try
	{
		std::string username = "";
		std::string secondUsername = "";
		std::string newMessage = "";
		int packetCode = 0;			
		int length = 0;				

		username = this->login(clientSocket);

		Helper::send_update_message_to_client(clientSocket, "", returnUsers(), username);


		while (true)
		{
		 
			packetCode = Helper::getIntPartFromSocket(clientSocket, 3);

			int secondUserLength = Helper::getIntPartFromSocket(clientSocket, 2);
			std::string secondUsername = Helper::getStringPartFromSocket(clientSocket, secondUserLength);
			int newMessageLength = Helper::getIntPartFromSocket(clientSocket, 5);
			std::string newMessage = Helper::getStringPartFromSocket(clientSocket, newMessageLength);

			if (secondUserLength == 0)
				Helper::send_update_message_to_client(clientSocket, "", "", this->returnUsers());
			else
			{

				if (newMessageLength == 0)
					Helper::send_update_message_to_client(clientSocket, this->getFileContent(username, secondUsername), secondUsername, this->returnUsers());
				else
				{
					std::cout << "New message: \nAUTHOR: " << username << "\nReciever: " << secondUsername << std::endl;
					{
						std::lock_guard<std::mutex> lock(this->_queueLock);
						//Push the message like that: author#reciever#message
						this->_globalMessages.push(username + "#" + secondUsername + "#" + newMessage);
					}

					std::string updatedMessage = "&MAGSH_MESSAGE&&Author&" + username + "&DATA&" + newMessage;
					std::string fileContent = this->getFileContent(username, secondUsername) + updatedMessage, secondUsername;

					Helper::send_update_message_to_client(clientSocket, fileContent, secondUsername, this->returnUsers());
				}
				std::cout << username << ": " << packetCode << secondUsername;
			}
		}
	}
	catch (const std::exception& e)
	{
		closesocket(clientSocket);
		std::string username = e.what();
		username = username.substr(0, username.find("#"));

		std::vector<std::string>::iterator name = std::find(this->_activeUsers.begin(), this->_activeUsers.end(), username);
		if (name != this->_activeUsers.end())
			this->_activeUsers.erase(name);
		std::cout << "User Disconnected: " << e.what() << std::endl;
	}
}


std::string Server::returnUsers() const
{
	std::string result = "";
	for (int i = 0; i < this->_activeUsers.size(); i++)
	{
		result += this->_activeUsers[i];
		result += "&";
	}
	//fixed last & problem
	if (result != "")
	{
		result.pop_back();
	}
	return result;
}

std::string Server::getFileName(std::string firstUsername, std::string secondUsername)
{
	std::string txtFileName = "";
	if (firstUsername.compare(secondUsername) < 0)
		txtFileName = secondUsername + "&" + firstUsername;
	else
		txtFileName = firstUsername + "&" + secondUsername;
	txtFileName += ".txt";
	return txtFileName;
}


void Server::writeToFile()
{
	bool wroteToFile = false;
	std::string messageQueue = "";
	std::string finalFileMessage = "";
	while (true)
	{
		
		while (!this->_globalMessages.empty())
		{
			{
				std::lock_guard<std::mutex> lock(this->_queueLock);
				messageQueue = this->_globalMessages.front();
				this->_globalMessages.pop();
			}
			std::string firstUsername = messageQueue;  
			std::string secondUsername = messageQueue;

			firstUsername = messageQueue.substr(0, messageQueue.find("#"));
			messageQueue = messageQueue.substr(messageQueue.find("#") + 1, messageQueue.size() - messageQueue.find("#") + 1);

			secondUsername = messageQueue.substr(0, messageQueue.find("#"));
			messageQueue = messageQueue.substr(messageQueue.find("#") + 1, messageQueue.size() - messageQueue.find("#") + 1);

			std::string txtFileName = this->getFileName(firstUsername, secondUsername);
			finalFileMessage = "&MAGSH_MESSAGE&&Author&" + firstUsername + "&DATA&" + messageQueue;

			std::ofstream file(txtFileName, std::ios_base::app);

			
			do
			{
				if (file.is_open())
				{
					file << finalFileMessage;
					file.close();
					wroteToFile = true;
				}
				else
					file.open(txtFileName);
			} while (!wroteToFile);
		}
	}
}

std::string Server::getFileContent(std::string firstUsername, std::string secondUsername)
{
	std::string txtFileName = this->getFileName(firstUsername, secondUsername);
	std::string content = "";
	std::string line = "";

	std::ifstream file(txtFileName);

	if (file.is_open())
	{
		while (std::getline(file, line))
			content += line;
		file.close();
	}
	std::cout << content << std::endl;
	return content;
}

std::string Server::login(SOCKET clientSocket)
{
	std::string username = "";
	int packetCode = 0;	
	int length = 0;				

	packetCode = Helper::getIntPartFromSocket(clientSocket, 3);
	std::cout << packetCode << " ";
	length = Helper::getIntPartFromSocket(clientSocket, 2);
	std::cout << length << " ";

	username = Helper::getStringPartFromSocket(clientSocket, length);
	this->_activeUsers.push_back(username);
	std::cout << username << std::endl;
	return username;
}

