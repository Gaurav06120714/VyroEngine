// VyroEngine — Networking tests
#include "vyro/net/Replication.hpp"
#include "vyro/net/Transport.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("net");

    // Loopback_SendReceive_Roundtrips (9.1)
    {
        LoopbackTransport pipe;
        pipe.a().send(Packet{byte{1}, byte{2}, byte{3}});
        suite.check(pipe.b().pending() == 1, "packet pending at peer");
        Packet got;
        suite.check(pipe.b().receive(got), "peer receives");
        suite.check(got.size() == 3 && got[2] == byte{3}, "payload intact");
        suite.check(!pipe.b().receive(got), "queue drained");
    }

    // Replication_EncodeDecode_Roundtrips (9.2)
    {
        Snapshot snap;
        snap.sequence = 7;
        snap.positions[1] = Vec3{1, 2, 3};
        snap.positions[2] = Vec3{-4, 0, 9};
        const Snapshot back = replication::decode(replication::encode(snap));
        suite.check(back.sequence == 7, "sequence survives roundtrip");
        suite.check(back.positions.size() == 2, "entity count survives");
        suite.check(back.positions.at(1) == (Vec3{1, 2, 3}), "position 1 survives");
        suite.check(back.positions.at(2) == (Vec3{-4, 0, 9}), "position 2 survives");
    }

    // ServerClient_Broadcast_ReplicatesState (9.1/9.2)
    {
        LoopbackTransport pipe;
        NetServer server(pipe.a());
        NetClient client(pipe.b());

        server.set_position(10, Vec3{5, 0, 0});
        server.broadcast();
        suite.check(client.poll() == 1, "client applied one snapshot");
        suite.check(client.position(10) == (Vec3{5, 0, 0}), "client sees server position");
        suite.check(client.latest_sequence() == 1, "sequence advanced");
    }

    // Client_Interpolation_BlendsSnapshots (9.3)
    {
        LoopbackTransport pipe;
        NetServer server(pipe.a());
        NetClient client(pipe.b());

        server.set_position(1, Vec3{0, 0, 0});
        server.broadcast();
        server.set_position(1, Vec3{10, 0, 0});
        server.broadcast();
        client.poll();

        suite.check(approx(client.position(1, 0.0f).x, 0.0f), "t=0 is previous snapshot");
        suite.check(approx(client.position(1, 0.5f).x, 5.0f), "t=0.5 interpolates halfway");
        suite.check(approx(client.position(1, 1.0f).x, 10.0f), "t=1 is latest snapshot");
    }

    // Client_StaleSnapshot_Dropped (9.3)
    {
        LoopbackTransport pipe;
        NetClient client(pipe.b());

        Snapshot newer;
        newer.sequence = 5;
        newer.positions[1] = Vec3{9, 9, 9};
        Snapshot stale;
        stale.sequence = 3;
        stale.positions[1] = Vec3{0, 0, 0};

        pipe.a().send(replication::encode(newer));
        pipe.a().send(replication::encode(stale)); // arrives later but older
        suite.check(client.poll() == 1, "only the newer snapshot applied");
        suite.check(client.position(1) == (Vec3{9, 9, 9}), "stale data ignored");
    }

    return suite.summary();
}
