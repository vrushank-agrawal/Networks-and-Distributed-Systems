#ifndef SERVER_CLASS
#define SERVER_CLASS

#include "libraries.hpp"
#include "messages.hpp"

std::map<int, int> parseStatusMessage(const std::string& message);

typedef struct ClientInfo {
    struct sockaddr_in address;     /**< Client address */
    int socket;                     /**< Client socket */
    int port;                       /**< Client port */
    std::string message;                  /**< Message from client */
    std::vector<std::string> messageParts;  /**< Message divided into 3 parts */
} ClientInfo;

typedef enum {
    FOLLOWER,
    CANDIDATE,
    LEADER
} ServerStatus;

class Server {
    public:
        Server(int pid, int totalServers, int port);
        ~Server();

        /**
         * @brief Start the server and bind it to the specified port
        */
        void start();

        /**
         * @brief Listen for clients to connect
        */
        void listenClient();

    private:
        /*****************************
         * Server variables          *
         * ***************************/
        int port;
        int totalServers;
        int pid;
        int serverSocket;
        socklen_t addressLength = sizeof(struct sockaddr_in);
        ClientInfo* clients[3];

        ServerStatus serverCurrentRole = FOLLOWER;
        int currLeaderIndex = 0;

        std::shared_mutex logMutex;
        StatusMessage status;
        std::vector<std::thread> threads;

        /*****************************
         * Methods                   *
         * ***************************/
        /**
         * @brief Destroy all threads
        */
        void destroyThreads();

        /*****************************
         * Server socket methods     *
         * ***************************/

        /**
         * @brief Accept a new client connection
        */
        ClientInfo* acceptClient();

        /**
         * @brief Add a client to the clients array
         * @param client The client to add
        */
        int addClient(ClientInfo* client);

        /**
         * @brief Check if a certain client is connected
        */
        bool clientConnected(int clientIndex);

        /**
         * @brief Close a client connection
        */
        void closeClient(int clientIndex);

        /**
         * @brief Close all client connections and the server
        */
        void closeAllConnections();

        /*****************************
         * Message handling methods  *
         * ***************************/

        /**
         * @brief Read a message from the newly accepted client
         * @param clientIndex The index of the client in the clients array
        */
        void readMessage(int clientIndex);

        /**
         * @brief Process the message from the client by splitting it into parts
         * @param clientIndex The index of the client in the clients array
        */
        void processMessage(int clientIndex);

        /**
         * @brief Decode the message from the client and perform the appropriate action
         * @param clientIndex The index of the client in the clients array
        */
        void decodeMessage(int clientIndex);

        /**
         * @brief Receive messages from a connected client untill disconnected
         * @param clientIndex The index of the client in the clients array
        */
        void rcvMessages(int clientIndex);

        /*****************************
         * Server-Proxy interaction *
         * ***************************/

        void sendRelayMessage(int clientIndex);

        void relayMessage(bool towardsLeader, int clientIndex);

        /**
         * @brief Add the received message from the proxy to the chat log
         * @param clientIndex The index of the client in the clients array
        */
        void logProxyMessage(int clientIndex);

        /**
         * @brief Close the server and all client connections
        */
        void crashSequence();

        /**
         * @brief Send the chat log to the proxy
         * @param clientIndex The index of the client in the clients array
        */
        void sendChatLog(int index);

        /*****************************
         * Server-Server interaction *
         * ***************************/

        /**
         * @brief Connect to the left neighbor and receive messages from it
        */
        void connectToLeftNeighbor();

        /**
         * @brief Send heartbeat message if you are leader
        */
        void heartbeat();

        /**
         * @brief Add the received rumor message from a neighbor to the chat log
         * @param clientIndex The index of the client in the clients array
        */
        void logRumorMessage(int clientIndex);

        /**
         * @brief Check the status of the server and send rumors to the client in ascending order of sequence numbers
         * @param clientIndex The index of the client in the client array
         */
        void checkStatus(int clientIndex);

        /**
         * @brief Send the status of the server to a neighbor client
         * @param index The index of the client in the clients array
        */
        void sendStatus(int clientIndex);

        /**
         * @brief Send a message to the client with the next message in the chat log
         * @param maxSeqNoRcvd The maximum sequence number with the client
         * @param clientIndex The index of the client in the clients array
        */
        void rumorMongering(int from, int maxSeqNoRcvd, int clientIndex);
};

#endif