// VyroEngine — UDP transport tests
// Integration test over real localhost sockets. Localhost UDP delivery is
// effectively immediate; a bounded retry loop absorbs scheduler jitter so the
// test stays deterministic in practice.
#include "vyro/net/Replication.hpp"
#include "vyro/net/UdpTransport.hpp"

#include "test_harness.hpp"

#include <chrono>
#include <thread>

namespace {

// Poll `fn` until it returns true or ~1s elapses.
template<typename F>
bool eventually(F&& fn)
{
    for (int i = 0; i < 100; ++i) {
        if (fn()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("udp");

    // UdpTransport_Bind_PicksEphemeralPort
    {
        UdpTransport t;
        suite.check(t.bind(0), "bind(0) succeeds");
        suite.check(t.local_port() != 0, "ephemeral port discovered");
        suite.check(t.is_open(), "socket open");
    }

    // UdpTransport_SendReceive_DeliversDatagram
    {
        UdpTransport a;
        UdpTransport b;
        suite.check(a.bind(0) && b.bind(0), "both endpoints bound");
        suite.check(a.set_peer("127.0.0.1", b.local_port()), "a peers with b");
        suite.check(b.set_peer("127.0.0.1", a.local_port()), "b peers with a");

        a.send(Packet{byte{7}, byte{8}, byte{9}});
        Packet got;
        const bool delivered = eventually([&] { return b.receive(got); });
        suite.check(delivered, "datagram delivered over localhost");
        suite.check(got.size() == 3 && got[0] == byte{7} && got[2] == byte{9},
                    "payload intact");
        suite.check(a.set_peer("not-an-ip", 1) == false, "invalid peer rejected");
    }

    // UdpTransport_Replication_ServerToClientOverRealSockets
    {
        UdpTransport server_io;
        UdpTransport client_io;
        server_io.bind(0);
        client_io.bind(0);
        server_io.set_peer("127.0.0.1", client_io.local_port());
        client_io.set_peer("127.0.0.1", server_io.local_port());

        NetServer server(server_io);
        NetClient client(client_io);

        server.set_position(1, Vec3{3, 4, 5});
        server.broadcast();

        const bool synced = eventually([&] { return client.poll() > 0; });
        suite.check(synced, "snapshot replicated over UDP");
        suite.check(client.position(1) == (Vec3{3, 4, 5}), "client sees server state");
        suite.check(client.latest_sequence() == 1, "sequence replicated");
    }

    return suite.summary();
}
