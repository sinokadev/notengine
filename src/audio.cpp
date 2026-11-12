#include <knot/audio.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

#include <miniaudio.h>

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace knot {
namespace {

void clearAlErrors() {
    while (alGetError() != AL_NO_ERROR) {
    }
}

void logAlError(const char* operation, ALenum error) {
    std::cerr << "[Error] OpenAL: " << operation << " failed (error " << error << ')' << std::endl;
}

void logAlcError(const char* operation, ALCdevice* device) {
    const ALCenum error = alcGetError(device);
    std::cerr << "[Error] OpenAL: " << operation << " failed (error " << error << ')' << std::endl;
}

void logMiniaudioError(const char* operation, ma_result result) {
    std::cerr << "[Error] miniaudio: " << operation << ": " << ma_result_description(result) << std::endl;
}

bool isFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool isValid(const AudioEnvironment& environment) {
    const auto inRange = [](float value, float minimum, float maximum) { return std::isfinite(value) && value >= minimum && value <= maximum; };

    return inRange(environment.density, 0.0f, 1.0f) && inRange(environment.diffusion, 0.0f, 1.0f) && inRange(environment.gain, 0.0f, 1.0f) &&
           inRange(environment.gainHF, 0.0f, 1.0f) && inRange(environment.decayTime, 0.1f, 20.0f) && inRange(environment.decayHFRatio, 0.1f, 2.0f) &&
           inRange(environment.reflectionsGain, 0.0f, 3.16f) && inRange(environment.reflectionsDelay, 0.0f, 0.3f) &&
           inRange(environment.lateReverbGain, 0.0f, 10.0f) && inRange(environment.lateReverbDelay, 0.0f, 0.1f) &&
           inRange(environment.airAbsorptionGainHF, 0.892f, 1.0f) && inRange(environment.roomRolloffFactor, 0.0f, 10.0f);
}

} // namespace

struct Audio::Impl {
    struct Clip {
        ALuint stereoBuffer = 0;
        ALuint monoBuffer = 0;
    };

    struct Playback {
        ALuint source = 0;
        bool loop = false;
        bool spatialized = false;
    };

    using PlaybackGroup = std::unordered_multimap<std::string, std::unique_ptr<Playback>>;

    LPALGENEFFECTS genEffects = nullptr;
    LPALDELETEEFFECTS deleteEffects = nullptr;
    LPALEFFECTI effectI = nullptr;
    LPALEFFECTF effectF = nullptr;
    LPALGENAUXILIARYEFFECTSLOTS genEffectSlots = nullptr;
    LPALDELETEAUXILIARYEFFECTSLOTS deleteEffectSlots = nullptr;
    LPALAUXILIARYEFFECTSLOTI effectSlotI = nullptr;

    ALCdevice* device = nullptr;
    ALCcontext* context = nullptr;
    bool initialized = false;
    bool efxAvailable = false;
    ALuint effect = 0;
    ALuint effectSlot = 0;
    AudioEnvironment environment;
    std::unordered_map<std::string, Clip> clips;
    std::unordered_map<std::string, PlaybackGroup> groups;

    bool play(const std::string& name, const std::string& groupName, const glm::vec3* position, float volume, float pitch, bool loop,
              float referenceDistance, float rolloff, float maxDistance) {
        const auto clip = clips.find(name);
        if (clip == clips.end()) {
            std::cerr << "[Error] Audio is not loaded: " << name << std::endl;
            return false;
        }

        auto playback = std::make_unique<Playback>();
        clearAlErrors();
        alGenSources(1, &playback->source);
        ALenum error = alGetError();
        if (error != AL_NO_ERROR || playback->source == 0) {
            if (error != AL_NO_ERROR) {
                logAlError("source creation", error);
            } else {
                std::cerr << "[Error] OpenAL: source creation returned an invalid handle" << std::endl;
            }
            releasePlayback(*playback);
            return false;
        }

        const bool spatialized = position != nullptr;
        alSourcei(playback->source, AL_BUFFER, static_cast<ALint>(spatialized ? clip->second.monoBuffer : clip->second.stereoBuffer));
        alSourcei(playback->source, AL_SOURCE_RELATIVE, spatialized ? AL_FALSE : AL_TRUE);
        alSourcef(playback->source, AL_ROLLOFF_FACTOR, spatialized ? rolloff : 0.0f);
        alSourcef(playback->source, AL_REFERENCE_DISTANCE, referenceDistance);
        alSourcef(playback->source, AL_MAX_DISTANCE, maxDistance);
        alSourcef(playback->source, AL_GAIN, volume);
        alSourcef(playback->source, AL_PITCH, pitch);
        alSourcei(playback->source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        if (position != nullptr) {
            alSource3f(playback->source, AL_POSITION, position->x, position->y, position->z);
        }
        if (environment.reverbEnabled && effectSlot != 0) {
            alSource3i(playback->source, AL_AUXILIARY_SEND_FILTER, static_cast<ALint>(effectSlot), 0, AL_FILTER_NULL);
        }
        alSourcePlay(playback->source);
        error = alGetError();
        if (error != AL_NO_ERROR) {
            logAlError("source playback", error);
            releasePlayback(*playback);
            return false;
        }

        playback->loop = loop;
        playback->spatialized = spatialized;
        groups[groupName].emplace(name, std::move(playback));
        return true;
    }

    void releasePlayback(Playback& playback) {
        if (playback.source == 0) {
            return;
        }

        clearAlErrors();
        alSourceStop(playback.source);
        alDeleteSources(1, &playback.source);
        const ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            logAlError("source release", error);
        }
        playback.source = 0;
    }

    void initializeEfx() {
        if (alcIsExtensionPresent(device, ALC_EXT_EFX_NAME) != ALC_TRUE) {
            std::cerr << "[Warning] OpenAL EFX is unavailable; reverb environments are disabled" << std::endl;
            return;
        }

        genEffects = reinterpret_cast<LPALGENEFFECTS>(alGetProcAddress("alGenEffects"));
        deleteEffects = reinterpret_cast<LPALDELETEEFFECTS>(alGetProcAddress("alDeleteEffects"));
        effectI = reinterpret_cast<LPALEFFECTI>(alGetProcAddress("alEffecti"));
        effectF = reinterpret_cast<LPALEFFECTF>(alGetProcAddress("alEffectf"));
        genEffectSlots = reinterpret_cast<LPALGENAUXILIARYEFFECTSLOTS>(alGetProcAddress("alGenAuxiliaryEffectSlots"));
        deleteEffectSlots = reinterpret_cast<LPALDELETEAUXILIARYEFFECTSLOTS>(alGetProcAddress("alDeleteAuxiliaryEffectSlots"));
        effectSlotI = reinterpret_cast<LPALAUXILIARYEFFECTSLOTI>(alGetProcAddress("alAuxiliaryEffectSloti"));

        if (genEffects == nullptr || deleteEffects == nullptr || effectI == nullptr || effectF == nullptr || genEffectSlots == nullptr ||
            deleteEffectSlots == nullptr || effectSlotI == nullptr) {
            std::cerr << "[Warning] OpenAL EFX entry points are unavailable; reverb environments are disabled" << std::endl;
            return;
        }

        clearAlErrors();
        genEffectSlots(1, &effectSlot);
        const ALenum error = alGetError();
        if (error != AL_NO_ERROR || effectSlot == 0) {
            logAlError("reverb slot creation", error);
            effectSlot = 0;
            return;
        }
        efxAvailable = true;
    }

    void shutdownEfx() {
        if (effectSlot != 0 && deleteEffectSlots != nullptr) {
            clearAlErrors();
            deleteEffectSlots(1, &effectSlot);
            const ALenum error = alGetError();
            if (error != AL_NO_ERROR) {
                logAlError("reverb slot release", error);
            }
        }
        if (effect != 0 && deleteEffects != nullptr) {
            clearAlErrors();
            deleteEffects(1, &effect);
            const ALenum error = alGetError();
            if (error != AL_NO_ERROR) {
                logAlError("reverb effect release", error);
            }
        }
        effectSlot = 0;
        effect = 0;
        efxAvailable = false;
        genEffects = nullptr;
        deleteEffects = nullptr;
        effectI = nullptr;
        effectF = nullptr;
        genEffectSlots = nullptr;
        deleteEffectSlots = nullptr;
        effectSlotI = nullptr;
    }

    bool applyEnvironment(const AudioEnvironment& value) {
        if (!efxAvailable) {
            std::cerr << "[Error] OpenAL EFX is unavailable" << std::endl;
            return false;
        }

        ALuint newEffect = 0;
        if (value.reverbEnabled) {
            clearAlErrors();
            genEffects(1, &newEffect);
            ALenum error = alGetError();
            if (error != AL_NO_ERROR || newEffect == 0) {
                logAlError("reverb effect creation", error);
                return false;
            }

            effectI(newEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
            effectF(newEffect, AL_REVERB_DENSITY, value.density);
            effectF(newEffect, AL_REVERB_DIFFUSION, value.diffusion);
            effectF(newEffect, AL_REVERB_GAIN, value.gain);
            effectF(newEffect, AL_REVERB_GAINHF, value.gainHF);
            effectF(newEffect, AL_REVERB_DECAY_TIME, value.decayTime);
            effectF(newEffect, AL_REVERB_DECAY_HFRATIO, value.decayHFRatio);
            effectF(newEffect, AL_REVERB_REFLECTIONS_GAIN, value.reflectionsGain);
            effectF(newEffect, AL_REVERB_REFLECTIONS_DELAY, value.reflectionsDelay);
            effectF(newEffect, AL_REVERB_LATE_REVERB_GAIN, value.lateReverbGain);
            effectF(newEffect, AL_REVERB_LATE_REVERB_DELAY, value.lateReverbDelay);
            effectF(newEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, value.airAbsorptionGainHF);
            effectF(newEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, value.roomRolloffFactor);
            effectI(newEffect, AL_REVERB_DECAY_HFLIMIT, value.decayHFLimit ? AL_TRUE : AL_FALSE);
            error = alGetError();
            if (error != AL_NO_ERROR) {
                logAlError("reverb parameter setup", error);
                deleteEffects(1, &newEffect);
                return false;
            }
        }

        if (value.reverbEnabled) {
            clearAlErrors();
            for (auto& group : groups) {
                for (auto& playback : group.second) {
                    alSource3i(playback.second->source, AL_AUXILIARY_SEND_FILTER, static_cast<ALint>(effectSlot), 0, AL_FILTER_NULL);
                }
            }
            const ALenum routingError = alGetError();
            if (routingError != AL_NO_ERROR) {
                logAlError("reverb source routing", routingError);
                if (newEffect != 0) {
                    deleteEffects(1, &newEffect);
                }
                return false;
            }
        }

        clearAlErrors();
        effectSlotI(effectSlot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(newEffect));
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            logAlError("reverb slot update", error);
            if (newEffect != 0) {
                deleteEffects(1, &newEffect);
            }
            return false;
        }

        if (effect != 0) {
            clearAlErrors();
            deleteEffects(1, &effect);
            error = alGetError();
            if (error != AL_NO_ERROR) {
                logAlError("previous reverb effect release", error);
            }
        }
        effect = newEffect;
        environment = value;
        return true;
    }
};

Audio& Audio::get() {
    static Audio instance;
    return instance;
}

Audio& Audio::getInstance() {
    return get();
}

Audio::Audio() : impl(std::make_unique<Impl>()) {
}

Audio::~Audio() {
    shutdown();
}

bool Audio::init() {
    if (impl->initialized) {
        return true;
    }

    std::cout << "[Info] Not Engine Audio Init" << std::endl;

    impl->device = alcOpenDevice(nullptr);
    if (impl->device == nullptr) {
        std::cerr << "[Error] OpenAL: unable to open the default audio device" << std::endl;
        return false;
    }

    impl->context = alcCreateContext(impl->device, nullptr);
    if (impl->context == nullptr) {
        logAlcError("context creation", impl->device);
        alcCloseDevice(impl->device);
        impl->device = nullptr;
        return false;
    }

    if (alcMakeContextCurrent(impl->context) == ALC_FALSE) {
        logAlcError("making the context current", impl->device);
        alcDestroyContext(impl->context);
        alcCloseDevice(impl->device);
        impl->context = nullptr;
        impl->device = nullptr;
        return false;
    }

    clearAlErrors();
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    const ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        logAlError("distance model setup", error);
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(impl->context);
        alcCloseDevice(impl->device);
        impl->context = nullptr;
        impl->device = nullptr;
        return false;
    }

    impl->initialized = true;
    impl->initializeEfx();
    if (impl->environment.reverbEnabled && impl->efxAvailable && !impl->applyEnvironment(impl->environment)) {
        std::cerr << "[Warning] Failed to restore the previous EFX reverb environment" << std::endl;
    }
    return true;
}

