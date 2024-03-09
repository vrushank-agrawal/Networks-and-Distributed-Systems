#ifndef SERVER_CLASS
#define SERVER_CLASS

#include "libraries.hpp"
#include "messages.hpp"

class Server {
    private:
        int port;

        int serverSocket;
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(this->clientAddress);
        int clientSocket;
        int clientPort;

        std::vector<std::string> messageParts;
        StatusMessage status;

    public:
        Server(int port);
        ~Server();

        /* Server-Socket interaction */
        void start();
        void listenClient();
        void acceptClient();
        void readMessage();
        void decodeMessage();
        void closeConnection();

        /* Server-Proxy interaction */
        void logMessage();
        void crashSequence();
        void sendChatLog();

        /* Server-Server interaction */
        void checkStatus();
        void sendStatus();
        void rumorMongering(int maxSeqNoRcvd);
};

#endif