// VyroEngine — Co-op link tests (V4.4)
// Two CoopLinks over a loopback pipe model two networked soldiers: each
// publishes its own avatar and replicates the peer's, including interpolation.
#include "vyro/net/Coop.hpp"
#include "vyro/net/Transport.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("coop");

    LoopbackTransport pipe;
    CoopLink host(pipe.a(), /*local*/ 1, /*peer*/ 2);
    CoopLink join(pipe.b(), /*local*/ 2, /*peer*/ 1);

    // Before any traffic, neither side sees a peer.
    suite.check(!host.peer_connected(), "host has no peer yet");
    suite.check(!join.peer_connected(), "joiner has no peer yet");

    // One exchange: each publishes its avatar, then drains the other's.
    host.set_local_position(Vec3{-2, 0, 5});
    join.set_local_position(Vec3{2, 0, 5});
    host.broadcast();
    join.broadcast();
    suite.check(host.poll() == 1, "host applied the joiner snapshot");
    suite.check(join.poll() == 1, "joiner applied the host snapshot");

    suite.check(host.peer_connected(), "host now sees the joiner");
    suite.check(join.peer_connected(), "joiner now sees the host");
    suite.check(host.peer_position() == (Vec3{2, 0, 5}), "host replicates joiner avatar");
    suite.check(join.peer_position() == (Vec3{-2, 0, 5}), "joiner replicates host avatar");

    // Movement replicates and interpolates between the two latest snapshots.
    join.set_local_position(Vec3{2, 0, 5});
    join.broadcast();
    host.poll();
    join.set_local_position(Vec3{6, 0, 5});
    join.broadcast();
    host.poll();

    suite.check(approx(host.peer_position(0.0f).x, 2.0f), "t=0 is previous peer pos");
    suite.check(approx(host.peer_position(0.5f).x, 4.0f), "t=0.5 interpolates peer pos");
    suite.check(approx(host.peer_position(1.0f).x, 6.0f), "t=1 is latest peer pos");

    return suite.summary();
}
