// VyroEngine — State replication
// Phase 9.2/9.3: world snapshots and client synchronization. The server
// serializes entity positions into sequenced snapshots; the client stores the
// two most recent and interpolates between them to mask network jitter.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"
#include "vyro/net/Transport.hpp"

#include <unordered_map>

namespace vyro {

using NetEntityId = u32;

struct Snapshot {
    u32 sequence = 0;
    std::unordered_map<NetEntityId, Vec3> positions;
};

namespace replication {

// Wire encode/decode (deterministic little-endian layout).
[[nodiscard]] Packet encode(const Snapshot& snapshot);
[[nodiscard]] Snapshot decode(const Packet& packet);

} // namespace replication

// Authoritative side (9.1): owns canonical entity state, broadcasts snapshots.
class NetServer
{
public:
    explicit NetServer(ITransport& transport) : m_transport(transport) {}

    void set_position(NetEntityId id, Vec3 position) { m_state.positions[id] = position; }
    [[nodiscard]] Vec3 position(NetEntityId id) const
    {
        const auto it = m_state.positions.find(id);
        return it != m_state.positions.end() ? it->second : Vec3{};
    }

    // Send the current world snapshot with the next sequence number.
    void broadcast();

    [[nodiscard]] u32 sequence() const { return m_state.sequence; }

private:
    ITransport& m_transport;
    Snapshot m_state;
};

// Replica side (9.3): receives snapshots, interpolates between the last two.
class NetClient
{
public:
    explicit NetClient(ITransport& transport) : m_transport(transport) {}

    // Drain incoming snapshots; stale (out-of-order) ones are dropped.
    // Returns the number of snapshots applied.
    u32 poll();

    // Interpolated position at t in [0,1] between the previous and latest
    // snapshot (entity interpolation).
    [[nodiscard]] Vec3 position(NetEntityId id, f32 t = 1.0f) const;

    [[nodiscard]] u32 latest_sequence() const { return m_latest.sequence; }
    [[nodiscard]] usize known_entities() const { return m_latest.positions.size(); }

private:
    ITransport& m_transport;
    Snapshot m_previous;
    Snapshot m_latest;
};

} // namespace vyro
