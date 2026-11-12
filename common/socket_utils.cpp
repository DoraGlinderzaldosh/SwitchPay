#include <socket_utils.hpp>

#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <netdb.h>

std::expected<void, std::string> writeAll(int fd, const Bytes& bytes) {
    std::size_t sent = 0;

    while(sent < bytes.size()) {
        ssize_t n = send(fd, bytes.data()+sent, bytes.size()-sent, MSG_NOSIGNAL);

        if(n < 0) {
            if(errno == EINTR) continue;
            return std::unexpected(std::strerror(errno));
        }

        if(n == 0) return std::unexpected("Socket closed during write");

        sent += static_cast<std::size_t>(n);
    }

    return {};
}

//may be redundant in which case remove this / find an effecient alternative
std::expected<void, std::string> writeFrame (int fd, const Bytes& body) {
    if(body.empty() || body.size() > MAX_FRAME_SIZE)
    return std::unexpected("Invalid frame size");

    std::uint16_t length = static_cast<std::uint16_t>(body.size());

    Bytes frame;

    //again may be redundant 
    //create a helper function for this and use that here and multiple other places
    frame.reserve(body.size()+2);
    frame.push_back(static_cast<std::uint16_t>(length>>8));
    frame.push_back(static_cast<std::uint8_t>(length&0xFF));

    frame.insert(frame.end(), body.begin(), body.end());

    return writeAll(fd, frame);
}

std::expected<std::optional<Bytes>, std::string> readFrame(int fd, FrameBuffer& buffer) {
    while(true) {
        auto result = buffer.next();

        if(!result) return std::unexpected(result.error());
        if(result->has_value()) return std::move(result.value());
        Bytes incoming(1024);

        ssize_t n = read(fd, incoming.data(), incoming.size());
        if(n < 0) {
            if(errno == EINTR) continue;
            return std::unexpected(std::strerror(errno));
        }

        if(n == 0) {
            if(buffer.hasPendingData()) return std::unexpected("Connection closed with incomplete frame");
            return std::optional<Bytes>{};
        }

        incoming.resize(static_cast<std::size_t>(n));
        buffer.append(incoming);
    }
}

std::expected<int, std::string> connectTo(const std::string& host, const std::string& port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addresses = nullptr;
    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses);

    if(status != 0) return std::unexpected(gai_strerror(status));

    int fd = -1;
    std::string error = "Unable to connect";

    for(addrinfo* addr = addresses; addr != nullptr; addr = addr->ai_next) {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if(fd < 0) { 
            error = std::strerror(errno);
            continue;
        }

        if(connect(fd, addr->ai_addr, addr->ai_addrlen) == 0) {
            freeaddrinfo(addresses);
            return fd;
        }

        error = std::strerror(errno);
        close(fd);
        fd = -1;
    }

    freeaddrinfo(addresses);
    return std::unexpected(error);
}