void Audio::shutdown() {
    clear();

    if (!impl->initialized) {
        return;
    }

    impl->shutdownEfx();
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(impl->context);
    alcCloseDevice(impl->device);
    impl->context = nullptr;
    impl->device = nullptr;
    impl->initialized = false;
}

void Audio::clear() {
    stopAll();

    if (impl->initialized) {
        for (auto& clip : impl->clips) {
            clearAlErrors();
            const ALuint buffers[] = {clip.second.stereoBuffer, clip.second.monoBuffer};
            for (auto& buffer : buffers) {
                if (buffer != 0) {
                    alDeleteBuffers(1, &buffer);
                }
            }
            const ALenum error = alGetError();
            if (error != AL_NO_ERROR) {
                logAlError("buffer release", error);
            }
        }
    }
    impl->clips.clear();
}

bool Audio::isInitialized() const {
    return impl->initialized;
}

bool Audio::load(const std::string& path, const std::string& name) {
    if (!impl->initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return false;
    }

    if (path.empty() || name.empty()) {
        std::cerr << "[Error] Audio path and name must not be empty" << std::endl;
        return false;
    }

    if (impl->clips.find(name) != impl->clips.end()) {
        std::cerr << "[Error] Audio name is already loaded: " << name << std::endl;
        return false;
    }

    const ma_decoder_config config = ma_decoder_config_init(ma_format_s16, 2, 0);
    ma_decoder decoder{};
    ma_result result = ma_decoder_init_file(path.c_str(), &config, &decoder);
    if (result != MA_SUCCESS) {
        logMiniaudioError("Audio load failed", result);
        return false;
    }

    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;
    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);
    if (result != MA_SUCCESS || channels != 2 || sampleRate == 0) {
        if (result != MA_SUCCESS) {
            logMiniaudioError("Audio output format lookup failed", result);
        } else {
            std::cerr << "[Error] Audio decoder produced an unsupported output format" << std::endl;
        }
        ma_decoder_uninit(&decoder);
        return false;
    }

    ma_uint64 frameCount = 0;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (result != MA_SUCCESS) {
        logMiniaudioError("Audio could not determine clip length", result);
        ma_decoder_uninit(&decoder);
        return false;
    }

    if (frameCount == 0) {
        std::cerr << "[Error] Audio file contains no PCM frames: " << path << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    constexpr std::size_t bytesPerFrame = 2 * sizeof(std::int16_t);
    const auto maxBufferBytes = static_cast<std::uint64_t>(std::numeric_limits<ALsizei>::max());
    if (frameCount > std::numeric_limits<std::size_t>::max() / bytesPerFrame || frameCount > maxBufferBytes / bytesPerFrame ||
        sampleRate > static_cast<ma_uint32>(std::numeric_limits<ALsizei>::max())) {
        std::cerr << "[Error] Audio file is too large to preload: " << path << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    std::vector<std::int16_t> samples(static_cast<std::size_t>(frameCount) * 2);
    ma_uint64 framesRead = 0;
    result = ma_decoder_read_pcm_frames(&decoder, samples.data(), frameCount, &framesRead);
    ma_decoder_uninit(&decoder);
    if (result != MA_SUCCESS && result != MA_AT_END) {
        logMiniaudioError("Audio decode failed", result);
        return false;
    }

    if (framesRead == 0) {
        std::cerr << "[Error] Audio decoder produced no PCM frames: " << path << std::endl;
        return false;
    }

    std::vector<std::int16_t> monoSamples(static_cast<std::size_t>(framesRead));
    for (std::size_t frame = 0; frame < monoSamples.size(); ++frame) {
        const auto left = static_cast<std::int32_t>(samples[frame * 2]);
        const auto right = static_cast<std::int32_t>(samples[frame * 2 + 1]);
        monoSamples[frame] = static_cast<std::int16_t>((left + right) / 2);
    }

    const auto stereoByteCount = static_cast<ALsizei>(static_cast<std::size_t>(framesRead) * bytesPerFrame);
    const auto monoByteCount = static_cast<ALsizei>(static_cast<std::size_t>(framesRead) * sizeof(std::int16_t));
    ALuint buffers[2]{};
    clearAlErrors();
    alGenBuffers(2, buffers);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR || buffers[0] == 0 || buffers[1] == 0) {
        if (error != AL_NO_ERROR) {
            logAlError("buffer creation", error);
        } else {
            std::cerr << "[Error] OpenAL: buffer creation returned an invalid handle" << std::endl;
        }
        for (auto& buffer : buffers) {
            if (buffer != 0) {
                alDeleteBuffers(1, &buffer);
            }
        }
        return false;
    }

    clearAlErrors();
    alBufferData(buffers[0], AL_FORMAT_STEREO16, samples.data(), stereoByteCount, static_cast<ALsizei>(sampleRate));
    alBufferData(buffers[1], AL_FORMAT_MONO16, monoSamples.data(), monoByteCount, static_cast<ALsizei>(sampleRate));
    error = alGetError();
    if (error != AL_NO_ERROR) {
        logAlError("buffer upload", error);
        for (auto& buffer : buffers) {
            if (buffer != 0) {
                alDeleteBuffers(1, &buffer);
            }
        }
        return false;
    }

    impl->clips.emplace(name, Impl::Clip{buffers[0], buffers[1]});
    return true;
}

