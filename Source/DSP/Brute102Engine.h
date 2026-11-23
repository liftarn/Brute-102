#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace brute102
{
    struct EngineParams
    {
        bool retrigger          = true;
        float glideMs           = 8.0f;

        bool subtractiveEnabled = true;
        int osc1Wave            = 1;     // 0 = Saw, 1 = Square
        float osc2Coarse        = 0.0f;  // -24..+24 st
        float osc2Detune        = 8.0f;  // -50..+50 ct
        float osc2Level         = 0.68f;
        int subWave             = 0;     // 0 = Square, 1 = Sine
        float subLevel          = 0.78f;

        bool fmEnabled          = true;
        float fmRatio           = 1.0f;  // 0.5, 1.0, 2.0, 3.0
        float fmIndex           = 1.4f;
        float fmDecayMs         = 45.0f;
        float fmMix             = 0.22f;

        float cutoffHz          = 260.0f;
        float resonance         = 0.36f;
        float envDepth          = 0.72f;

        float filterDecayMs     = 145.0f;
        float ampDecayMs        = 220.0f;
        float ampReleaseMs      = 45.0f;

        float drive             = 2.8f;
        int shaperMode          = 0;     // 0 = Soft tanh, 1 = Hard clamp
        float masterVol         = 0.78f;
    };

    class Brute102Engine
    {
    public:
        static constexpr int kMaxNoteStack = 16;

        void prepare(double newSampleRate) noexcept
        {
            sampleRate = std::max(8000.0, newSampleRate);
            invSampleRate = 1.0 / sampleRate;

            constexpr double smoothSec = 0.008;
            subtractiveGateSmooth.reset(sampleRate, smoothSec);
            fmGateSmooth.reset(sampleRate, smoothSec);
            osc2CoarseSmooth.reset(sampleRate, smoothSec);
            osc2DetuneSmooth.reset(sampleRate, smoothSec);
            osc2LevelSmooth.reset(sampleRate, smoothSec);
            subLevelSmooth.reset(sampleRate, smoothSec);
            fmIndexSmooth.reset(sampleRate, smoothSec);
            fmMixSmooth.reset(sampleRate, smoothSec);
            cutoffHzSmooth.reset(sampleRate, smoothSec);
            resonanceSmooth.reset(sampleRate, smoothSec);
            envDepthSmooth.reset(sampleRate, smoothSec);
            driveSmooth.reset(sampleRate, smoothSec);
            masterVolSmooth.reset(sampleRate, smoothSec);

            // Fixed ~150 Hz 12 dB/oct ZDF SVF High-Pass Filter coefficients for FM layer
            constexpr double hpfCutoff = 150.0;
            const double g = std::tan(juce::MathConstants<double>::pi * hpfCutoff * invSampleRate);
            constexpr double r = 0.7071067811865475; // Butterworth Q = 1/sqrt(2)
            hpfK = 2.0 * r;
            hpfA1 = 1.0 / (1.0 + g * (g + hpfK));
            hpfA2 = g * hpfA1;
            hpfA3 = g * hpfA2;

            attackStep = 1.0 / std::max(1.0, 0.001 * sampleRate);
            filtFastRelCoeff = std::exp(-4.60517 / (0.003 * sampleRate));

            resetState();
            updateEnvelopeCoefficients();
        }

        void resetState() noexcept
        {
            noteCount = 0;
            activeNote = -1;
            gate = false;
            hasPlayedOnce = false;

            osc1Phase = 0.0;
            osc2Phase = 0.0;
            subHalfCycle = 0;
            fmCarPhase = 0.0;
            fmModPhase = 0.0;

            hpfIc1 = 0.0;
            hpfIc2 = 0.0;
            ladS1 = ladS2 = ladS3 = ladS4 = 0.0;

            filtEnvState = EnvState::Idle;
            filtEnvVal = 0.0;
            fmEnvState = EnvState::Idle;
            fmEnvVal = 0.0;
            ampEnvState = EnvState::Idle;
            ampEnvVal = 0.0;
        }

        void setParameters(const EngineParams& p) noexcept
        {
            retrigger = p.retrigger;
            osc1Wave = p.osc1Wave;
            subWave = p.subWave;
            fmRatio = p.fmRatio;
            shaperMode = p.shaperMode;

            subtractiveGateSmooth.setTargetValue(p.subtractiveEnabled ? 1.0f : 0.0f);
            fmGateSmooth.setTargetValue(p.fmEnabled ? 1.0f : 0.0f);
            osc2CoarseSmooth.setTargetValue(p.osc2Coarse);
            osc2DetuneSmooth.setTargetValue(p.osc2Detune);
            osc2LevelSmooth.setTargetValue(p.osc2Level);
            subLevelSmooth.setTargetValue(p.subLevel);
            fmIndexSmooth.setTargetValue(p.fmIndex);
            fmMixSmooth.setTargetValue(p.fmMix);
            cutoffHzSmooth.setTargetValue(p.cutoffHz);
            resonanceSmooth.setTargetValue(p.resonance);
            envDepthSmooth.setTargetValue(p.envDepth);
            driveSmooth.setTargetValue(p.drive);
            masterVolSmooth.setTargetValue(p.masterVol);

            if (glideMs != p.glideMs || fmDecayMs != p.fmDecayMs ||
                filterDecayMs != p.filterDecayMs || ampDecayMs != p.ampDecayMs ||
                ampReleaseMs != p.ampReleaseMs)
            {
                glideMs = p.glideMs;
                fmDecayMs = p.fmDecayMs;
                filterDecayMs = p.filterDecayMs;
                ampDecayMs = p.ampDecayMs;
                ampReleaseMs = p.ampReleaseMs;
                updateEnvelopeCoefficients();
            }
        }

        void noteOn(int note) noexcept
        {
            for (int i = 0; i < noteCount; ++i)
            {
                if (noteStack[static_cast<size_t>(i)] == note)
                {
                    for (int j = i; j < noteCount - 1; ++j)
                        noteStack[static_cast<size_t>(j)] = noteStack[static_cast<size_t>(j + 1)];
                    --noteCount;
                    break;
                }
            }
            if (noteCount < kMaxNoteStack)
                noteStack[static_cast<size_t>(noteCount++)] = note;

            const bool wasGateOpen = gate;
            gate = true;
            activeNote = note;
            targetMidi = static_cast<double>(note);

            if (!hasPlayedOnce)
            {
                currentMidi = targetMidi;
                hasPlayedOnce = true;
            }

            if (!wasGateOpen || retrigger)
                triggerEnvelopes();
        }

        void noteOff(int note) noexcept
        {
            for (int i = 0; i < noteCount; ++i)
            {
                if (noteStack[static_cast<size_t>(i)] == note)
                {
                    for (int j = i; j < noteCount - 1; ++j)
                        noteStack[static_cast<size_t>(j)] = noteStack[static_cast<size_t>(j + 1)];
                    --noteCount;
                    break;
                }
            }

            if (noteCount > 0)
            {
                const int topNote = noteStack[static_cast<size_t>(noteCount - 1)];
                if (topNote != activeNote)
                {
                    activeNote = topNote;
                    targetMidi = static_cast<double>(topNote);
                    if (retrigger)
                        triggerEnvelopes();
                }
            }
            else
            {
                gate = false;
                activeNote = -1;
                if (ampEnvState != EnvState::Idle)  ampEnvState  = EnvState::Release;
                if (filtEnvState != EnvState::Idle) filtEnvState = EnvState::Release;
                if (fmEnvState != EnvState::Idle)   fmEnvState   = EnvState::Release;
            }
        }

        void allNotesOff() noexcept
        {
            noteCount = 0;
            gate = false;
            activeNote = -1;
            if (ampEnvState != EnvState::Idle)  ampEnvState  = EnvState::Release;
            if (filtEnvState != EnvState::Idle) filtEnvState = EnvState::Release;
            if (fmEnvState != EnvState::Idle)   fmEnvState   = EnvState::Release;
        }

        float processSample() noexcept
        {
            // 1. Per-sample parameter smoothing
            const double subGate    = subtractiveGateSmooth.getNextValue();
            const double fmGate     = fmGateSmooth.getNextValue();
            const double osc2Coarse = osc2CoarseSmooth.getNextValue();
            const double osc2Detune = osc2DetuneSmooth.getNextValue();
            const double osc2Lvl    = osc2LevelSmooth.getNextValue();
            const double subLvl     = subLevelSmooth.getNextValue();
            const double fmIdx      = fmIndexSmooth.getNextValue();
            const double fmMixVal   = fmMixSmooth.getNextValue();
            const double baseCutHz  = cutoffHzSmooth.getNextValue();
            const double reso       = resonanceSmooth.getNextValue();
            const double envDep     = envDepthSmooth.getNextValue();
            const double drv        = driveSmooth.getNextValue();
            const double master     = masterVolSmooth.getNextValue();

            // 2. Exponential pitch glide
            currentMidi += (targetMidi - currentMidi) * glideCoeff;

            // 3. Envelopes
            stepEnvelopes();

            if (ampEnvState == EnvState::Idle && ampEnvVal == 0.0)
                return 0.0f;

            // 4. Pitch & phase increments
            const double freq1 = 440.0 * std::pow(2.0, (currentMidi - 69.0) * (1.0 / 12.0));
            const double dt1 = freq1 * invSampleRate;

            const double osc2Semi = osc2Coarse + osc2Detune * 0.01;
            const double freq2 = freq1 * std::pow(2.0, osc2Semi * (1.0 / 12.0));
            const double dt2 = freq2 * invSampleRate;

            osc1Phase += dt1;
            if (osc1Phase >= 1.0)
            {
                osc1Phase -= 1.0;
                subHalfCycle ^= 1;
            }

            osc2Phase += dt2;
            if (osc2Phase >= 1.0)
                osc2Phase -= 1.0;

            // 5. Subtractive Oscillators (PolyBLEP)
            double osc1Sample = 0.0;
            if (osc1Wave == 0)
            {
                osc1Sample = 2.0 * osc1Phase - 1.0 - polyBlep(osc1Phase, dt1);
            }
            else
            {
                double pShift = osc1Phase + 0.5;
                if (pShift >= 1.0) pShift -= 1.0;
                osc1Sample = (osc1Phase < 0.5 ? 1.0 : -1.0)
                           + polyBlep(osc1Phase, dt1)
                           - polyBlep(pShift, dt1);
            }

            const double osc2Sample =
                (2.0 * osc2Phase - 1.0 - polyBlep(osc2Phase, dt2)) * osc2Lvl;

            // Phase-locked Sub Oscillator (-12 st)
            const double subPhase = 0.5 * (osc1Phase + static_cast<double>(subHalfCycle));
            const double dtSub = dt1 * 0.5;
            double subSample = 0.0;
            if (subWave == 0)
            {
                double pShiftSub = subPhase + 0.5;
                if (pShiftSub >= 1.0) pShiftSub -= 1.0;
                subSample = (subPhase < 0.5 ? 1.0 : -1.0)
                          + polyBlep(subPhase, dtSub)
                          - polyBlep(pShiftSub, dtSub);
            }
            else
            {
                subSample = std::sin(juce::MathConstants<double>::twoPi * subPhase);
            }
            subSample *= subLvl;

            // 6. Parallel 2-Op FM Layer + Fixed ~150 Hz 12 dB HPF
            fmCarPhase += dt1;
            if (fmCarPhase >= 1.0) fmCarPhase -= 1.0;

            fmModPhase += dt1 * static_cast<double>(fmRatio);
            if (fmModPhase >= 1.0) fmModPhase -= std::floor(fmModPhase);

            const double modOut = std::sin(juce::MathConstants<double>::twoPi * fmModPhase);
            const double effIndex = fmIdx * fmEnvVal;
            const double fmRaw = std::sin(juce::MathConstants<double>::twoPi * fmCarPhase + effIndex * modOut);

            const double v3Hpf = fmRaw - hpfIc2;
            const double v1Hpf = hpfA1 * hpfIc1 + hpfA2 * v3Hpf;
            const double v2Hpf = hpfIc2 + hpfA2 * hpfIc1 + hpfA3 * v3Hpf;
            hpfIc1 = 2.0 * v1Hpf - hpfIc1;
            hpfIc2 = 2.0 * v2Hpf - hpfIc2;
            const double fmHighPassed = fmRaw - hpfK * v1Hpf - v2Hpf;

            const double subtractiveBus = (osc1Sample * 0.5 + osc2Sample * 0.45 + subSample * 0.55) * subGate;
            const double fmBus = fmHighPassed * fmMixVal * 0.65 * fmGate;
            const double oscMix = (subtractiveBus + fmBus) * 0.65;

            // 7. 24 dB/oct ZDF 4-Pole Ladder Low-Pass Filter
            const double maxFc = std::min(18000.0, sampleRate * 0.44);
            const double fc = std::clamp(baseCutHz * std::pow(2.0, envDep * 6.0 * filtEnvVal), 20.0, maxFc);

            const double g = std::tan(juce::MathConstants<double>::pi * fc * invSampleRate);
            const double G = g / (1.0 + g);
            const double G2 = G * G;
            const double G3 = G2 * G;
            const double G4 = G3 * G;
            const double k = reso * 4.0;

            const double S = G3 * ladS1 + G2 * ladS2 + G * ladS3 + ladS4;
            const double compensatedInput = oscMix * (1.0 + k * 0.2);
            const double u = std::tanh((compensatedInput - k * S) / (1.0 + k * G4));

            const double v1 = (u - ladS1) * G;
            const double y1 = v1 + ladS1;
            ladS1 = y1 + v1;

            const double v2 = (y1 - ladS2) * G;
            const double y2 = v2 + ladS2;
            ladS2 = y2 + v2;

            const double v3 = (y2 - ladS3) * G;
            const double y3 = v3 + ladS3;
            ladS3 = y3 + v3;

            const double v4 = (y3 - ladS4) * G;
            const double y4 = v4 + ladS4;
            ladS4 = y4 + v4;

            // 8. Amp Envelope + Post-filter Grit & Waveshaper + Master Volume
            const double preDrive = y4 * ampEnvVal;
            const double driven = preDrive * drv;
            const double shaped = (shaperMode == 0)
                ? std::tanh(driven)
                : std::clamp(driven, -1.0, 1.0);

            const double driveComp = 1.0 / (1.0 + 0.12 * (drv - 1.0));
            return static_cast<float>(shaped * driveComp * master * 0.85);
        }

    private:
        enum class EnvState
        {
            Idle,
            Attack,
            Decay,
            Sustain,
            Release
        };

        static double polyBlep(double t, double dt) noexcept
        {
            if (t < dt)
            {
                const double x = t / dt;
                return x + x - x * x - 1.0;
            }
            if (t > 1.0 - dt)
            {
                const double x = (t - 1.0) / dt;
                return x * x + x + x + 1.0;
            }
            return 0.0;
        }

        void triggerEnvelopes() noexcept
        {
            filtEnvState = EnvState::Attack;
            fmEnvState   = EnvState::Attack;
            ampEnvState  = EnvState::Attack;
        }

        void updateEnvelopeCoefficients() noexcept
        {
            if (glideMs <= 0.2f)
            {
                glideCoeff = 1.0;
            }
            else
            {
                const double glideSec = static_cast<double>(glideMs) * 0.001;
                glideCoeff = 1.0 - std::exp(-3.0 / (glideSec * sampleRate));
            }

            fmDecayCoeff   = std::exp(-4.60517 / (std::max(0.01,  static_cast<double>(fmDecayMs)     * 0.001) * sampleRate));
            filtDecayCoeff = std::exp(-4.60517 / (std::max(0.015, static_cast<double>(filterDecayMs) * 0.001) * sampleRate));
            ampDecayCoeff  = std::exp(-4.60517 / (std::max(0.03,  static_cast<double>(ampDecayMs)    * 0.001) * sampleRate));
            ampRelCoeff    = std::exp(-4.60517 / (std::max(0.005, static_cast<double>(ampReleaseMs)  * 0.001) * sampleRate));
        }

        void stepEnvelopes() noexcept
        {
            switch (filtEnvState)
            {
                case EnvState::Attack:
                    filtEnvVal += attackStep;
                    if (filtEnvVal >= 1.0) { filtEnvVal = 1.0; filtEnvState = EnvState::Decay; }
                    break;
                case EnvState::Decay:
                    filtEnvVal *= filtDecayCoeff;
                    if (filtEnvVal < 1e-5) { filtEnvVal = 0.0; filtEnvState = EnvState::Idle; }
                    break;
                case EnvState::Release:
                    filtEnvVal *= filtFastRelCoeff;
                    if (filtEnvVal < 1e-5) { filtEnvVal = 0.0; filtEnvState = EnvState::Idle; }
                    break;
                case EnvState::Idle:
                case EnvState::Sustain:
                    break;
            }

            switch (fmEnvState)
            {
                case EnvState::Attack:
                    fmEnvVal += attackStep;
                    if (fmEnvVal >= 1.0) { fmEnvVal = 1.0; fmEnvState = EnvState::Decay; }
                    break;
                case EnvState::Decay:
                case EnvState::Release:
                    fmEnvVal *= fmDecayCoeff;
                    if (fmEnvVal < 1e-5) { fmEnvVal = 0.0; fmEnvState = EnvState::Idle; }
                    break;
                case EnvState::Idle:
                case EnvState::Sustain:
                    break;
            }

            switch (ampEnvState)
            {
                case EnvState::Attack:
                    ampEnvVal += attackStep;
                    if (ampEnvVal >= 1.0) { ampEnvVal = 1.0; ampEnvState = EnvState::Decay; }
                    break;
                case EnvState::Decay:
                    ampEnvVal = 0.7 + (ampEnvVal - 0.7) * ampDecayCoeff;
                    if (ampEnvVal - 0.7 < 1e-4) { ampEnvVal = 0.7; ampEnvState = EnvState::Sustain; }
                    break;
                case EnvState::Release:
                    ampEnvVal *= ampRelCoeff;
                    if (ampEnvVal < 1e-5) { ampEnvVal = 0.0; ampEnvState = EnvState::Idle; }
                    break;
                case EnvState::Idle:
                case EnvState::Sustain:
                    break;
            }
        }

        double sampleRate = 44100.0;
        double invSampleRate = 1.0 / 44100.0;

        std::array<int, kMaxNoteStack> noteStack {};
        int noteCount = 0;
        int activeNote = -1;
        bool gate = false;
        bool hasPlayedOnce = false;

        bool retrigger = true;
        float glideMs = 8.0f;
        double targetMidi = 36.0;
        double currentMidi = 36.0;
        double glideCoeff = 1.0;

        double osc1Phase = 0.0;
        double osc2Phase = 0.0;
        int subHalfCycle = 0;
        double fmCarPhase = 0.0;
        double fmModPhase = 0.0;

        int osc1Wave = 1;
        int subWave = 0;
        float fmRatio = 1.0f;
        int shaperMode = 0;

        float fmDecayMs = 45.0f;
        float filterDecayMs = 145.0f;
        float ampDecayMs = 220.0f;
        float ampReleaseMs = 45.0f;

        double attackStep = 1.0 / 44.1;
        double filtFastRelCoeff = 0.99;
        double fmDecayCoeff = 0.999;
        double filtDecayCoeff = 0.999;
        double ampDecayCoeff = 0.999;
        double ampRelCoeff = 0.999;

        EnvState filtEnvState = EnvState::Idle;
        double filtEnvVal = 0.0;
        EnvState fmEnvState = EnvState::Idle;
        double fmEnvVal = 0.0;
        EnvState ampEnvState = EnvState::Idle;
        double ampEnvVal = 0.0;

        double hpfK = 1.41421356237;
        double hpfA1 = 0.0, hpfA2 = 0.0, hpfA3 = 0.0;
        double hpfIc1 = 0.0, hpfIc2 = 0.0;

        double ladS1 = 0.0, ladS2 = 0.0, ladS3 = 0.0, ladS4 = 0.0;

        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> subtractiveGateSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fmGateSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> osc2CoarseSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> osc2DetuneSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> osc2LevelSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> subLevelSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fmIndexSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fmMixSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> cutoffHzSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> resonanceSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> envDepthSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmooth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterVolSmooth;
    };
}
