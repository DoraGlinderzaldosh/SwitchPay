#include "socket_utils.hpp"

#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <netdb.h>

expected<void, string> writeAll(int fd, const Bytes& bytes) {
    size_t sent = 0;

    while(sent < bytes.size()) {
        ssize_t n = send(fd, bytes.data()+sent, bytes.size()-sent, MSG_NOSIGNAL);

        if(n < 0) {
            if(errno == EINTR) continue;
            return unexpected(strerror(errno));
        }

        if(n == 0) return unexpected("Socket closed during write");

        sent += static_cast<size_t>(n);
    }

    return {};
}

//may be redundant in which case remove this / find an effecient alternative
expected<void, string> writeFrame (int fd, const Bytes& body) {
    if(body.empty() || body.size() > MAX_FRAME_SIZE)
    return unexpected("Invalid frame size");

    uint16_t length = static_cast<uint16_t>(body.size());

    Bytes frame;

    //again may be redundant 
    //create a helper function for this and use that here and multiple other places
    frame.reserve(body.size()+2);
    frame.push_back(static_cast<uint8_t>(length>>8));
    frame.push_back(static_cast<uint8_t>(length&0xFF));

    frame.insert(frame.end(), body.begin(), body.end());

    return writeAll(fd, frame);
}

expected<optional<Bytes>, string> readFrame(int fd, FrameBuffer& buffer) {
    while(true) {
        auto result = buffer.next();

        if(!result) return unexpected(result.error());
        if(result->has_value()) return move(result.value());
        Bytes incoming(1024);

        ssize_t n = read(fd, incoming.data(), incoming.size());
        if(n < 0) {
            if(errno == EINTR) continue;
            return unexpected(strerror(errno));
        }

        if(n == 0) {
            if(buffer.hasPendingData()) return unexpected("Connection closed with incomplete frame");
            return optional<Bytes>{};
        }

        incoming.resize(static_cast<size_t>(n));
        buffer.append(incoming);
    }
}

expected<int, string> connectTo(const string& host, const string& port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addresses = nullptr;
    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses);

    if(status != 0) return unexpected(gai_strerror(status));

    int fd = -1;
    string error = "Unable to connect";

    for(addrinfo* addr = addresses; addr != nullptr; addr = addr->ai_next) {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if(fd < 0) { 
            error = strerror(errno);
            continue;
        }

        if(connect(fd, addr->ai_addr, addr->ai_addrlen) == 0) {
            freeaddrinfo(addresses);
            return fd;
        }

        error = strerror(errno);
        close(fd);
        fd = -1;
    }

    freeaddrinfo(addresses);
    return unexpected(error);
}