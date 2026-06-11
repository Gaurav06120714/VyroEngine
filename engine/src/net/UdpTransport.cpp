// VyroEngine — UDP transport implementation
#include "vyro/net/UdpTransport.hpp"

#include "vyro/core/Log.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

namespace vyro {

namespace {
constexpr usize kMaxDatagram = 65507; // max UDP payload
} // namespace

UdpTransport::~UdpTransport()
{
    if (m_socket >= 0) {
        ::close(m_socket);
    }
}

bool UdpTransport::bind(u16 port)
{
    m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        VYRO_ERROR("Net", "socket() failed");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (::bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        VYRO_ERROR("Net", "bind({}) failed", port);
        ::close(m_socket);
        m_socket = -1;
        return false;
    }

    // Discover the actual port (relevant when binding port 0).
    socklen_t len = sizeof(addr);
    if (::getsockname(m_socket, reinterpret_cast<sockaddr*>(&addr), &len) == 0) {
        m_local_port = ntohs(addr.sin_port);
    }

    // Non-blocking receive.
    const int flags = ::fcntl(m_socket, F_GETFL, 0);
    ::fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

    VYRO_INFO("Net", "UDP bound on port {}", m_local_port);
    return true;
}

bool UdpTransport::set_peer(std::string_view host, u16 peer_port)
{
    m_peer_host = std::string(host);
    m_peer_port = peer_port;

    sockaddr_in probe{};
    if (::inet_pton(AF_INET, m_peer_host.c_str(), &probe.sin_addr) != 1) {
        VYRO_ERROR("Net", "invalid peer address '{}'", m_peer_host);
        m_has_peer = false;
        return false;
    }
    m_has_peer = true;
    return true;
}

void UdpTransport::send(const Packet& packet)
{
    if (m_socket < 0 || !m_has_peer || packet.size() > kMaxDatagram) {
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_peer_port);
    ::inet_pton(AF_INET, m_peer_host.c_str(), &addr.sin_addr);
    ::sendto(m_socket, packet.data(), packet.size(), 0,
             reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

bool UdpTransport::receive(Packet& out)
{
    if (m_socket < 0) {
        return false;
    }
    out.resize(kMaxDatagram);
    const ssize_t n = ::recvfrom(m_socket, out.data(), out.size(), 0, nullptr, nullptr);
    if (n <= 0) {
        out.clear();
        return false;
    }
    out.resize(static_cast<usize>(n));
    return true;
}

usize UdpTransport::pending() const
{
    if (m_socket < 0) {
        return 0;
    }
    int bytes = 0;
    if (::ioctl(m_socket, FIONREAD, &bytes) < 0 || bytes <= 0) {
        return 0;
    }
    return 1; // at least one datagram is readable
}

} // namespace vyro