bool Audio::isLoaded(const std::string& name) const {
    return impl->clips.find(name) != impl->clips.end();
}

bool Audio::play(const std::string& name, const std::string& groupName, float volume, float pitch, bool loop) {
    if (!impl->initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return false;
    }

    if (name.empty() || groupName.empty() || !std::isfinite(volume) || volume < 0.0f || volume > 1.0f || !std::isfinite(pitch) || pitch <= 0.0f) {
        std::cerr << "[Error] Invalid audio playback request" << std::endl;
        return false;
    }

    return impl->play(name, groupName, nullptr, volume, pitch, loop, 1.0f, 0.0f, 100.0f);
}

bool Audio::play3D(const std::string& name, const std::string& groupName, const glm::vec3& position, float volume, float pitch, bool loop,
                   float referenceDistance, float rolloff, float maxDistance) {
    if (!impl->initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return false;
    }

    if (name.empty() || groupName.empty() || !isFinite(position) || !std::isfinite(volume) || volume < 0.0f || volume > 1.0f ||
        !std::isfinite(pitch) || pitch <= 0.0f || !std::isfinite(referenceDistance) || referenceDistance <= 0.0f || !std::isfinite(rolloff) ||
        rolloff < 0.0f || !std::isfinite(maxDistance) || maxDistance < referenceDistance) {
        std::cerr << "[Error] Invalid spatial audio playback request" << std::endl;
        return false;
    }

    return impl->play(name, groupName, &position, volume, pitch, loop, referenceDistance, rolloff, maxDistance);
}

