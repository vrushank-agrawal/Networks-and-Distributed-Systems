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
    this->destroyThreads();
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

    int yes = 1;
    if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
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
    /* Start thread to connect to left neighbor and detach it */
    std::thread temp = std::thread(&Server::connectToLeftNeighbor, this);
    temp.detach();
    this->threads.emplace_back(std::move(temp));

    /* Start thread to perform anti-entropy and detach it */
    std::thread antiEntropyThread(&Server::antiEntropy, this);
    antiEntropyThread.detach();
    this->threads.emplace_back(std::move(antiEntropyThread));

    std::cout << "Listening for clients..." << std::endl;
    while (true) {
        /* Listen for only right neighbor and proxy */
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
        std::thread thread = std::thread(&Server::rcvMessages, this, clientIndex);
        thread.detach();
        this->threads.emplace_back(std::move(thread));
    }

    /* Join threads */
    this->destroyThreads();
}


/*******************************************************************************
 *                           Server Socket methods                             *
 * *****************************************************************************/

/**
 * @brief Destroy all threads
*/
void Server::destroyThreads() {
    for (auto& thread : this->threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
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
    std::cout << "Received initial conn req from " << client->port << std::endl;
    if (client->port == this->port+1) { /* Right neighbor */
        this->clients[1] = client;
        std::cout << "Connected to right neighbor " << this->port+1 << std::endl;
        return 1;
    } else if (client->port == this->port-1) { /* Left neighbor */
        std::cerr << "Error: Left neighbor cannot send connect request" << std::endl;
        exit(1);
    } else { /* Proxy */
        this->clients[2] = client;
        std::cout << "Connected to proxy " << client->port << std::endl;
        return 2;
    }
}

/**
 * @brief Check if a certain client is connected
*/
bool Server::clientConnected(int clientIndex) {
    return this->clients[clientIndex] != nullptr;
}

/**
 * @brief Close a client connection
*/
void Server::closeClient(int clientIndex) {
    std::cout << "Closing client connection " << this->clients[clientIndex]->port << std::endl;
    close(this->clients[clientIndex]->socket);
    delete this->clients[clientIndex];
    this->clients[clientIndex] = nullptr;
}

/**
 * @brief Close all client connections and the server
*/
void Server::closeAllConnections() {
    for (auto& client : this->clients) {
        if (client != nullptr && client->socket > 0) {
            close(client->socket);
        }
    }
    if (this->serverSocket > 0) {
        close(this->serverSocket);
    }
    std::cout << "Closed all connections" << std::endl;
}


/*******************************************************************************
 *                           Message handling methods                          *
 * *****************************************************************************/

/**
 * @brief Read a message from the newly accepted client
 * @param clientIndex The index of the client in the clients array
*/
void Server::readMessage(int clientIndex) {
    char buffer[256];
    bzero(buffer, 256);
    int n = read(this->clients[clientIndex]->socket, buffer, 255);
    if (n < 0) {
        std::cerr << "Error: Could not read from socket" << std::endl;
        exit(1);
    }
    std::cout << "Message: " << buffer << std::endl;
    this->clients[clientIndex]->message = std::string(buffer, n);
    std::cout << "Thread " << std::this_thread::get_id() << " reading message from client " << clientIndex << ": " << this->clients[clientIndex]->message << std::endl;
}

/**
 * @brief Process the message from the client by splitting it into parts
 * @param clientIndex The index of the client in the clients array
*/
void Server::processMessage(int clientIndex) {
    this->clients[clientIndex]->messageParts = std::vector<std::string>();
    std::stringstream ss(this->clients[clientIndex]->message);
    std::string part;
    while (ss >> part) {
        this->clients[clientIndex]->messageParts.push_back(part);
    }
}

/**
 * @brief Decode the message from the client and perform the appropriate action
 * @param clientIndex The index of the client in the clients array
*/
void Server::decodeMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    (messageParts[0] == "msg") ? this->logProxyMessage(clientIndex)
        : (messageParts[0] == "rumor") ? this->logRumorMessage(clientIndex)
        : (messageParts[0] == "get" && messageParts[1] == "chatLog") ? this->sendChatLog(clientIndex)
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
    std::cout << "Receiving messages from client " << client->port << std::endl;

    /* Rcv messages from client until disconnected */
    int n;
    char buffer[256];
    bzero(buffer, 256);
    while ((n = recv(client->socket, buffer, 255, 0)) > 0) {
        client->message = buffer;
        std::cout << "Message: " << client->message << std::endl;
        this->processMessage(clientIndex);
        this->decodeMessage(clientIndex);
        bzero(buffer, 256);
    }

    /* The client has disconnected */
    if (n < 0) {
        this->closeClient(clientIndex);
    }
}


