// VyroEngine — Audio file loading implementation
// Uses miniaudio's decoder (the MINIAUDIO_IMPLEMENTATION lives in
// AudioDevice.cpp; this TU only references the declarations).
#include "vyro/audio/AudioFile.hpp"

#include <miniaudio.h>

#include <filesystem>
#include <string>

namespace vyro {

std::expected<std::vector<f32>, AudioFileError> load_audio_file(std::string_view path)
{
    const std::string p(path);
    if (!std::filesystem::exists(p)) {
        return std::unexpected(AudioFileError::FileNotFound);
    }

    // Decode straight to mono f32 at 44.1 kHz to match the device mix format.
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 44100);
    ma_decoder decoder;
    if (ma_decoder_init_file(p.c_str(), &config, &decoder) != MA_SUCCESS) {
        return std::unexpected(AudioFileError::DecodeFailed);
    }

    ma_uint64 total_frames = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &total_frames) != MA_SUCCESS
        || total_frames == 0) {
        // Length may be unknown for some streams; fall back to chunked reads.
        std::vector<f32> samples;
        f32 chunk[4096];
        ma_uint64 read = 0;
        while (ma_decoder_read_pcm_frames(&decoder, chunk, 4096, &read) == MA_SUCCESS
               && read > 0) {
            samples.insert(samples.end(), chunk, chunk + read);
        }
        ma_decoder_uninit(&decoder);
        if (samples.empty()) {
            return std::unexpected(AudioFileError::DecodeFailed);
        }
        return samples;
    }

    std::vector<f32> samples(static_cast<usize>(total_frames));
    ma_uint64 frames_read = 0;
    ma_decoder_read_pcm_frames(&decoder, samples.data(), total_frames, &frames_read);
    samples.resize(static_cast<usize>(frames_read));
    ma_decoder_uninit(&decoder);

    if (samples.empty()) {
        return std::unexpected(AudioFileError::DecodeFailed);
    }
    return samples;
}

} // namespace vyro
