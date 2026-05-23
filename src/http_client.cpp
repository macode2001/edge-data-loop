#include "http_client.h"

#include <cstring>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

namespace {

void close_socket(SocketHandle socket_fd) {
#ifdef _WIN32
    closesocket(socket_fd);
#else
    close(socket_fd);
#endif
}

class SocketRuntime {
public:
    SocketRuntime() {
#ifdef _WIN32
        WSADATA data;
        started_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#endif
    }

    ~SocketRuntime() {
#ifdef _WIN32
        if (started_) {
            WSACleanup();
        }
#endif
    }

    bool ok() const {
#ifdef _WIN32
        return started_;
#else
        return true;
#endif
    }

private:
    bool started_ = true;
};

bool send_all(SocketHandle socket_fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
#ifdef _WIN32
        const int n = send(socket_fd, data.data() + sent, static_cast<int>(data.size() - sent), 0);
#else
        const ssize_t n = send(socket_fd, data.data() + sent, data.size() - sent, 0);
#endif
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

int parse_status_code(const std::string& response) {
    std::istringstream in(response);
    std::string http_version;
    int status = 0;
    in >> http_version >> status;
    return status;
}

}  // namespace

HttpResponse HttpClient::post_json(const std::string& host,
                                   int port,
                                   const std::string& path,
                                   const std::string& json_body) const {
    SocketRuntime runtime;
    if (!runtime.ok()) {
        HttpResponse response;
        response.error = "socket runtime initialization failed";
        return response;
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* address_list = nullptr;
    const auto port_text = std::to_string(port);
    const int resolved = getaddrinfo(host.c_str(), port_text.c_str(), &hints, &address_list);
    if (resolved != 0 || address_list == nullptr) {
        HttpResponse response;
        response.error = "failed to resolve host";
        return response;
    }

    SocketHandle socket_fd = kInvalidSocket;
    for (addrinfo* item = address_list; item != nullptr; item = item->ai_next) {
        socket_fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (socket_fd == kInvalidSocket) {
            continue;
        }
        if (connect(socket_fd, item->ai_addr, static_cast<int>(item->ai_addrlen)) == 0) {
            break;
        }
        close_socket(socket_fd);
        socket_fd = kInvalidSocket;
    }
    freeaddrinfo(address_list);

    if (socket_fd == kInvalidSocket) {
        HttpResponse response;
        response.error = "failed to connect server";
        return response;
    }

    std::ostringstream request;
    request << "POST " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << json_body.size() << "\r\n";
    request << "Connection: close\r\n\r\n";
    request << json_body;

    if (!send_all(socket_fd, request.str())) {
        close_socket(socket_fd);
        HttpResponse response;
        response.error = "failed to send request";
        return response;
    }

    std::string response;
    char buffer[4096];
    while (true) {
#ifdef _WIN32
        const int n = recv(socket_fd, buffer, sizeof(buffer), 0);
#else
        const ssize_t n = recv(socket_fd, buffer, sizeof(buffer), 0);
#endif
        if (n <= 0) {
            break;
        }
        response.append(buffer, buffer + n);
    }
    close_socket(socket_fd);

    const int status = parse_status_code(response);
    HttpResponse result;
    result.ok = status >= 200 && status < 300;
    result.status_code = status;
    result.body = response;
    return result;
}
