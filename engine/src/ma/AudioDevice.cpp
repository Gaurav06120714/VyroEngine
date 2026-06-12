// VyroEngine — Real audio output device implementation (miniaudio)
#include "vyro/audio/AudioDevice.hpp"

#include "vyro/core/Log.hpp"

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODING
#define MA_NO_ENCODING
#include <miniaudio.h>

#include <algorithm>
#include <mutex>

namespace vyro {

struct Voice {
    std::vector<f32> samples;
    usize cursor = 0;
    f32 gain = 1.0f;
};

struct AudioDevice::Impl {
    ma_device device{};
    bool open = false;
    std::mutex mutex;
    std::vector<Voice> voices;
    f32 master = 1.0f;

    static void callback(ma_device* device, void* output, const void* /*input*/,
                         ma_uint32 frame_count)
    {
        auto* impl = static_cast<Impl*>(device->pUserData);
        auto* out = static_cast<float*>(output); // stereo interleaved, pre-zeroed

        std::lock_guard<std::mutex> lock(impl->mutex);
        for (Voice& voice : impl->voices) {
            for (ma_uint32 f = 0; f < frame_count && voice.cursor < voice.samples.size();
                 ++f, ++voice.cursor) {
                const f32 s = voice.samples[voice.cursor] * voice.gain * impl->master;
                out[f * 2 + 0] += s;
                out[f * 2 + 1] += s;
            }
        }
        impl->voices.erase(std::remove_if(impl->voices.begin(), impl->voices.end(),
                                          [](const Voice& v) {
                                              return v.cursor >= v.samples.size();
                                          }),
                           impl->voices.end());
    }
};

AudioDevice::AudioDevice() : m_impl(std::make_unique<Impl>()) {}

AudioDevice::~AudioDevice()
{
    shutdown();
}

bool AudioDevice::init()
{
    if (m_impl->open) {
        return true;
    }
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 44100;
    config.dataCallback = Impl::callback;
    config.pUserData = m_impl.get();

    if (ma_device_init(nullptr, &config, &m_impl->device) != MA_SUCCESS) {
        VYRO_ERROR("Audio", "failed to open output device");
        return false;
    }
    if (ma_device_start(&m_impl->device) != MA_SUCCESS) {
        VYRO_ERROR("Audio", "failed to start output device");
        ma_device_uninit(&m_impl->device);
        return false;
    }
    m_impl->open = true;
    VYRO_INFO("Audio", "output device open: {} Hz", m_impl->device.sampleRate);
    return true;
}

void AudioDevice::shutdown()
{
    if (m_impl != nullptr && m_impl->open) {
        ma_device_uninit(&m_impl->device);
        m_impl->open = false;
    }
}

bool AudioDevice::is_open() const
{
    return m_impl->open;
}

void AudioDevice::play(const std::vector<f32>& samples, f32 gain)
{
    if (!m_impl->open || samples.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->voices.push_back(Voice{samples, 0, std::clamp(gain, 0.0f, 4.0f)});
}

u32 AudioDevice::active_voices() const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return static_cast<u32>(m_impl->voices.size());
}

void AudioDevice::set_master_gain(f32 gain)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->master = std::clamp(gain, 0.0f, 1.0f);
}

} // namespace vyro
