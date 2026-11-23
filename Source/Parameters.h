#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace brute102::ParamIDs
{
    inline constexpr const char* retrigger = "retrigger";
    inline constexpr const char* glideMs = "glideMs";

    inline constexpr const char* subtractiveEnabled = "subtractiveEnabled";
    inline constexpr const char* osc1Wave = "osc1Wave";
    inline constexpr const char* osc2Coarse = "osc2Coarse";
    inline constexpr const char* osc2Detune = "osc2Detune";
    inline constexpr const char* osc2Level = "osc2Level";
    inline constexpr const char* subWave = "subWave";
    inline constexpr const char* subLevel = "subLevel";

    inline constexpr const char* fmEnabled = "fmEnabled";
    inline constexpr const char* fmRatio = "fmRatio";
    inline constexpr const char* fmIndex = "fmIndex";
    inline constexpr const char* fmDecayMs = "fmDecayMs";
    inline constexpr const char* fmMix = "fmMix";

    inline constexpr const char* cutoffHz = "cutoffHz";
    inline constexpr const char* resonance = "resonance";
    inline constexpr const char* envDepth = "envDepth";

    inline constexpr const char* filterDecayMs = "filterDecayMs";
    inline constexpr const char* ampDecayMs = "ampDecayMs";
    inline constexpr const char* ampReleaseMs = "ampReleaseMs";

    inline constexpr const char* drive = "drive";
    inline constexpr const char* shaperMode = "shaperMode";
    inline constexpr const char* masterVol = "masterVol";
}

namespace brute102
{
    inline float fmRatioFromChoice(int idx) noexcept
    {
        switch (idx)
        {
        case 0:  return 0.5f;
        case 1:  return 1.0f;
        case 2:  return 2.0f;
        case 3:  return 3.0f;
        default: return 1.0f;
        }
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ ParamIDs::retrigger, 1 }, "Retrigger",
            juce::StringArray{ "OFF", "ON" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::glideMs, 1 }, "Glide",
            juce::NormalisableRange<float>(0.0f, 300.0f, 1.0f), 8.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ ParamIDs::subtractiveEnabled, 1 }, "Subtractive On",
            juce::StringArray{ "OFF", "ON" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ ParamIDs::osc1Wave, 1 }, "Osc 1 Wave",
            juce::StringArray{ "SAW", "SQR" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::osc2Coarse, 1 }, "Osc 2 Coarse",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("st")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::osc2Detune, 1 }, "Osc 2 Detune",
            juce::NormalisableRange<float>(-50.0f, 50.0f, 1.0f), 8.0f,
            juce::AudioParameterFloatAttributes().withLabel("ct")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::osc2Level, 1 }, "Osc 2 Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.68f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ ParamIDs::subWave, 1 }, "Sub Wave",
            juce::StringArray{ "SQR", "SIN" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::subLevel, 1 }, "Sub Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.78f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ ParamIDs::fmEnabled, 1 }, "FM On",
            juce::StringArray{ "OFF", "ON" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ ParamIDs::fmRatio, 1 }, "FM Ratio",
            juce::StringArray{ "0.5", "1.0", "2.0", "3.0" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::fmIndex, 1 }, "Mod Index",
            juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 1.4f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::fmDecayMs, 1 }, "FM Decay",
            juce::NormalisableRange<float>(10.0f, 500.0f, 0.1f, 0.35f), 45.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::fmMix, 1 }, "FM Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.22f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::cutoffHz, 1 }, "Cutoff",
            juce::NormalisableRange<float>(20.0f, 18000.0f, 0.1f, 0.25f), 260.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::resonance, 1 }, "Resonance",
            juce::NormalisableRange<float>(0.0f, 0.9f, 0.001f), 0.36f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::envDepth, 1 }, "Env Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.72f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::filterDecayMs, 1 }, "Filter Decay",
            juce::NormalisableRange<float>(15.0f, 1000.0f, 0.1f, 0.35f), 145.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::ampDecayMs, 1 }, "Amp Decay",
            juce::NormalisableRange<float>(30.0f, 1500.0f, 0.1f, 0.35f), 220.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::ampReleaseMs, 1 }, "Amp Release",
            juce::NormalisableRange<float>(5.0f, 500.0f, 0.1f, 0.35f), 45.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::drive, 1 }, "Drive",
            juce::NormalisableRange<float>(1.0f, 10.0f, 0.01f), 2.8f,
            juce::AudioParameterFloatAttributes().withLabel("x")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ ParamIDs::shaperMode, 1 }, "Clip Mode",
            juce::StringArray{ "SOFT TANH", "HARD CLAMP" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ ParamIDs::masterVol, 1 }, "Master",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.78f));

        return { params.begin(), params.end() };
    }
}
