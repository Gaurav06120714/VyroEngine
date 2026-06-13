// VyroEngine — Audio file loading (V4.2)
// Decodes WAV / MP3 / FLAC files into mono 44.1 kHz float PCM via miniaudio's
// built-in decoders, ready to hand to AudioDevice::play / play_looping.
#pragma once

#include "vyro/core/Types.hpp"

#include <expected>
#include <string_view>
#include <vector>

namespace vyro {

enum class AudioFileError {
    FileNotFound,
    DecodeFailed,
};

// Load and decode an audio file to mono 44.1 kHz float samples.
[[nodiscard]] std::expected<std::vector<f32>, AudioFileError> load_audio_file(std::string_view path);

} // namespace vyro
