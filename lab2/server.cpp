#include "headers.hpp"

Server::Server(int port) {
    this->port = port;
    this->status = StatusMessage(port);
}

Server::~Server() {
    close(this->serverSocket);
    close(this->clientSocket);
}

void Server::start() {
    this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (this->serverSocket < 0) {
        std::cerr << "Error: Could not open socket" << std::endl;
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(this->port);

    if (bind(this->serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Error: Could not bind socket" << std::endl;
        exit(1);
    }
}

void Server::listenClient() {
    std::cout << "Listening for clients..." << std::endl;

    while (true) {
        if (listen(this->serverSocket, 5) < 0) {
            std::cerr << "Error: Could not listen" << std::endl;
            exit(1);
        }

        this->acceptClient();

        this->readMessage();

    }
}

void Server::acceptClient() {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    this->clientSocket = accept(this->serverSocket, (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (this->clientSocket < 0) {
        std::cerr << "Error: Could not accept client" << std::endl;
        exit(1);
    }
    this->clientPort = ntohs(clientAddress.sin_port);
}

void Server::readMessage() {
    char buffer[256];
    bzero(buffer, 256);
    int n = read(this->clientSocket, buffer, 255);
    if (n < 0) {
        std::cerr << "Error: Could not read from socket" << std::endl;
        exit(1);
    }
    this->clientMessage = buffer;
    std::cout << "Message: " << this->clientMessage << std::endl;
}