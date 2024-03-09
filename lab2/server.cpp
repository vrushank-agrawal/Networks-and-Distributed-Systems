#include "headers.hpp"

/**
 * @brief Construct a new Server object
 * @param port The port to bind the server to
*/
Server::Server(int port) {
    this->port = port;
    this->status = StatusMessage(port);
    for (auto& client : this->clients)
        client = nullptr;
}

Server::~Server() {
    close(this->serverSocket);
    for (auto& client : this->clients)
        close(client->socket);
}

/**
 * @brief Start the server and bind it to the specified port
*/
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

/**
 * @brief Listen for clients to connect
*/
void Server::listenClient() {
    std::cout << "Listening for clients..." << std::endl;
    std::thread t[3];

    while (true) {
        if (listen(this->serverSocket, 3) < 0) {
            std::cerr << "Error: Could not listen" << std::endl;
            exit(1);
        }

        /* Accept and add a new client */
        ClientInfo* client = this->acceptClient();
        int clientIndex = this->addClient(client);
        this->readMessage(clientIndex);
        this->processMessage(clientIndex);
        this->decodeMessage(clientIndex);

        /* Start thread to rcv messages from client and detach it */
        t[clientIndex] = std::thread(&Server::rcvMessages, this, clientIndex);
        t[clientIndex].detach();
    }

    /* Join threads */
    for (auto& thread : t) thread.join();
}

/**
 * @brief Accept a new client connection
*/
ClientInfo* Server::acceptClient() {
    ClientInfo* client = new ClientInfo();
    client->socket = accept(this->serverSocket, (struct sockaddr *) &(client->address), &(this->addressLength));
    if (client->socket < 0) {
        std::cerr << "Error: Could not accept client" << std::endl;
        exit(1);
    }
    client->port = ntohs(client->address.sin_port);
    return client;
}

/**
 * @brief Add a client to the clients array
 * @param client The client to add
*/
int Server::addClient(ClientInfo* client) {
    int filledIndex = -1;
    for (int i = 0; i < 3; i++) {
        if (this->clients[i] == nullptr) {
        this->clients[i] = new ClientInfo(*client);
            filledIndex = i;
            break;
        }
    }
    return filledIndex;
}

/**
 * @brief Read a message from the newly accepted client
 * @param clientIndex The index of the client in the clients array
*/
void Server::readMessage(int clientIndex) {
    char buffer[256];
    bzero(buffer, 256);
    int n = read((*this->clients[clientIndex]).socket, buffer, 255);
    if (n < 0) {
        std::cerr << "Error: Could not read from socket" << std::endl;
        exit(1);
    }
    std::cout << "Message: " << buffer;
    this->clients[clientIndex]->message = buffer;
}

/**
 * @brief Process the message from the client by splitting it into parts
 * @param clientIndex The index of the client in the clients array
*/
void Server::processMessage(int clientIndex) {
    std::string message = this->clients[clientIndex]->message;
    this->clients[clientIndex]->messageParts = std::vector<std::string>();
    std::stringstream ss(message);
    std::string part;
    while (ss >> part) {
        // printf("Part: %s\n", part.c_str());
        this->clients[clientIndex]->messageParts.push_back(part);
    }
}

/**
 * @brief Decode the message from the client and perform the appropriate action
 * @param clientIndex The index of the client in the clients array
*/
void Server::decodeMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    (messageParts[0] == "msg") ? this->logMessage(clientIndex)
        : (messageParts[0] == "get" && messageParts[1] == "chatLog") ? this->sendChatLog()
        : (messageParts[0] == "status") ? this->checkStatus(clientIndex)
        : (messageParts[0] == "crash") ? this->crashSequence()
        : void(std::cerr << "Error: Unknown message type" << std::endl);
}

/**
 * @brief Receive messages from a connected client untill disconnected
 * @param clientIndex The index of the client in the clients array
*/
void Server::rcvMessages(int clientIndex) {
    ClientInfo* client = this->clients[clientIndex];

    /* Rcv messages from client until disconnected */
    int n;
    while ((n = recv(client->socket, client->message, 255, 0)) > 0) {
        this->processMessage(clientIndex);
        this->decodeMessage(clientIndex);
    }

    /* The client has disconnected */
    if (n < 0) {
        std::cerr << "Error: Could not read from socket" << std::endl;
        this->closeClient(clientIndex);
    }
}

/**
 * @brief Add the received message to the chat log
 * @param clientIndex The index of the client in the clients array
*/
void Server::logMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    assert(messageParts.size() == 3 && "Error: msg should have 3 parts");

    int seq = stoi(messageParts[1]);
    char* message = (char*) messageParts[2].c_str();

    RumorMessage rumor(seq, message);
    this->status.addMessageToLog(rumor, seq);
}

void Server::sendChatLog() {

}

/**
 * @brief Check the status of the client if it is behind or ahead of the server
 *      and rumor monger if behind or send status if ahead
 * @param clientIndex The index of the client in the clients array
*/
void Server::checkStatus(int clientIndex) {
    int maxSeqNoRcvd = stoi(this->clients[clientIndex]->messageParts[1]);
    int ourMaxSeqNo = this->status.getMaxSeqNo();
    (maxSeqNoRcvd < ourMaxSeqNo) ? this->rumorMongering(maxSeqNoRcvd, clientIndex)
        : (maxSeqNoRcvd > ourMaxSeqNo) ? this->sendStatus(clientIndex)
        : void(); /* Stop rumor mongering */
}

/**
 * @brief Send a message to the client with the next message in the chat log
 * @param maxSeqNoRcvd The maximum sequence number with the client
 * @param clientIndex The index of the client in the clients array
*/
void Server::rumorMongering(int maxSeqNoRcvd, int clientIndex) {
    ClientInfo* client = this->clients[clientIndex];

    /* Create message for client */
    std::string message = "msg " + std::to_string(this->status.getMaxSeqNo()) + " " + this->status.getMessage(maxSeqNoRcvd+1);
    const char* msg = message.c_str();

    int n = sendto(client->socket, msg, strlen(msg), 0, (struct sockaddr*)&(client->address), this->addressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to " << client->port << std::endl;
        this->closeClient(clientIndex);
        return;
    }

    /* Message sent */
    std::cout << "Rumor mongering msg " << maxSeqNoRcvd+1 << "..." << std::endl;
}

/**
 * @brief Send the status of the server to a neighbor client
 * @param index The index of the client in the clients array
*/
void Server::sendStatus(int index) {
    ClientInfo* client = this->clients[index];
    std::string statusMessage = "status " + std::to_string(this->status.getMaxSeqNo());
    const char* message = statusMessage.c_str();
    int messageLength = strlen(message);
    int n = sendto(client->socket, message, messageLength, 0, (struct sockaddr*)&(client->address), this->addressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to " << client->port << std::endl;
        this->closeClient(index);
    }
}

/**
 * @brief Close the server and all client connections
*/
void Server::crashSequence() {
    std::cout << "Crashing..." << std::endl;
    this->closeAllConnections();
    exit(0);
}

/**
 * @brief Close a client connection
*/
void Server::closeClient(int clientIndex) {
    std::cout << "Closing client connection " << this->clients[clientIndex]->port << "..." << std::endl;
    close(this->clients[clientIndex]->socket);
    delete this->clients[clientIndex];
    this->clients[clientIndex] = nullptr;
}

/**
 * @brief Close all client connections and the server
*/
void Server::closeAllConnections() {
    for (auto& client : this->clients)
        close(client->socket);
    close(this->serverSocket);
}