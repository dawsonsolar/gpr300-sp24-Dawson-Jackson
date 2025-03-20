#pragma once

#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dawslib
{
    enum class EasingMethod
    {
        Lerp,
        EaseInOutSine,
        EaseInOutQuart,
        EaseInOutBack
    };

    struct Vec3Key
    {
        float mTime = 0.0f;
        glm::vec3 mValue = glm::vec3(0.0f);
        int mMethod = 0;

        Vec3Key(float time = 0.0f, glm::vec3 value = glm::vec3(0.0f), int method = 0)
            : mTime(time), mValue(value), mMethod(method) {}
    };

    struct AnimationClip
    {
        float duration = 0.0f;
        std::vector<Vec3Key> positionKeys;
        std::vector<Vec3Key> rotationKeys;
        std::vector<Vec3Key> scaleKeys;
    };

    class Animator
    {
    public:
        AnimationClip* clip;
        bool isPlaying = false;
        bool isLooping = false;
        float playbackSpeed = 1.0f;
        float playbackTime = 0.0f;

        Animator() : clip(new AnimationClip) {}
        ~Animator() { delete clip; }

        void Update(float dt)
        {
            if (!isPlaying || !clip) return;

            playbackTime += dt * playbackSpeed;

            if (playbackTime > clip->duration)
            {
                playbackTime = isLooping ? 0.0f : clip->duration;
                isPlaying = isLooping;
            }
            else if (playbackTime < 0.0f)
            {
                playbackTime = isLooping ? clip->duration : 0.0f;
                isPlaying = isLooping;
            }
        }

        glm::vec3 GetValue(const std::vector<Vec3Key>& keyFrames, const glm::vec3& fallBackValue) const
        {
            if (keyFrames.empty()) return fallBackValue;
            if (keyFrames.size() == 1) return keyFrames.front().mValue;

            for (size_t i = 1; i < keyFrames.size(); ++i)
            {
                if (keyFrames[i].mTime > playbackTime)
                {
                    const Vec3Key& prev = keyFrames[i - 1];
                    const Vec3Key& next = keyFrames[i];

                    float t = (next.mTime - prev.mTime) != 0 ? (playbackTime - prev.mTime) / (next.mTime - prev.mTime) : 0.0f;
                    return Easing(prev.mValue, next.mValue, t, static_cast<EasingMethod>(prev.mMethod));
                }
            }
            return keyFrames.back().mValue;
        }

    private:
        static glm::vec3 Easing(const glm::vec3& a, const glm::vec3& b, float t, EasingMethod method)
        {
            switch (method)
            {
            case EasingMethod::Lerp: return glm::mix(a, b, t);
            case EasingMethod::EaseInOutSine: return EaseInOutSine(a, b, t);
            case EasingMethod::EaseInOutQuart: return EaseInOutQuart(a, b, t);
            case EasingMethod::EaseInOutBack: return EaseInOutBack(a, b, t);
            default: return glm::mix(a, b, t);
            }
        }

        static glm::vec3 EaseInOutSine(const glm::vec3& a, const glm::vec3& b, float t)
        {
            return glm::mix(a, b, -(std::cos(M_PI * t) - 1) / 2.0f);
        }

        static glm::vec3 EaseInOutQuart(const glm::vec3& a, const glm::vec3& b, float t)
        {
            return glm::mix(a, b, t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4) / 2.0f);
        }

        static glm::vec3 EaseInOutBack(const glm::vec3& a, const glm::vec3& b, float t)
        {
            constexpr float c1 = 1.70158f;
            constexpr float c2 = c1 * 1.525f;
            return glm::mix(a, b, t < 0.5f
                ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
                : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (2.0f * t - 2.0f) + c2) + 2.0f) / 2.0f);
        }
    };
}
