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
        this->decodeMessage();
    }
}

void Server::acceptClient() {
    this->clientSocket = accept(this->serverSocket, (struct sockaddr *) &(this->clientAddress), &(this->clientAddressLength));
    if (this->clientSocket < 0) {
        std::cerr << "Error: Could not accept client" << std::endl;
        exit(1);
    }
    this->clientPort = ntohs(this->clientAddress.sin_port);
}

void Server::readMessage() {
    char buffer[256];
    bzero(buffer, 256);
    int n = read(this->clientSocket, buffer, 255);
    if (n < 0) {
        std::cerr << "Error: Could not read from socket" << std::endl;
        exit(1);
    }

    std::cout << "Message: " << buffer;

    std::string message = buffer;
    this->messageParts = std::vector<std::string>();
    std::stringstream ss(message);
    std::string part;
    while (ss >> part) {
        printf("Part: %s\n", part.c_str());
        this->messageParts.push_back(part);
    }
}

void Server::decodeMessage() {
    if (this->messageParts[0] == "msg") {
        this->logMessage();
    } else if (this->messageParts[0] == "get" && this->messageParts[1] == "chatLog") {
        this->sendChatLog();
    } else if (this->messageParts[0] == "status") {
        this->checkStatus();
    } else if (this->messageParts[0] == "crash") {
        this->crashSequence();
    } else {
        std::cerr << "Error: Unknown message type" << std::endl;
    }
}

void Server::logMessage() {
    assert(this->messageParts.size() == 3 && "Error: msg should have 3 parts");
    int seq = stoi(this->messageParts[1]);
    char* message = (char*) this->messageParts[2].c_str();
    RumorMessage rumor(seq, message);
    this->status.addMessageToLog(rumor, seq);
}

void Server::sendChatLog() {

}

void Server::checkStatus() {
    int maxSeqNoRcvd = stoi(this->messageParts[1]);
    int ourMaxSeqNo = this->status.getMaxSeqNo();
    (maxSeqNoRcvd < ourMaxSeqNo) ? this->rumorMongering(maxSeqNoRcvd) :
    (maxSeqNoRcvd > ourMaxSeqNo) ? this->sendStatus()
                                : void(); /* Stop rumor mongering */
}

void Server::rumorMongering(int maxSeqNoRcvd) {
    std::string message = "msg " + std::to_string(this->status.getMaxSeqNo()) + " " + this->status.getMessage(maxSeqNoRcvd+1);
    const char* msg = message.c_str();
    int n = sendto(this->clientSocket, msg, strlen(msg), 0, (struct sockaddr*)&(this->clientAddress), this->clientAddressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to peer server" << std::endl;
        exit(1);
    }
    std::cout << "Rumor mongering msg " << maxSeqNoRcvd+1 << "..." << std::endl;
}

void Server::sendStatus() {
    std::string statusMessage = "status " + std::to_string(this->status.getMaxSeqNo());
    const char* message = statusMessage.c_str();
    int messageLength = strlen(message);
    int n = sendto(this->clientSocket, message, messageLength, 0, (struct sockaddr*)&(this->clientAddress), this->clientAddressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to peer server" << std::endl;
        exit(1);
    }
}

void Server::crashSequence() {
    std::cout << "Crashing..." << std::endl;
    this->closeConnection();
    exit(0);
}

void Server::closeConnection() {
    close(this->clientSocket);
    close(this->serverSocket);
}