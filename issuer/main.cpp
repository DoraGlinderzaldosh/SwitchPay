#include "socket_utils.hpp"
#include "codec.hpp"
#include "message.hpp"
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void handleClient(int client_fd) {
    FrameBuffer buffer;
    
    while(true) {
        auto frame = readFrame(client_fd, buffer);

        if(!frame) {
            cerr << "Read error: " << frame.error() << '\n';
            return;
        }

        if(!frame.value().has_value()) {
            cout << "Client disconnected\n";
            return;
        }

        auto request = decode(*frame.value());
        if(!request) {
            cerr << "Decode error: " << request.error() << '\n';
            return;
        }

        cout << "Authorization request received\n";

        if(request->type != 0x0100) {
            cerr << "Unexpected message type/n";
            return;
        }

        Message response{};
        response.type = 0x0110;
        response.stan = request->stan;
        response.response = "00";

        auto encoded = encode(response);
        if(!encoded) {
            cerr << "Encode error: " << encoded.error() << '\n';
            return;
        }

        auto result = writeFrame(client_fd, *encoded);
        if(!result) {
            cerr << "Write error: " << result.error() << '\n';
            return;
        }

        cout << "Approval sent for STAN: " << request->stan.value() << '\n';
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(9000);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if(listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    cout << "Issuer listening on 127.0.0.1:9000\n";

    while(true) {
        int client_fd = accept(server_fd, nullptr, nullptr);

        if(client_fd < 0) {
            if(errno == EINTR) continue;

            perror("accept");
            continue;
        }

        cout << "Switch connected\n";

        handleClient(client_fd);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}