/*******************************************************************************
 *                           Server-Proxy interaction                          *
 * *****************************************************************************/

/**
 * @brief Add the received message from the proxy to the chat log
 * @param clientIndex The index of the client in the clients array
*/
void Server::logProxyMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    assert(messageParts.size() == 3 && "Error: msg should have 3 parts");

    std::string seq = messageParts[1];
    std::string message = messageParts[2];
    message = "from:" + std::to_string(this->port) + "," + "chatText:" + message + "," + "seqNo:" + seq;

    std::cout << "Adding message in logProxyMessage(): " << message << std::endl;

    RumorMessage rumor(message);
    std::unique_lock<std::shared_mutex> unique_lock(this->logMutex);
    this->status.addMessageToLog(rumor, std::stoi(seq));
    unique_lock.unlock();
}

/**
 * @brief Close the server and all client connections
*/
void Server::crashSequence() {
    std::cout << "Crashing..." << std::endl;
    this->closeAllConnections();
    this->destroyThreads();
    exit(0);
}

/**
 * @brief Send the chat log to the proxy
 * @param clientIndex The index of the client in the clients array
*/
void Server::sendChatLog(int cliendIndex) {
    /* Lock the logMutex when reading the chat log */
    std::shared_lock<std::shared_mutex> shared_lock(this->logMutex);
    std::vector<RumorMessage> chatLogMsgs = this->status.getChatLog();
    shared_lock.unlock();

    std::string chatLog = "chatLog";
    for (size_t i = 0; i < chatLogMsgs.size(); i++) {
        chatLog += (i > 0 ? "," : " ") + chatLogMsgs[i].getChatText();
    }
    chatLog += "\n";
    std::cout << "I am sending this chatLog: " << chatLog;

    const char* msg = chatLog.c_str();
    int messageLength = strlen(msg);

    constexpr int numClients = sizeof(this->clients) / sizeof(this->clients[0]);
    if (cliendIndex < 0 || cliendIndex >= numClients) {
        std::cerr << "Error: Invalid client index " << cliendIndex << std::endl;
        return;
    }

    int n = sendto(this->clients[cliendIndex]->socket, msg, messageLength, 0, (struct sockaddr*)&(this->clients[cliendIndex]->address), this->addressLength);

    if (n < 0) {
        std::cerr << "Error: Could not send data to proxy! " << std::endl;
        std::cerr << "errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
        this->closeClient(cliendIndex);
    }
}


/*******************************************************************************
 *                           Server-Server interaction                        *
 * *****************************************************************************/

/**
 * @brief Connect to the left neighbor and receive messages from it
*/
void Server::connectToLeftNeighbor() {
    /* We reconnect to the left neighbor if the connection is lost */
    while (true) {
        std::cout << "Connecting to left neighbor..." << std::endl;

        ClientInfo* left = new ClientInfo();
        left->socket = socket(AF_INET, SOCK_STREAM, 0);
        if (left->socket < 0) {
            std::cerr << "Error: Could not open socket" << std::endl;
            exit(1);
        }

        left->address.sin_family = AF_INET;
        left->address.sin_port = htons((this->port)-1);
        left->address.sin_addr.s_addr = INADDR_ANY;

        while (true) {
            if (connect(left->socket, (struct sockaddr *) &left->address, addressLength) < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                left->port = (this->port)-1;
                std::cout << "Connected to left neighbor " << (this->port)-1 << std::endl;
                break;
            }
        }

        /* Add left neighbor to zero index of clients array */
        this->clients[0] = left;
        this->rcvMessages(0);
    }
}

/**
 * @brief Perform anti-entropy by randomly sending status to a neighbor
*/
void Server::antiEntropy() {
    std::cout << "Starting anti-entropy..." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        int random = rand() % 2;
        std::cout << "Random: " << random << std::endl;
        if (this->clientConnected(random)) {
            this->sendStatus(random);
        }
    }
}


/**
 * @brief Add the received rumor message from a neighbor to the chat log
 * @param clientIndex The index of the client in the clients array
*/
void Server::logRumorMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    for (auto& part : messageParts) {
        std::cout << "Part: " << part << std::endl;
    }
    assert(messageParts.size() == 2 && "Error: rumor should have 2 parts");

    std::string message = messageParts[1];

    std::cout << "Adding message in logRumorMessage(): " << message << std::endl;

    RumorMessage rumor(message);
    std::unique_lock<std::shared_mutex> unique_lock(this->logMutex);
    this->status.addMessageToLog(rumor, rumor.getSeqNo());
    unique_lock.unlock();
}


