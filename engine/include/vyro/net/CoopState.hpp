// VyroEngine — Co-op shared world state (V6.3)
// V4.4 replicated only the two players' avatar positions. Co-op *gameplay*
// needs the authoritative host to share the world: score, wave, player health,
// and the horde. CoopWorldState is that snapshot; CoopStateChannel sends/receives
// it over its own transport (a separate channel from the avatar sync), keeping
// only the newest sequence. Deterministic wire format, headless and tested.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"
#include "vyro/net/Transport.hpp"

#include <cstring>
#include <vector>

namespace vyro {

struct CoopWorldState {
    u32 sequence = 0;
    i32 score = 0;
    i32 wave = 0;
    i32 host_hp = 0;
    std::vector<Vec3> horde; // authoritative zombie world positions
};

namespace coopstate {

namespace detail {
template<typename T>
inline void write(Packet& out, const T& v)
{
    const auto* p = reinterpret_cast<const byte*>(&v);
    out.insert(out.end(), p, p + sizeof(T));
}
template<typename T>
inline T read(const Packet& in, usize& off)
{
    T v{};
    if (off + sizeof(T) <= in.size()) {
        std::memcpy(&v, in.data() + off, sizeof(T));
        off += sizeof(T);
    }
    return v;
}
} // namespace detail

[[nodiscard]] inline Packet encode(const CoopWorldState& s)
{
    Packet out;
    detail::write(out, s.sequence);
    detail::write(out, s.score);
    detail::write(out, s.wave);
    detail::write(out, s.host_hp);
    detail::write(out, static_cast<u32>(s.horde.size()));
    for (const Vec3& p : s.horde) {
        detail::write(out, p.x);
        detail::write(out, p.y);
        detail::write(out, p.z);
    }
    return out;
}

[[nodiscard]] inline CoopWorldState decode(const Packet& packet)
{
    CoopWorldState s;
    usize off = 0;
    s.sequence = detail::read<u32>(packet, off);
    s.score = detail::read<i32>(packet, off);
    s.wave = detail::read<i32>(packet, off);
    s.host_hp = detail::read<i32>(packet, off);
    const u32 count = detail::read<u32>(packet, off);
    s.horde.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        Vec3 p;
        p.x = detail::read<f32>(packet, off);
        p.y = detail::read<f32>(packet, off);
        p.z = detail::read<f32>(packet, off);
        s.horde.push_back(p);
    }
    return s;
}

} // namespace coopstate

// Host sends world snapshots; joiner polls and keeps the newest.
class CoopStateChannel
{
public:
    explicit CoopStateChannel(ITransport& transport) : m_transport(transport) {}

    // Send `state` with the next sequence number.
    void send(CoopWorldState state)
    {
        state.sequence = ++m_send_seq;
        m_transport.send(coopstate::encode(state));
    }

    // Drain incoming snapshots, keeping the newest; returns the number applied.
    u32 poll()
    {
        u32 applied = 0;
        Packet packet;
        while (m_transport.receive(packet)) {
            CoopWorldState s = coopstate::decode(packet);
            if (s.sequence <= m_latest.sequence) {
                continue; // stale / out of order
            }
            m_latest = std::move(s);
            ++applied;
        }
        return applied;
    }

    [[nodiscard]] bool connected() const { return m_latest.sequence != 0; }
    [[nodiscard]] const CoopWorldState& latest() const { return m_latest; }

private:
    ITransport& m_transport;
    u32 m_send_seq = 0;
    CoopWorldState m_latest;
};

} // namespace vyro
