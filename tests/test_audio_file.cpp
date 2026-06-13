// VyroEngine — Audio file loading tests (V4.2)
// Writes a minimal 16-bit PCM WAV to a temp path, decodes it via miniaudio,
// and verifies the samples come back. Linked against vyro::audio.
#include "vyro/audio/AudioFile.hpp"

#include "test_harness.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

void write_u32(std::ofstream& f, std::uint32_t v) { f.write(reinterpret_cast<char*>(&v), 4); }
void write_u16(std::ofstream& f, std::uint16_t v) { f.write(reinterpret_cast<char*>(&v), 2); }

// Write a mono 44.1kHz 16-bit WAV containing a short sine tone.
bool write_test_wav(const std::string& path)
{
    constexpr std::uint32_t rate = 44100;
    constexpr std::uint32_t frames = 4410; // 0.1s
    std::vector<std::int16_t> pcm(frames);
    for (std::uint32_t i = 0; i < frames; ++i) {
        const double t = static_cast<double>(i) / rate;
        pcm[i] = static_cast<std::int16_t>(std::sin(t * 6.28318 * 440.0) * 20000.0);
    }
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return false;
    }
    const std::uint32_t data_bytes = frames * 2;
    f.write("RIFF", 4);
    write_u32(f, 36 + data_bytes);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    write_u32(f, 16);
    write_u16(f, 1);          // PCM
    write_u16(f, 1);          // mono
    write_u32(f, rate);
    write_u32(f, rate * 2);   // byte rate
    write_u16(f, 2);          // block align
    write_u16(f, 16);         // bits
    f.write("data", 4);
    write_u32(f, data_bytes);
    f.write(reinterpret_cast<char*>(pcm.data()), data_bytes);
    return f.good();
}

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("audio_file");

    const auto path = (std::filesystem::temp_directory_path() / "vyro_tone.wav").string();
    if (!write_test_wav(path)) {
        suite.check(true, "could not write temp WAV; skipping");
        return suite.summary();
    }

    const auto samples = load_audio_file(path);
    suite.check(samples.has_value(), "WAV decodes");
    if (samples.has_value()) {
        suite.check(samples->size() > 4000 && samples->size() < 4800,
                    "decoded ~4410 mono frames");
        f32 peak = 0.0f;
        for (const f32 s : *samples) {
            peak = std::max(peak, std::fabs(s));
        }
        suite.check(peak > 0.4f, "decoded samples carry the tone");
    }

    suite.check(!load_audio_file("/no/such/file.wav").has_value(), "missing file errors");
    std::filesystem::remove(path);
    return suite.summary();
}
