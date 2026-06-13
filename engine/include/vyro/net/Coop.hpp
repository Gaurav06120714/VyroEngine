// VyroEngine — Co-op peer link (V4.4)
// NetServer/NetClient model a one-way authoritative flow (server broadcasts,
// client interpolates). Peer co-op is symmetric: each side both publishes its
// own avatar and replicates the other's. CoopLink bundles a NetServer and a
// NetClient over one bidirectional transport so each peer runs the same code:
// set the local avatar, broadcast(), poll(), then read the peer's interpolated
// position. Works over UdpTransport for real play and LoopbackTransport in
// tests, since both deliver what the other endpoint sends.
#pragma once

#include "vyro/net/Replication.hpp"
#include "vyro/net/Transport.hpp"

namespace vyro {

class CoopLink
{
public:
    // local_id is this peer's avatar id; peer_id is the id we replicate from
    // the other side. The two peers swap these (host 1/2, joiner 2/1).
    CoopLink(ITransport& transport, NetEntityId local_id, NetEntityId peer_id)
        : m_server(transport), m_client(transport), m_local_id(local_id), m_peer_id(peer_id)
    {
    }

    // Stage this frame's local avatar position.
    void set_local_position(Vec3 position) { m_server.set_position(m_local_id, position); }

    // Publish the staged local avatar to the peer.
    void broadcast() { m_server.broadcast(); }

    // Drain incoming snapshots; returns the number applied.
    u32 poll() { return m_client.poll(); }

    // Has the peer reported its avatar yet?
    [[nodiscard]] bool peer_connected() const { return m_client.has(m_peer_id); }

    // Peer avatar position, interpolated between the last two snapshots.
    [[nodiscard]] Vec3 peer_position(f32 t = 1.0f) const { return m_client.position(m_peer_id, t); }

    [[nodiscard]] NetEntityId local_id() const { return m_local_id; }
    [[nodiscard]] NetEntityId peer_id() const { return m_peer_id; }

private:
    NetServer m_server;
    NetClient m_client;
    NetEntityId m_local_id;
    NetEntityId m_peer_id;
};

} // namespace vyro
