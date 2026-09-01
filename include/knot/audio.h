// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 watta0

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <miniaudio/miniaudio.h>

namespace knot {

/**
 * @brief A standalone miniaudio-based 2D audio system.
 *
 * Audio clips are fully decoded to float PCM when loaded. Playback instances
 * are identified by (groupName, soundName): the same sound can play in
 * several groups, while a repeated play() call for the same pair restarts the
 * existing instance. Format selection is delegated to miniaudio; this class
 * deliberately does not whitelist filename extensions, so newly enabled
 * miniaudio decoders work without changes to this API.
 *
 * Public methods must be called from one application thread. Audio does not
 * depend on knot::Engine and may be created and used directly.
 */
class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;
    Audio(Audio&&) = delete;
    Audio& operator=(Audio&&) = delete;

    /** @brief Opens miniaudio's default output device.
     *  @return true when playback is available. */
    bool init();

    /** @brief Stops playback, releases loaded clips, and closes the device. */
    void shutdown();

    /** @brief Stops playback and releases every loaded clip without closing the device. */
    void clear();

    /** @brief Reports whether the output device is available. */
    bool isInitialized() const;

    /** @brief Decodes @p path and registers it under @p name.
     *  The file extension is not restricted; miniaudio decides whether the
     *  underlying file format is supported.
     *  @return false for invalid names or paths, duplicate names, decode
     *  failures, or unavailable audio. */
    bool load(const std::string& path, const std::string& name);

    /** @brief Reports whether a clip has been successfully loaded. */
    bool isLoaded(const std::string& name) const;

    /** @brief Plays a loaded clip in @p groupName.
     *  @param volume Linear gain in the inclusive range [0, 1].
     *  @param pitch Playback rate multiplier greater than zero.
     *  @return false for invalid arguments, missing clips, or unavailable audio. */
    bool play(const std::string& name, const std::string& groupName, float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    /** @brief Stops every active instance of @p name in all groups. */
    void stop(const std::string& name);
    /** @brief Stops every active instance in @p groupName. */
    void stopGroup(const std::string& groupName);
    /** @brief Stops the active @p name instance in @p groupName. */
    void stopInGroup(const std::string& name, const std::string& groupName);
    /** @brief Stops every active instance. */
    void stopAll();

    /** @brief Releases non-looping instances that finished since the last call.
     *  Call once per application frame while audio is in use. */
    void update();

private:
    struct Clip {
        std::vector<float> samples;
        ma_uint64 frameCount = 0;
    };

    struct Playback {
        ma_audio_buffer_ref buffer{};
        ma_sound sound{};
        bool bufferInitialized = false;
        bool soundInitialized = false;
        bool loop = false;
    };

    using PlaybackGroup = std::unordered_map<std::string, std::unique_ptr<Playback>>;

    void releasePlayback(Playback& playback);

    ma_engine engine{};
    bool initialized = false;
    std::unordered_map<std::string, Clip> clips;
    std::unordered_map<std::string, PlaybackGroup> groups;
};

} // namespace knot
