// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 watta0

#include <knot/audio.h>

#include <iostream>
#include <limits>
#include <utility>

namespace knot {
namespace {

void logMiniaudioError(const char* operation, ma_result result) {
    std::cerr << "[Error] miniaudio: " << operation << ": " << ma_result_description(result) << std::endl;
}

} // namespace

Audio::Audio() = default;

Audio::~Audio() {
    shutdown();
}

bool Audio::init() {
    if (initialized) {
        return true;
    }

    std::cout << "[Info] Not Engine Audio Init" << std::endl;

    const ma_engine_config config = ma_engine_config_init();
    const ma_result result = ma_engine_init(&config, &engine);
    if (result != MA_SUCCESS) {
        logMiniaudioError("Audio initialization failed", result);
        return false;
    }

    initialized = true;
    return true;
}

void Audio::shutdown() {
    clear();

    if (initialized) {
        ma_engine_uninit(&engine);
        engine = {};
        initialized = false;
    }
}

void Audio::clear() {
    stopAll();
    clips.clear();
}

bool Audio::isInitialized() const {
    return initialized;
}

bool Audio::load(const std::string& path, const std::string& name) {
    if (!initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return false;
    }

    if (path.empty() || name.empty()) {
        std::cerr << "[Error] Audio path and name must not be empty" << std::endl;
        return false;
    }

    if (clips.find(name) != clips.end()) {
        std::cerr << "[Error] Audio name is already loaded: " << name << std::endl;
        return false;
    }

    const ma_uint32 channels = ma_engine_get_channels(&engine);
    const ma_uint32 sampleRate = ma_engine_get_sample_rate(&engine);
    if (channels == 0 || sampleRate == 0) {
        std::cerr << "[Error] Audio output format is unavailable" << std::endl;
        return false;
    }

    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, channels, sampleRate);
    ma_decoder decoder{};
    ma_result result = ma_decoder_init_file(path.c_str(), &config, &decoder);
    if (result != MA_SUCCESS) {
        logMiniaudioError("Audio load failed", result);
        return false;
    }

    ma_uint64 frameCount = 0;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (result != MA_SUCCESS || frameCount == 0) {
        if (result != MA_SUCCESS) {
            logMiniaudioError("Audio could not determine clip length", result);
        } else {
            std::cerr << "[Error] Audio file contains no PCM frames: " << path << std::endl;
        }
        ma_decoder_uninit(&decoder);
        return false;
    }

    if (frameCount > std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(channels)) {
        std::cerr << "[Error] Audio file is too large to preload: " << path << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    Clip clip;
    clip.samples.resize(static_cast<std::size_t>(frameCount) * channels);

    ma_uint64 framesRead = 0;
    result = ma_decoder_read_pcm_frames(&decoder, clip.samples.data(), frameCount, &framesRead);
    ma_decoder_uninit(&decoder);

    if ((result != MA_SUCCESS && result != MA_AT_END) || framesRead == 0) {
        if (result != MA_SUCCESS && result != MA_AT_END) {
            logMiniaudioError("Audio decode failed", result);
        } else {
            std::cerr << "[Error] Audio decoder produced no PCM frames: " << path << std::endl;
        }
        return false;
    }

    clip.frameCount = framesRead;
    clip.samples.resize(static_cast<std::size_t>(framesRead) * channels);
    clips.emplace(name, std::move(clip));
    return true;
}

bool Audio::isLoaded(const std::string& name) const {
    return clips.find(name) != clips.end();
}

bool Audio::play(const std::string& name, const std::string& groupName, float volume, float pitch, bool loop) {
    if (!initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return false;
    }

    if (name.empty() || groupName.empty() || volume < 0.0f || volume > 1.0f || pitch <= 0.0f) {
        std::cerr << "[Error] Invalid audio playback request" << std::endl;
        return false;
    }

    const auto clip = clips.find(name);
    if (clip == clips.end()) {
        std::cerr << "[Error] Audio is not loaded: " << name << std::endl;
        return false;
    }

    auto playback = std::make_unique<Playback>();
    ma_result result = ma_audio_buffer_ref_init(ma_format_f32, ma_engine_get_channels(&engine), clip->second.samples.data(), clip->second.frameCount,
                                                &playback->buffer);
    if (result != MA_SUCCESS) {
        logMiniaudioError("Audio playback buffer initialization failed", result);
        return false;
    }
    playback->bufferInitialized = true;

    result = ma_sound_init_from_data_source(&engine, reinterpret_cast<ma_data_source*>(&playback->buffer), MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr,
                                            &playback->sound);
    if (result != MA_SUCCESS) {
        logMiniaudioError("Audio sound initialization failed", result);
        releasePlayback(*playback);
        return false;
    }
    playback->soundInitialized = true;
    playback->loop = loop;

    ma_sound_set_volume(&playback->sound, volume);
    ma_sound_set_pitch(&playback->sound, pitch);
    ma_sound_set_looping(&playback->sound, loop ? MA_TRUE : MA_FALSE);

    result = ma_sound_start(&playback->sound);
    if (result != MA_SUCCESS) {
        logMiniaudioError("Audio start failed", result);
        releasePlayback(*playback);
        return false;
    }

    groups[groupName].emplace(name, std::move(playback));
    return true;
}

void Audio::stop(const std::string& name) {
    if (name.empty()) {
        return;
    }

    for (auto group = groups.begin(); group != groups.end();) {
        const auto range = group->second.equal_range(name);
        for (auto it = range.first; it != range.second; ++it) {
            releasePlayback(*it->second);
        }
        group->second.erase(range.first, range.second);

        if (group->second.empty()) {
            group = groups.erase(group);
        } else {
            ++group;
        }
    }
}

void Audio::stopGroup(const std::string& groupName) {
    const auto group = groups.find(groupName);
    if (group == groups.end()) {
        return;
    }

    for (auto& playback : group->second) {
        releasePlayback(*playback.second);
    }
    groups.erase(group);
}

void Audio::stopInGroup(const std::string& name, const std::string& groupName) {
    const auto group = groups.find(groupName);
    if (group == groups.end()) {
        return;
    }

    const auto range = group->second.equal_range(name);
    if (range.first == range.second) {
        return;
    }

    for (auto it = range.first; it != range.second; ++it) {
        releasePlayback(*it->second);
    }
    group->second.erase(range.first, range.second);
    if (group->second.empty()) {
        groups.erase(group);
    }
}

void Audio::stopAll() {
    for (auto& group : groups) {
        for (auto& playback : group.second) {
            releasePlayback(*playback.second);
        }
    }
    groups.clear();
}

void Audio::update() {
    if (!initialized) {
        return;
    }

    for (auto group = groups.begin(); group != groups.end();) {
        for (auto playback = group->second.begin(); playback != group->second.end();) {
            if (!playback->second->loop && ma_sound_at_end(&playback->second->sound)) {
                releasePlayback(*playback->second);
                playback = group->second.erase(playback);
            } else {
                ++playback;
            }
        }

        if (group->second.empty()) {
            group = groups.erase(group);
        } else {
            ++group;
        }
    }
}

void Audio::releasePlayback(Playback& playback) {
    if (playback.soundInitialized) {
        ma_sound_stop(&playback.sound);
        ma_sound_uninit(&playback.sound);
        playback.soundInitialized = false;
    }

    if (playback.bufferInitialized) {
        ma_audio_buffer_ref_uninit(&playback.buffer);
        playback.bufferInitialized = false;
    }
}

} // namespace knot
