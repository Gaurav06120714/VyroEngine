// VyroEngine — State replication implementation
#include "vyro/net/Replication.hpp"

#include <cstring>

namespace vyro {

namespace replication {

namespace {

template<typename T>
void write(Packet& out, const T& value)
{
    const auto* p = reinterpret_cast<const byte*>(&value);
    out.insert(out.end(), p, p + sizeof(T));
}

template<typename T>
T read(const Packet& in, usize& offset)
{
    T value{};
    if (offset + sizeof(T) <= in.size()) {
        std::memcpy(&value, in.data() + offset, sizeof(T));
        offset += sizeof(T);
    }
    return value;
}

} // namespace

Packet encode(const Snapshot& snapshot)
{
    Packet out;
    write(out, snapshot.sequence);
    write(out, static_cast<u32>(snapshot.positions.size()));
    for (const auto& [id, pos] : snapshot.positions) {
        write(out, id);
        write(out, pos.x);
        write(out, pos.y);
        write(out, pos.z);
    }
    return out;
}

Snapshot decode(const Packet& packet)
{
    Snapshot snap;
    usize offset = 0;
    snap.sequence = read<u32>(packet, offset);
    const u32 count = read<u32>(packet, offset);
    for (u32 i = 0; i < count; ++i) {
        const NetEntityId id = read<NetEntityId>(packet, offset);
        Vec3 pos;
        pos.x = read<f32>(packet, offset);
        pos.y = read<f32>(packet, offset);
        pos.z = read<f32>(packet, offset);
        snap.positions[id] = pos;
    }
    return snap;
}

} // namespace replication

void NetServer::broadcast()
{
    ++m_state.sequence;
    m_transport.send(replication::encode(m_state));
}

u32 NetClient::poll()
{
    u32 applied = 0;
    Packet packet;
    while (m_transport.receive(packet)) {
        Snapshot snap = replication::decode(packet);
        if (snap.sequence <= m_latest.sequence) {
            continue; // stale or duplicate — authoritative order wins
        }
        m_previous = std::move(m_latest);
        m_latest = std::move(snap);
        ++applied;
    }
    return applied;
}

Vec3 NetClient::position(NetEntityId id, f32 t) const
{
    const auto latest = m_latest.positions.find(id);
    if (latest == m_latest.positions.end()) {
        return Vec3{};
    }
    const auto prev = m_previous.positions.find(id);
    if (prev == m_previous.positions.end()) {
        return latest->second;
    }
    const f32 w = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return prev->second + (latest->second - prev->second) * w;
}

} // namespace vyro
