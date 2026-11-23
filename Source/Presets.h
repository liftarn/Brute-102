#pragma once

#include <array>
#include "Parameters.h"

namespace brute102
{
    struct PresetValues
    {
        const char* name;

        bool retrigger;
        float glideMs;

        bool subtractiveEnabled;
        int osc1Wave;      // 0 = SAW, 1 = SQR
        float osc2Coarse;
        float osc2Detune;
        float osc2Level;
        int subWave;       // 0 = SQR, 1 = SIN
        float subLevel;

        bool fmEnabled;
        int fmRatioIdx;    // 0 = 0.5, 1 = 1.0, 2 = 2.0, 3 = 3.0
        float fmIndex;
        float fmDecayMs;
        float fmMix;

        float cutoffHz;
        float resonance;
        float envDepth;

        float filterDecayMs;
        float ampDecayMs;
        float ampReleaseMs;

        float drive;
        int shaperMode;    // 0 = Soft Tanh, 1 = Hard Clamp
        float masterVol;
    };

    inline constexpr std::array<PresetValues, 8> kPresets {{
        {
            "Iron Sequence",
            true, 8.0f,
            true, 1, 0.0f, 8.0f, 0.68f, 0, 0.78f,
            true, 1, 1.4f, 45.0f, 0.22f,
            260.0f, 0.36f, 0.72f,
            145.0f, 220.0f, 45.0f,
            2.8f, 0, 0.78f
        },
        {
            "Clang Bass",
            true, 0.0f,
            true, 0, 0.0f, -11.0f, 0.42f, 1, 0.85f,
            true, 3, 7.8f, 185.0f, 0.84f,
            680.0f, 0.24f, 0.58f,
            240.0f, 320.0f, 80.0f,
            3.4f, 1, 0.75f
        },
        {
            "Acid Body",
            false, 78.0f,
            true, 0, -12.0f, 14.0f, 0.52f, 0, 0.60f,
            true, 2, 2.1f, 70.0f, 0.24f,
            165.0f, 0.82f, 0.88f,
            360.0f, 420.0f, 95.0f,
            4.6f, 0, 0.74f
        },
        {
            "Sub Monolith",
            true, 12.0f,
            true, 1, 7.0f, -6.0f, 0.48f, 1, 0.95f,
            true, 0, 1.8f, 65.0f, 0.32f,
            190.0f, 0.28f, 0.52f,
            130.0f, 280.0f, 55.0f,
            5.8f, 0, 0.76f
        },
        {
            "Steel Anvil",
            true, 0.0f,
            true, 0, 12.0f, 15.0f, 0.38f, 0, 0.72f,
            true, 2, 10.4f, 135.0f, 0.95f,
            920.0f, 0.18f, 0.62f,
            190.0f, 250.0f, 40.0f,
            6.4f, 1, 0.72f
        },
        {
            "Kraft Glide",
            false, 62.0f,
            true, 0, 0.0f, 18.0f, 0.78f, 0, 0.65f,
            true, 1, 2.6f, 50.0f, 0.28f,
            310.0f, 0.58f, 0.76f,
            210.0f, 310.0f, 75.0f,
            2.1f, 0, 0.78f
        },
        {
            "FM Clang",
            true, 0.0f,
            false, 0, 0.0f, 0.0f, 0.0f, 0, 0.0f,
            true, 1, 6.4f, 260.0f, 1.0f,
            1100.0f, 0.22f, 0.65f,
            280.0f, 360.0f, 85.0f,
            4.5f, 1, 0.85f
        },
        {
            "Analog Sub",
            true, 14.0f,
            true, 0, 0.0f, 12.0f, 0.85f, 0, 0.85f,
            false, 1, 0.0f, 95.0f, 0.0f,
            240.0f, 0.48f, 0.78f,
            220.0f, 320.0f, 65.0f,
            3.2f, 0, 0.78f
        }
    }};

    inline void applyPresetToAPVTS(juce::AudioProcessorValueTreeState& apvts, int presetIndex)
    {
        if (presetIndex < 0 || presetIndex >= static_cast<int>(kPresets.size()))
            return;

        const auto& p = kPresets[static_cast<size_t>(presetIndex)];
        auto setVal = [&apvts](const char* id, float plainVal)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(plainVal));
        };

        setVal(ParamIDs::retrigger,          p.retrigger ? 1.0f : 0.0f);
        setVal(ParamIDs::glideMs,            p.glideMs);
        setVal(ParamIDs::subtractiveEnabled, p.subtractiveEnabled ? 1.0f : 0.0f);
        setVal(ParamIDs::osc1Wave,           static_cast<float>(p.osc1Wave));
        setVal(ParamIDs::osc2Coarse,         p.osc2Coarse);
        setVal(ParamIDs::osc2Detune,         p.osc2Detune);
        setVal(ParamIDs::osc2Level,          p.osc2Level);
        setVal(ParamIDs::subWave,            static_cast<float>(p.subWave));
        setVal(ParamIDs::subLevel,           p.subLevel);
        setVal(ParamIDs::fmEnabled,          p.fmEnabled ? 1.0f : 0.0f);
        setVal(ParamIDs::fmRatio,            static_cast<float>(p.fmRatioIdx));
        setVal(ParamIDs::fmIndex,            p.fmIndex);
        setVal(ParamIDs::fmDecayMs,          p.fmDecayMs);
        setVal(ParamIDs::fmMix,              p.fmMix);
        setVal(ParamIDs::cutoffHz,           p.cutoffHz);
        setVal(ParamIDs::resonance,          p.resonance);
        setVal(ParamIDs::envDepth,           p.envDepth);
        setVal(ParamIDs::filterDecayMs,      p.filterDecayMs);
        setVal(ParamIDs::ampDecayMs,         p.ampDecayMs);
        setVal(ParamIDs::ampReleaseMs,       p.ampReleaseMs);
        setVal(ParamIDs::drive,              p.drive);
        setVal(ParamIDs::shaperMode,         static_cast<float>(p.shaperMode));
        setVal(ParamIDs::masterVol,          p.masterVol);
    }
}
