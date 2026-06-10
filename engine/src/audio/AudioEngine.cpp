// VyroEngine — Audio engine implementation
#include "vyro/audio/AudioEngine.hpp"

#include <cmath>

namespace vyro {

AudioEngine::AudioEngine()
{
    m_bus_volume.fill(1.0f);
}

ClipId AudioEngine::load_clip(std::string_view name, u32 sample_rate, u16 channels, u32 frame_count)
{
    m_clips.push_back(AudioClip{std::string(name), sample_rate, channels, frame_count});
    return static_cast<ClipId>(m_clips.size() - 1);
}

const AudioClip* AudioEngine::clip(ClipId id) const
{
    return id < m_clips.size() ? &m_clips[id] : nullptr;
}

SourceId AudioEngine::play(ClipId clip_id, f32 volume, AudioBus bus, bool looping)
{
    if (clip_id >= m_clips.size()) {
        return kInvalidSource;
    }
    Source src;
    src.clip = clip_id;
    src.volume = clamp01(volume);
    src.bus = bus;
    src.looping = looping;
    src.active = true;

    // Reuse an inactive slot if available.
    for (usize i = 0; i < m_sources.size(); ++i) {
        if (!m_sources[i].active) {
            m_sources[i] = src;
            return static_cast<SourceId>(i);
        }
    }
    m_sources.push_back(src);
    return static_cast<SourceId>(m_sources.size() - 1);
}

SourceId AudioEngine::play_spatial(ClipId clip_id, Vec3 position, f32 volume, bool looping)
{
    const SourceId id = play(clip_id, volume, AudioBus::Sfx, looping);
    if (id != kInvalidSource) {
        m_sources[id].spatial = true;
        m_sources[id].position = position;
    }
    return id;
}

void AudioEngine::stop(SourceId source)
{
    if (source < m_sources.size()) {
        m_sources[source].active = false;
    }
}

bool AudioEngine::is_playing(SourceId source) const
{
    return source < m_sources.size() && m_sources[source].active;
}

void AudioEngine::update(f32 dt)
{
    for (Source& s : m_sources) {
        if (!s.active) {
            continue;
        }
        const AudioClip& c = m_clips[s.clip];
        s.playhead += dt;
        const f32 duration = c.duration();
        if (s.playhead >= duration) {
            if (s.looping && duration > 0.0f) {
                s.playhead = std::fmod(s.playhead, duration);
            } else {
                s.active = false;
            }
        }
    }
}

void AudioEngine::set_attenuation(f32 ref_distance, f32 max_distance)
{
    m_ref_distance = ref_distance;
    m_max_distance = max_distance > ref_distance ? max_distance : ref_distance + 1.0f;
}

f32 AudioEngine::attenuation_for(Vec3 position) const
{
    const f32 d = length(position - m_listener);
    if (d <= m_ref_distance) {
        return 1.0f;
    }
    if (d >= m_max_distance) {
        return 0.0f;
    }
    // Linear rolloff between ref and max distance.
    return 1.0f - (d - m_ref_distance) / (m_max_distance - m_ref_distance);
}

f32 AudioEngine::effective_gain(SourceId source) const
{
    if (source >= m_sources.size() || !m_sources[source].active) {
        return 0.0f;
    }
    const Source& s = m_sources[source];
    f32 gain = s.volume * m_bus_volume[static_cast<usize>(s.bus)] * m_master;
    if (s.spatial) {
        gain *= attenuation_for(s.position);
    }
    return gain;
}

f32 AudioEngine::pan(SourceId source) const
{
    if (source >= m_sources.size() || !m_sources[source].active || !m_sources[source].spatial) {
        return 0.0f;
    }
    const f32 offset = m_sources[source].position.x - m_listener.x;
    const f32 p = offset / m_max_distance;
    return p < -1.0f ? -1.0f : (p > 1.0f ? 1.0f : p);
}

u32 AudioEngine::active_sources() const
{
    u32 count = 0;
    for (const Source& s : m_sources) {
        if (s.active) {
            ++count;
        }
    }
    return count;
}

} // namespace vyro
