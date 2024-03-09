#ifndef SERVER_CLASS
#define SERVER_CLASS

#include "libraries.hpp"
#include "messages.hpp"

class Server {
    private:
        int port;

        int serverSocket;
        int clientSocket;
        int clientPort;

        char* clientMessage;
        StatusMessage status;

    public:
        Server(int port);
        ~Server();
        void start();
        void listenClient();
        void acceptClient();
        void readMessage();
        void updateStatus();
};

#endif