// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 watta0

#pragma once

#include <memory>
#include <string>

namespace knot {

class Engine;

/**
 *
 * load() fully decodes WAV and MP3 files into float PCM data in memory.
 * Playback instances are identified by (groupName, soundName). A sound may
 * play simultaneously in different groups, but replaying the same pair stops
 * its current instance and restarts it with the new volume, pitch, and loop
 * settings.
 *
 * Audio remains unavailable when the default output device cannot be opened.
 * In that case, load() and play() return false.
 * All Audio methods must be called from the Engine's main thread.
 */
class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;
    Audio(Audio&&) = delete;
    Audio& operator=(Audio&&) = delete;

    /** @brief Opens the default output device.
     *  @return true when playback is available. */
    bool init();

    /** @brief Stops playback and releases loaded clips and the output device. */
    void shutdown();
    
    /** @brief Removes every loaded object */
    void clear();

    /** @brief Reports whether the default output device is available. */
    bool isInitialized() const;

    /** @brief Fully decodes a WAV or MP3 file and registers it under @p name.
     *  @return false for invalid names or paths, duplicate names, unsupported
     *  extensions, decode failures, or when audio is unavailable. */
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

private:
    friend class Engine;

    /** @brief Releases non-looping instances that finished during the frame. */
    void update();

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace knot
