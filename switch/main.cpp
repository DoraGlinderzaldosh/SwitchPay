#include "socket_utils.hpp"
#include "message.hpp"
#include "codec.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

void handleClient(int client_fd) {
    FrameBuffer buffer;

    while(true) {
        auto request = readFrame(client_fd, buffer);
        if(!request) {
            cerr << "Client read error: " << request.error() << '\n';
            return;
        }

        if(!request.value().has_value()) {
            cout << "Client disconnected\n";
            return;
        }

        cout << "Request received from client\n";

        auto issuer = connectTo("127.0.0.1", "9000");
        if(!issuer) {
            cerr << "Issuer connection failed: " << issuer.error() << '\n';
            return;
        }

        int issuer_fd = *issuer;
        auto sent = writeFrame(issuer_fd, *request.value());
        if(!sent) {
            cerr << "Forwarding failed: " << sent.error() << '\n';
            close(issuer_fd);
            return;
        }

        FrameBuffer issuer_buffer;
        auto response = readFrame(issuer_fd, issuer_buffer);

        if(!response) {
            cerr << "Issuer read error: "<< response.error() << '\n';
            close(issuer_fd);
            return;
        }

        if(!response.value().has_value()) {
            cerr << "Issuer disconnected without responding\n";
            close(issuer_fd);
            return;
        }
        close(issuer_fd);

        auto result = writeFrame(client_fd, *response.value());

        if(!result) {
            cerr << "Client write error: "<< result.error() << '\n';
            return;
        }

        cout << "Response forwarded to client\n";
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
    address.sin_port = htons(8000);
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

    cout << "Switch listening on 127.0.0.1:8000\n";

    while(true) {
        int client_fd = accept(server_fd, nullptr, nullptr);

        if(client_fd < 0) {
            if(errno == EINTR) continue;

            perror("accept");
            continue;
        }

        cout << "New client connected\n";

        thread worker(handleClient, client_fd);
        worker.detach();
    }

    close(server_fd);
    return 0;
}