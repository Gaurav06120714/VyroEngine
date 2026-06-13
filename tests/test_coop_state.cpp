// VyroEngine — Co-op shared world-state tests (V6.3)
// World snapshots (score/wave/hp/horde) survive encode/decode and replicate
// host -> joiner over a transport, newest-wins.
#include "vyro/net/CoopState.hpp"
#include "vyro/net/Transport.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("coop_state");

    // Encode/decode roundtrip including the horde list.
    {
        CoopWorldState s;
        s.sequence = 12;
        s.score = 130;
        s.wave = 4;
        s.host_hp = 2;
        s.horde = {Vec3{1, 0, 2}, Vec3{-3, 0, 5}, Vec3{0, 0, -8}};
        const CoopWorldState back = coopstate::decode(coopstate::encode(s));
        suite.check(back.sequence == 12 && back.score == 130 && back.wave == 4
                        && back.host_hp == 2,
                    "scalars survive roundtrip");
        suite.check(back.horde.size() == 3, "horde count survives");
        suite.check(back.horde[1] == (Vec3{-3, 0, 5}), "horde position survives");
    }

    // Host -> joiner replication over a loopback pipe.
    {
        LoopbackTransport pipe;
        CoopStateChannel host(pipe.a());
        CoopStateChannel joiner(pipe.b());

        suite.check(!joiner.connected(), "joiner has no state yet");

        CoopWorldState s;
        s.score = 40;
        s.wave = 2;
        s.host_hp = 3;
        s.horde = {Vec3{5, 0, 5}};
        host.send(s);
        suite.check(joiner.poll() == 1, "joiner applied one snapshot");
        suite.check(joiner.connected(), "joiner is now connected");
        suite.check(joiner.latest().score == 40 && joiner.latest().wave == 2,
                    "joiner sees the host score/wave");
        suite.check(joiner.latest().horde.size() == 1, "joiner sees the host horde");
    }

    // Stale snapshots are dropped (newest wins).
    {
        LoopbackTransport pipe;
        CoopStateChannel joiner(pipe.b());
        CoopWorldState newer;
        newer.sequence = 9;
        newer.score = 99;
        CoopWorldState stale;
        stale.sequence = 4;
        stale.score = 11;
        pipe.a().send(coopstate::encode(newer));
        pipe.a().send(coopstate::encode(stale));
        suite.check(joiner.poll() == 1, "only the newer snapshot applied");
        suite.check(joiner.latest().score == 99, "stale score ignored");
    }

    return suite.summary();
}