/**
 * Helper method to parse incoming status message
 *
 * @param message Status msg payload in the form <port>:<seqNo>,<port>:<seqNo>,...
 * @return std::map status of the sender
 */
std::map<int, int> parseStatusMessage(const std::string& message) {
    std::map<int, int> statusMap;
    std::stringstream ss(message);
    std::string item;

    while (getline(ss, item, ',')) {
        std::stringstream itemStream(item);
        std::string keyStr, valueStr;

        if (getline(itemStream, keyStr, ':') && getline(itemStream, valueStr)) {
            int port = std::stoi(keyStr);
            int maxSeqNum = std::stoi(valueStr);
            statusMap[port] = maxSeqNum;
        }
    }
    return statusMap;
}

/**
 * Evaluates the status of a client relative to the server's message log to determine if the client
 * is behind or ahead. Depending on the client's status, the server may initiate rumor mongering to
 * update the client with newer messages, or respond with its own status to reconcile differences.
 * The method parses the client's status message, compares it with the server's status, and identifies
 * messages the client lacks. These messages are then sent to the client to ensure synchronization.
 *
 * @param clientIndex The index of the client in the server's client array, used to reference the client's
 *                    status message and to send necessary updates.
 */
void Server::checkStatus(int clientIndex) {
    std::string receivedStatusMsg = this->clients[clientIndex]->messageParts[1];
    std::map<int, int> receivedStatus = parseStatusMessage(receivedStatusMsg);

    std::shared_lock<std::shared_mutex> shared_lock(this->logMutex);
    std::map<int, int> ourStatus = this->status.getStatus();
    shared_lock.unlock();

    for (const auto& [port, ourSeqNo] : ourStatus) {
        auto it = receivedStatus.find(port);
        if (it == receivedStatus.end() || it->second < ourSeqNo) {
            for (int seqNo = (it == receivedStatus.end() ? 1 : it->second + 1); seqNo <= ourSeqNo; ++seqNo) {
                this->rumorMongering(port, seqNo, clientIndex);
            }
        }
    }
}


/**
 * @brief Send the status of the server to a neighbor client
 * @param index The index of the client in the clients array
*/
void Server::sendStatus(int index) {
    std::cout << "Sending status to " << this->clients[index]->port << std::endl;
    ClientInfo* client = this->clients[index];

    /* Create message for client */
    std::string statusMessage = "status ";
    int count = 0;
    std::shared_lock<std::shared_mutex> shared_lock(this->logMutex);
    std::map<int, int> status = this->status.getStatus();
    shared_lock.unlock();
    int size = status.size();
    for (const auto& pair : status) {
        statusMessage += std::to_string(pair.first) + ":" + std::to_string(pair.second);
        ++count;
        if (count < size) {
            statusMessage += ",";
        }
    }
    std::cout << "status_message that i'm sending is: " << statusMessage << std::endl;


    const char* message = statusMessage.c_str();
    int messageLength = strlen(message);

    int n = sendto(client->socket, message, messageLength, 0, (struct sockaddr*)&(client->address), this->addressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to " << client->port << std::endl;
        this->closeClient(index);
    }
}

/**
 * @brief Send a message to the client with the next message in the chat log
 * @param maxSeqNoRcvd The maximum sequence number with the client
 * @param clientIndex The index of the client in the clients array
*/
void Server::rumorMongering(int from, int seqNo, int clientIndex) {
    ClientInfo* client = this->clients[clientIndex];

    std::cout << "from: " << from << " seqNo: " << seqNo << std::endl;

    /* Lock the logMutex when reading the message */
    std::shared_lock<std::shared_mutex> shared_lock(this->logMutex);
    std::string m = this->status.getMessage(from, seqNo);
    shared_lock.unlock();

    /* Create message for client */
    std::string message = "rumor " + m;
    std::cout << "Sending message to " << client->port << ": " << message << std::endl;
    const char* msg = message.c_str();

    int n = sendto(client->socket, msg, strlen(msg), 0, (struct sockaddr*)&(client->address), this->addressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to " << client->port << std::endl;
        this->closeClient(clientIndex);
        return;
    }

    /* Message sent */
    std::cout << "Rumor mongering msg " << seqNo+1 << std::endl;
}
