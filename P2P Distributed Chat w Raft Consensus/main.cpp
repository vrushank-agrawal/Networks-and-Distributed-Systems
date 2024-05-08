#include "headers.hpp"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage:\n./process <pid> <nodes> <port>" << std::endl;
        return 1;
    }

    int port = atoi(argv[3]);
    int servers = atoi(argv[2]);

    Server server(servers, port);
    server.start();

    std::cout << "Server started on port " << port << std::endl;

    server.listenClient();

    return 0;
}