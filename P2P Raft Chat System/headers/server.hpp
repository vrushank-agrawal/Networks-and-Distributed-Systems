#ifndef SERVER_CLASS
#define SERVER_CLASS

#include "libraries.hpp"
#include "messages.hpp"

typedef struct ClientInfo {
    struct sockaddr_in address;     /**< Client address */
    int socket;                     /**< Client socket */
    int port;                       /**< Client port */
    std::string message;                  /**< Message from client */
    std::vector<std::string> messageParts;  /**< Message divided into 3 parts */
} ClientInfo;

class Server {
    public:
        Server(int port);
        ~Server();
        void start();
        void listenClient();

    private:
        /*****************************
         * Server variables          *
         * ***************************/
        int port;
        int serverSocket;
        socklen_t addressLength = sizeof(struct sockaddr_in);
        ClientInfo* clients[3];

        std::shared_mutex logMutex;
        StatusMessage status;
        std::vector<std::thread> threads;

        /*****************************
         * Methods                   *
         * ***************************/
        void destroyThreads();

        /*****************************
         * Server socket methods     *
         * ***************************/
        ClientInfo* acceptClient();
        int addClient(ClientInfo* client);
        bool clientConnected(int clientIndex);
        void closeClient(int clientIndex);
        void closeAllConnections();

        /*****************************
         * Message handling methods  *
         * ***************************/
        void readMessage(int clientIndex);
        void processMessage(int clientIndex);
        void decodeMessage(int clientIndex);
        void rcvMessages(int clientIndex);

        /*****************************
         * Server-Proxy interaction *
         * ***************************/
        void logProxyMessage(int clientIndex);
        void crashSequence();
        void sendChatLog(int index);

        /*****************************
         * Server-Server interaction *
         * ***************************/
        void connectToLeftNeighbor();
        void antiEntropy();
        void logRumorMessage(int clientIndex);
        void checkStatus(int clientIndex);
        void sendStatus(int clientIndex);
        void rumorMongering(int from, int maxSeqNoRcvd, int clientIndex);
};

#endif