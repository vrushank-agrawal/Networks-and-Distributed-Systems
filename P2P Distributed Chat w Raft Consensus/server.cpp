#include "headers.hpp"

/**
 * @brief Construct a new Server object
 * @param port The port to bind the server to
*/
Server::Server(int pid, int totalServers, int port) {
    this->port = port;
    this->totalServers = totalServers;
    this->pid = pid;
    if (pid == 0) this->serverCurrentRole = LEADER;

    printf("Server created with port=%d\n", port);
    printf("Total servers=%d\n", totalServers);
    printf("Your role is %s\n", (this->serverCurrentRole == LEADER) ? "LEADER" : "FOLLOWER");

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

void Server::listenClient() {
    /* Start thread to connect to left neighbor and detach it */
    std::thread temp = std::thread(&Server::connectToLeftNeighbor, this);
    temp.detach();
    this->threads.emplace_back(std::move(temp));

    /* Start thread to perform anti-entropy and detach it */
    // std::thread antiEntropyThread(&Server::antiEntropy, this);
    // antiEntropyThread.detach();
    // this->threads.emplace_back(std::move(antiEntropyThread));

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
        if (clientIndex == -1) continue;
        this->readMessage(clientIndex);
        std::cout << "accepted, added, read 1st msg from " << this->clients[clientIndex]->port << std::endl;

        // proxy
        if (this->clients[clientIndex]->port != this->port+1) {
            printf("Rcvd from port number %d\n", this->clients[clientIndex]->port);
            this->processMessage(clientIndex);
            this->decodeMessage(clientIndex);
        }
        // right neighbor
        else if (this->clients[2]->port == this->port+1) {
            std::cout << "Swapping clients[1] and clients[2]" << std::endl;
            ClientInfo* temp = this->clients[1];
            this->clients[1] = this->clients[2];
            this->clients[2] = temp;
            clientIndex = 1;
        }

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

void Server::destroyThreads() {
    for (auto& thread : this->threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

ClientInfo* Server::acceptClient() {
    ClientInfo* client = new ClientInfo();
    client->socket = accept(this->serverSocket, (struct sockaddr *) &(client->address), &(this->addressLength));
    if (client->socket < 0) {
        std::cerr << "Error: Could not accept client" << std::endl;
        exit(1);
    }
    std::cout << "Accepted new client at socket=" << client->socket << std::endl;
    client->port = ntohs(client->address.sin_port);
    return client;
}

int Server::addClient(ClientInfo* client) {
    std::cout << "Received initial conn req from " << client->port << std::endl;

    if (this->clients[2] == nullptr) {
        this->clients[2] = client;
        std::cout << "First conn req, setting it as \"Proxy\"" << std::endl;
        return 2;
    } else if (this->clients[1] == nullptr) {
        std::cout << "Second conn req, setting it as \"Right\"" << std::endl;
        this->clients[1] = client;
        return 1;
    } else {
        std::cout << "Rejecting connection!!" << std::endl;
        return -1;
    }
}

bool Server::clientConnected(int clientIndex) {
    return this->clients[clientIndex] != nullptr;
}

void Server::closeClient(int clientIndex) {
    std::cout << "Closing client connection " << this->clients[clientIndex]->port << std::endl;
    close(this->clients[clientIndex]->socket);
    this->clients[clientIndex] = nullptr;
}

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

void Server::readMessage(int clientIndex) {
    char buffer[256];
    bzero(buffer, 256);
    int n = read(this->clients[clientIndex]->socket, buffer, 255);
    if (n == 0 && clientIndex == 2) {
        std::cout << "Proxy disconnected so I'm exiting too!" << std::endl;
        exit(0);
    }
    if (n < 0) {
        std::cerr << "Error: Could not read from socket" << std::endl;
        exit(1);
    }
    std::cout << "from port=" << this->clients[clientIndex]->port << ", msg rcvd=" << buffer << std::endl;
    this->clients[clientIndex]->message = std::string(buffer, n);

    // check for whoami message it must startwith "whoami "
    if (this->clients[clientIndex]->message.substr(0, 7) == "whoami ") {
        int whoami_port = std::stoi(this->clients[clientIndex]->message.substr(7, 12));
        std::cout << "Received whoami message from port=" <<  whoami_port << std::endl;
        this->clients[clientIndex]->port = whoami_port;
        this->clients[clientIndex]->address.sin_port = ntohs(whoami_port);
    }

    std::cout << "Thread " << std::this_thread::get_id() << " reading message from client " << clientIndex << ": " << this->clients[clientIndex]->message << std::endl;
}

void Server::processMessage(int clientIndex) {
    this->clients[clientIndex]->messageParts = std::vector<std::string>();
    std::stringstream ss(this->clients[clientIndex]->message);
    std::string part;
    while (ss >> part) {
        this->clients[clientIndex]->messageParts.push_back(part);
    }
}

void Server::decodeMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    (messageParts[0] == "msg") ? this->logProxyMessage(clientIndex)
        // : (messageParts[0] == "rumor") ? this->logRumorMessage(clientIndex)
        : (messageParts[0] == "get" && messageParts[1] == "chatLog") ? this->sendChatLog(clientIndex)
        // : (messageParts[0] == "status") ? this->checkStatus(clientIndex)
        : (messageParts[0] == "crash") ? this->crashSequence()
        : void(std::cerr << "Error: Unknown message type" << std::endl);
}

void Server::rcvMessages(int clientIndex) {
    ClientInfo* client = this->clients[clientIndex];
    std::cout << "Receiving messages from client " << client->port << std::endl;

    /* Rcv messages from client until disconnected */
    int n;
    char buffer[256];
    bzero(buffer, 256);
    while ((n = recv(client->socket, buffer, 255, 0)) > 0) {
        client->message = buffer;
        std::cout << "msg rcvd: " << client->message << std::endl;
        this->processMessage(clientIndex);
        this->decodeMessage(clientIndex);
        bzero(buffer, 256);
    }

    /* The client has disconnected */
    if (n == 0) {
        std::cout << "Gracefully dumped :). port=" << this->clients[clientIndex]->port << std::endl;
        if (clientIndex == 2) {
            std::cout << "Proxy disconnected so I'm exiting too!" << std::endl;
            exit(0);
        }
        this->closeClient(clientIndex);
    }
    if (n < 0) {
        std::cout << "Badly dumped :(. port= " <<  this->clients[clientIndex]->port << std::endl;
        this->closeClient(clientIndex);
    }
}


/*******************************************************************************
 *                           Server-Proxy interaction                          *
 * *****************************************************************************/

void Server::sendRelayMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    // aggregate the message parts and simply send it to the socket of the client index as one message
    std::string message = "";
    for (size_t i = 1; i < messageParts.size(); i++) {
        message += messageParts[i] + " ";
    }
    message += "\n";
    const char* msg = message.c_str();
    int messageLength = strlen(msg);

    int n = sendto(this->clients[clientIndex]->socket, msg, messageLength, 0, (struct sockaddr*)&(this->clients[clientIndex]->address), this->addressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to proxy! " << std::endl;
        std::cerr << "errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
        this->closeClient(clientIndex);
    }
}

void Server::relayMessage(bool towardsLeader, int clientIndex) {
    if (this->serverCurrentRole == CANDIDATE) {
        // idk
    } else if (this->serverCurrentRole == FOLLOWER) {
        // send to leader
        if (towardsLeader) {
            if (this->currLeaderIndex > this->pid) {
                // foward to right
                printf("Forwarding message to right\n");
                this->sendRelayMessage(1);
            } else {
                // forward to left
                printf("Forwarding message to left\n");
                this->sendRelayMessage(0);
            }
        } else {
            if (this->currLeaderIndex > this->pid) {
                // forward to left
                printf("Forwarding message to left\n");
                this->sendRelayMessage(0);
            } else {
                // forward to right
                printf("Forwarding message to right\n");
                this->sendRelayMessage(1);
            }
        }

    } else {
        // error
    }
}

void Server::logProxyMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    assert(messageParts.size() == 3 && "Error: msg should have 3 parts");


    if (this->serverCurrentRole == LEADER) {
        // start commit process
        printf("Leader received message from %d\n", this->clients[clientIndex]->port);
    } else {
        // relay message to leader
        this->relayMessage(true, clientIndex);
    }


    // message = "from:" + std::to_string(this->port) + "," + "chatText:" + message + "," + "seqNo:" + seq;

    // std::cout << "Adding message in logProxyMessage(): " << message << std::endl;

    // RumorMessage rumor(message);


    // std::unique_lock<std::shared_mutex> unique_lock(this->logMutex);
    // this->status.addMessageToLog(rumor, std::stoi(seq));
    // unique_lock.unlock();
}

void Server::crashSequence() {
    std::cout << "Crashing..." << std::endl;
    this->closeAllConnections();
    this->destroyThreads();
    exit(0);
}

void Server::sendChatLog(int cliendIndex) {
    /* Lock the logMutex when reading the chat log */
    std::shared_lock<std::shared_mutex> shared_lock(this->logMutex);
    std::vector<RumorMessage> chatLogMsgs = this->status.getChatLog();
    shared_lock.unlock();

    std::string chatLog = "chatLog";
    for (size_t i = 0; i < chatLogMsgs.size(); i++) {
        chatLog += (i > 0 ? "," : " ") + chatLogMsgs[i].getChatText();
    }
    if (chatLogMsgs.size() == 0) {
        chatLog += " \x01";
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

                // send a message "whoami <my_port>"
                std::string whoami = "whoami " + std::to_string(this->port);
                const char* msg = whoami.c_str();
                int messageLength = strlen(msg);
                int n = sendto(left->socket, msg, messageLength, 0, (struct sockaddr*)&(left->address), this->addressLength);
                std::cout << "Sending whoami message to " << left->port << std::endl;

                if (n < 0) {
                    std::cerr << "Error: Could not send data to " << left->port << std::endl;
                    this->closeClient(0);
                }
                break;
            }
        }

        /* Add left neighbor to zero index of clients array */
        this->clients[0] = left;
        this->rcvMessages(0);
    }
}

void Server::heartbeat() {
    // confirm that I am the leader


    std::cout << "Starting heartbeat..." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        int random = rand() % 2;
        if (this->clientConnected(random)) {
            this->sendStatus(random);
        }
    }
}


void Server::logRumorMessage(int clientIndex) {
    std::vector<std::string> messageParts = this->clients[clientIndex]->messageParts;
    assert(messageParts.size() == 2 && "Error: rumor should have 2 parts");

    std::string message = messageParts[1];

    std::cout << "Adding message in logRumorMessage(): " << message << std::endl;

    RumorMessage rumor(message);
    std::unique_lock<std::shared_mutex> unique_lock(this->logMutex);
    this->status.addMessageToLog(rumor, rumor.getSeqNo());
    unique_lock.unlock();
}


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
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    }
}


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
    std::cout << "sending status: " << statusMessage << std::endl;


    const char* message = statusMessage.c_str();
    int messageLength = strlen(message);

    int n = sendto(client->socket, message, messageLength, 0, (struct sockaddr*)&(client->address), this->addressLength);
    if (n < 0) {
        std::cerr << "Error: Could not send data to " << client->port << std::endl;
        this->closeClient(index);
    }
}

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