bool Audio::setListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
    if (!impl->initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return false;
    }

    const float forwardLength = glm::length(forward);
    const glm::vec3 right = glm::cross(forward, up);
    const float rightLength = glm::length(right);
    if (!isFinite(position) || !isFinite(forward) || !isFinite(up) || forwardLength <= 0.0001f || rightLength <= 0.0001f) {
        std::cerr << "[Error] Invalid audio listener pose" << std::endl;
        return false;
    }

    const glm::vec3 normalizedForward = forward / forwardLength;
    const glm::vec3 normalizedRight = right / rightLength;
    const glm::vec3 normalizedUp = glm::normalize(glm::cross(normalizedRight, normalizedForward));
    const ALfloat orientation[] = {normalizedForward.x, normalizedForward.y, normalizedForward.z, normalizedUp.x, normalizedUp.y, normalizedUp.z};

    clearAlErrors();
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListenerfv(AL_ORIENTATION, orientation);
    const ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        logAlError("listener update", error);
        return false;
    }
    return true;
}

bool Audio::setEnvironment(const AudioEnvironment& environment) {
    if (!impl->initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return false;
    }
    if (!isValid(environment)) {
        std::cerr << "[Error] Invalid audio environment parameters" << std::endl;
        return false;
    }
    return impl->applyEnvironment(environment);
}

