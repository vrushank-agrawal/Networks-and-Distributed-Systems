#include "headers.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);

    Server server(port);
    server.start();

    std::cout << "Server started on port " << port << std::endl;

    server.listenClient();

    return 0;
}