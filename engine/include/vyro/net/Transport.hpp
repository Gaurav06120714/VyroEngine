// VyroEngine — Network transport
// Phase 9.1: a message-oriented transport interface. LoopbackTransport is an
// in-memory, deterministic implementation for local play and tests; a UDP
// socket transport implements the same interface for real networking.
#pragma once

#include "vyro/core/Types.hpp"

#include <deque>
#include <memory>
#include <vector>

namespace vyro {

using Packet = std::vector<byte>;

class ITransport
{
public:
    virtual ~ITransport() = default;
    virtual void send(const Packet& packet) = 0;
    [[nodiscard]] virtual bool receive(Packet& out) = 0;
    [[nodiscard]] virtual usize pending() const = 0;
};

// A pair of in-memory endpoints: what one sends, the other receives.
class LoopbackTransport
{
public:
    class Endpoint final : public ITransport
    {
    public:
        explicit Endpoint(std::deque<Packet>& tx, std::deque<Packet>& rx) : m_tx(tx), m_rx(rx) {}

        void send(const Packet& packet) override { m_tx.push_back(packet); }

        bool receive(Packet& out) override
        {
            if (m_rx.empty()) {
                return false;
            }
            out = std::move(m_rx.front());
            m_rx.pop_front();
            return true;
        }

        [[nodiscard]] usize pending() const override { return m_rx.size(); }

    private:
        std::deque<Packet>& m_tx;
        std::deque<Packet>& m_rx;
    };

    LoopbackTransport()
        : m_a(std::make_unique<Endpoint>(m_a_to_b, m_b_to_a)),
          m_b(std::make_unique<Endpoint>(m_b_to_a, m_a_to_b))
    {
    }

    [[nodiscard]] ITransport& a() { return *m_a; }
    [[nodiscard]] ITransport& b() { return *m_b; }

private:
    std::deque<Packet> m_a_to_b;
    std::deque<Packet> m_b_to_a;
    std::unique_ptr<Endpoint> m_a;
    std::unique_ptr<Endpoint> m_b;
};

} // namespace vyro