AudioEnvironment Audio::getEnvironment() const {
    return impl->environment;
}

void Audio::setSourcePosition(const std::string& name, const std::string& groupName, const glm::vec3& position) {
    if (!impl->initialized) {
        std::cerr << "[Error] Audio is not initialized" << std::endl;
        return;
    }
    if (name.empty() || groupName.empty() || !isFinite(position)) {
        std::cerr << "[Error] Invalid spatial audio source position" << std::endl;
        return;
    }

    const auto group = impl->groups.find(groupName);
    if (group == impl->groups.end()) {
        return;
    }

    clearAlErrors();
    const auto range = group->second.equal_range(name);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second->spatialized) {
            alSource3f(it->second->source, AL_POSITION, position.x, position.y, position.z);
        }
    }
    const ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        logAlError("source position update", error);
    }
}

void Audio::stop(const std::string& name) {
    if (name.empty()) {
        return;
    }

    for (auto group = impl->groups.begin(); group != impl->groups.end();) {
        const auto range = group->second.equal_range(name);
        for (auto it = range.first; it != range.second; ++it) {
            impl->releasePlayback(*it->second);
        }
        group->second.erase(range.first, range.second);

        if (group->second.empty()) {
            group = impl->groups.erase(group);
        } else {
            ++group;
        }
    }
}

