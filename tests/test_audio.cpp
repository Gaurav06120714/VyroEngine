// VyroEngine — Audio engine tests
#include "vyro/audio/AudioEngine.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("audio");

    // AudioEngine_LoadClip_StoresMetadata
    {
        AudioEngine audio;
        const ClipId c = audio.load_clip("shot", 44100, 2, 44100); // 1 second
        suite.check(audio.clip_count() == 1, "one clip loaded");
        suite.check(approx(audio.clip(c)->duration(), 1.0f), "duration = frames / sample_rate");
    }

    // AudioEngine_Play_StartsAndStops (5.1)
    {
        AudioEngine audio;
        const ClipId c = audio.load_clip("blip", 1000, 1, 500); // 0.5 s
        const SourceId s = audio.play(c);
        suite.check(audio.is_playing(s), "source playing after play");
        suite.check(audio.active_sources() == 1, "one active source");
        audio.update(0.6f); // past end of a non-looping clip
        suite.check(!audio.is_playing(s), "non-looping source stops at end");
    }

    // AudioEngine_Loop_WrapsPlayhead (5.1)
    {
        AudioEngine audio;
        const ClipId c = audio.load_clip("loop", 1000, 1, 1000); // 1 s
        const SourceId s = audio.play(c, 1.0f, AudioBus::Music, /*looping*/ true);
        audio.update(1.5f);
        suite.check(audio.is_playing(s), "looping source keeps playing past end");
    }

    // AudioEngine_Mixing_CombinesMasterBusSource (5.2)
    {
        AudioEngine audio;
        const ClipId c = audio.load_clip("m", 1000, 1, 1000);
        const SourceId s = audio.play(c, 0.5f, AudioBus::Music, true);
        audio.set_master_volume(0.5f);
        audio.set_bus_volume(AudioBus::Music, 0.5f);
        // 0.5 (source) * 0.5 (bus) * 0.5 (master) = 0.125
        suite.check(approx(audio.effective_gain(s), 0.125f), "gain multiplies source*bus*master");
    }

    // AudioEngine_Spatial_AttenuatesWithDistance (5.3)
    {
        AudioEngine audio;
        const ClipId c = audio.load_clip("sp", 1000, 1, 1000);
        audio.set_listener(Vec3{0, 0, 0});
        audio.set_attenuation(1.0f, 11.0f);
        const SourceId near = audio.play_spatial(c, Vec3{0, 0, 0}, 1.0f, true);
        const SourceId far = audio.play_spatial(c, Vec3{6, 0, 0}, 1.0f, true);
        suite.check(approx(audio.effective_gain(near), 1.0f), "source at listener is full gain");
        // d=6, ref=1, max=11 -> 1 - (6-1)/10 = 0.5
        suite.check(approx(audio.effective_gain(far), 0.5f), "distance attenuates gain");
        suite.check(audio.pan(far) > 0.0f, "source to the right pans positive");
    }

    return suite.summary();
}
