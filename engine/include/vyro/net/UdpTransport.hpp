// VyroEngine — UDP transport (V2.2)
// A real socket implementation of ITransport over POSIX UDP. Non-blocking:
// receive() drains at most one datagram per call and never waits. The
// LoopbackTransport remains the deterministic test double; this backend
// carries the same snapshot packets across real networks.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/net/Transport.hpp"

#include <string>
#include <string_view>

namespace vyro {

class UdpTransport final : public ITransport
{
public:
    UdpTransport() = default;
    ~UdpTransport() override;

    UdpTransport(const UdpTransport&) = delete;
    UdpTransport& operator=(const UdpTransport&) = delete;

    // Bind the local socket. Port 0 picks an ephemeral port. Returns false on
    // socket/bind failure.
    bool bind(u16 port);

    // Set the peer all sends go to.
    bool set_peer(std::string_view host, u16 peer_port);

    [[nodiscard]] u16 local_port() const { return m_local_port; }
    [[nodiscard]] bool is_open() const { return m_socket >= 0; }

    // ITransport
    void send(const Packet& packet) override;
    [[nodiscard]] bool receive(Packet& out) override;
    [[nodiscard]] usize pending() const override;

private:
    int m_socket = -1;
    u16 m_local_port = 0;
    std::string m_peer_host;
    u16 m_peer_port = 0;
    bool m_has_peer = false;
};

} // namespace vyro