void Audio::stopGroup(const std::string& groupName) {
    const auto group = impl->groups.find(groupName);
    if (group == impl->groups.end()) {
        return;
    }

    for (auto& playback : group->second) {
        impl->releasePlayback(*playback.second);
    }
    impl->groups.erase(group);
}

void Audio::stopInGroup(const std::string& name, const std::string& groupName) {
    const auto group = impl->groups.find(groupName);
    if (group == impl->groups.end()) {
        return;
    }

    const auto range = group->second.equal_range(name);
    if (range.first == range.second) {
        return;
    }

    for (auto it = range.first; it != range.second; ++it) {
        impl->releasePlayback(*it->second);
    }
    group->second.erase(range.first, range.second);
    if (group->second.empty()) {
        impl->groups.erase(group);
    }
}

void Audio::stopAll() {
    for (auto& group : impl->groups) {
        for (auto& playback : group.second) {
            impl->releasePlayback(*playback.second);
        }
    }
    impl->groups.clear();
}

void Audio::update() {
    if (!impl->initialized) {
        return;
    }

    for (auto group = impl->groups.begin(); group != impl->groups.end();) {
        for (auto playback = group->second.begin(); playback != group->second.end();) {
            if (!playback->second->loop) {
                ALint state = AL_INITIAL;
                clearAlErrors();
                alGetSourcei(playback->second->source, AL_SOURCE_STATE, &state);
                const ALenum error = alGetError();
                if (error != AL_NO_ERROR) {
                    logAlError("source state query", error);
                    ++playback;
                    continue;
                }

                if (state == AL_STOPPED) {
                    impl->releasePlayback(*playback->second);
                    playback = group->second.erase(playback);
                    continue;
                }
            }
            ++playback;
        }

        if (group->second.empty()) {
            group = impl->groups.erase(group);
        } else {
            ++group;
        }
    }
}

} // namespace knot
