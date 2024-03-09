#ifndef SERVER_CLASS
#define SERVER_CLASS

#include "libraries.hpp"
#include "messages.hpp"

typedef struct ClientInfo {
    struct sockaddr_in address;
    int socket;
    int port;
    std::vector<std::string> messageParts;
    char* message;
} ClientInfo;

class Server {
    public:
        Server(int port);
        ~Server();
        void start();
        void listenClient();

    private:
        /* Data */
        int port;
        int serverSocket;
        socklen_t addressLength = sizeof(struct sockaddr_in);
        ClientInfo* clients[3];
        StatusMessage status;

        /* Server-Socket interaction */
        ClientInfo* acceptClient();
        int addClient(ClientInfo* client);
        void rcvMessages(int clientIndex);
        void closeClient(int clientIndex);
        void closeAllConnections();

        /* Message processing */
        void readMessage(int clientIndex);
        void processMessage(int clientIndex);
        void decodeMessage(int clientIndex);

        /* Server-Proxy interaction */
        void logMessage(int clientIndex);
        void crashSequence();
        void sendChatLog(int index);

        /* Server-Server interaction */
        void checkStatus(int clientIndex);
        void sendStatus(int clientIndex);
        void rumorMongering(int maxSeqNoRcvd, int clientIndex);
};

#endif