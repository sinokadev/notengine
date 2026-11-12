#pragma once

#include <memory>
#include <string>

#include <glm/vec3.hpp>

namespace knot {

/** @brief EFX reverb properties for a shared acoustic environment. */
struct AudioEnvironment {
    /** @brief Enables the reverb send for active and future sources. */
    bool reverbEnabled = false;
    /** @brief Echo density in [0, 1]. */
    float density = 1.0f;
    /** @brief Echo diffusion in [0, 1]. */
    float diffusion = 1.0f;
    /** @brief Overall reverb gain in [0, 1]. */
    float gain = 0.32f;
    /** @brief High-frequency reverb gain in [0, 1]. */
    float gainHF = 0.89f;
    /** @brief Reverb decay time in seconds, in [0.1, 20]. */
    float decayTime = 1.49f;
    /** @brief High-frequency decay ratio in [0.1, 2]. */
    float decayHFRatio = 0.83f;
    /** @brief Early-reflection gain in [0, 3.16]. */
    float reflectionsGain = 0.05f;
    /** @brief Early-reflection delay in seconds, in [0, 0.3]. */
    float reflectionsDelay = 0.007f;
    /** @brief Late-reverb gain in [0, 10]. */
    float lateReverbGain = 1.26f;
    /** @brief Late-reverb delay in seconds, in [0, 0.1]. */
    float lateReverbDelay = 0.011f;
    /** @brief High-frequency air absorption in [0.892, 1]. */
    float airAbsorptionGainHF = 0.994f;
    /** @brief Distance rolloff factor in [0, 10]. */
    float roomRolloffFactor = 0.0f;
    /** @brief Enables high-frequency decay limiting. */
    bool decayHFLimit = true;
};

/**
 * @brief A standalone OpenAL Soft-based 2D and positional audio system.
 *
 * Audio clips are decoded when loaded. Playback instances are organized by
 * (groupName, soundName): the same sound can play in several groups or
 * concurrently within the same group.
 *
 * Public methods must be called from one application thread. Audio does not
 * depend on knot::Engine and is accessed via the singleton get() method.
 */
class Audio {
public:
    /** @brief Get the singleton instance of Audio.
     *  @return Reference to the single Audio instance. */
    static Audio& get();

    /** @brief Alias for get() to access the singleton instance.
     *  @return Reference to the single Audio instance. */
    static Audio& getInstance();

    ~Audio();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;
    Audio(Audio&&) = delete;
    Audio& operator=(Audio&&) = delete;

    /** @brief Opens the default OpenAL output device.
     *  @return true when playback is available. */
    bool init();

    /** @brief Stops playback, releases loaded clips, and closes the device. */
    void shutdown();

    /** @brief Stops playback and releases every loaded clip without closing the device. */
    void clear();

    /** @brief Reports whether the output device is available. */
    bool isInitialized() const;

    /** @brief Decodes @p path and registers it under @p name.
     *  @return false for invalid names or paths, duplicate names, decode
     *  failures, unsupported audio formats, or unavailable audio. */
    bool load(const std::string& path, const std::string& name);

    /** @brief Reports whether a clip has been successfully loaded. */
    bool isLoaded(const std::string& name) const;

    /**
     * @brief Plays a loaded clip in @p groupName.
     * Each call creates a new playback instance, allowing the same clip to
     * play concurrently within the same group.
     * @param volume Linear gain in the inclusive range [0, 1].
     * @param pitch Playback rate multiplier greater than zero.
     * @return false for invalid arguments, missing clips, or unavailable audio.
     */
    bool play(const std::string& name, const std::string& groupName, float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    /**
     * @brief Plays a loaded clip from a position in 3D space.
     * @param name Name of the loaded clip.
     * @param groupName Playback group for stopping and moving this source.
     * @param position World-space source position.
     * @param volume Linear gain in the inclusive range [0, 1].
     * @param pitch Playback rate multiplier greater than zero.
     * @param loop Whether to repeat playback.
     * @param referenceDistance Distance at which gain is unaffected by attenuation.
     * @param rolloff Attenuation rate beyond @p referenceDistance.
     * @param maxDistance Maximum distance used by the distance model.
     * @return false for invalid arguments, missing clips, or unavailable audio.
     */
    bool play3D(const std::string& name, const std::string& groupName, const glm::vec3& position, float volume = 1.0f, float pitch = 1.0f,
                bool loop = false, float referenceDistance = 1.0f, float rolloff = 1.0f, float maxDistance = 100.0f);

    /** @brief Sets the OpenAL listener pose, typically from the active camera. */
    bool setListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    /** @brief Applies the shared EFX reverb environment.
     *  @return false when the values are invalid, audio is unavailable, or EFX is unsupported. */
    bool setEnvironment(const AudioEnvironment& environment);

    /** @brief Gets the last successfully applied environment, or its defaults. */
    AudioEnvironment getEnvironment() const;

    /** @brief Moves every matching positional playback in @p groupName. */
    void setSourcePosition(const std::string& name, const std::string& groupName, const glm::vec3& position);

    /** @brief Stops every active instance of @p name in all groups. */
    void stop(const std::string& name);
    /** @brief Stops every active instance in @p groupName. */
    void stopGroup(const std::string& groupName);
    /** @brief Stops active @p name instances in @p groupName. */
    void stopInGroup(const std::string& name, const std::string& groupName);
    /** @brief Stops every active instance. */
    void stopAll();

    /** @brief Releases non-looping instances that finished since the last call.
     *  Call once per application frame while audio is in use. */
    void update();

private:
    struct Impl;

    Audio();
    std::unique_ptr<Impl> impl;
};

} // namespace knot
