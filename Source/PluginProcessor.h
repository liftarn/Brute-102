#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "DSP/Brute102Engine.h"
#include "Parameters.h"

class Brute102AudioProcessor : public juce::AudioProcessor
{
public:
    struct UiMidiEvent
    {
        int note = 0;
        bool isNoteOn = false;
    };

    Brute102AudioProcessor();
    ~Brute102AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Lock-free UI <-> Audio communication (zero mutexes in processBlock)
    void pushUiMidiEvent(int note, bool isNoteOn) noexcept;
    void pushUiAllNotesOff() noexcept;
    float getOutputPeak() const noexcept { return outputPeak.load(std::memory_order_relaxed); }
    uint64_t getDawNotesLow() const noexcept { return dawNotesLow.load(std::memory_order_relaxed); }
    uint64_t getDawNotesHigh() const noexcept { return dawNotesHigh.load(std::memory_order_relaxed); }

private:
    void setDawNoteBit(int note, bool isOn) noexcept;

    juce::AudioProcessorValueTreeState apvts;
    brute102::Brute102Engine engine;

    std::atomic<float>* retriggerParam          = nullptr;
    std::atomic<float>* glideMsParam            = nullptr;
    std::atomic<float>* subtractiveEnabledParam = nullptr;
    std::atomic<float>* osc1WaveParam           = nullptr;
    std::atomic<float>* osc2CoarseParam         = nullptr;
    std::atomic<float>* osc2DetuneParam         = nullptr;
    std::atomic<float>* osc2LevelParam          = nullptr;
    std::atomic<float>* subWaveParam            = nullptr;
    std::atomic<float>* subLevelParam           = nullptr;
    std::atomic<float>* fmEnabledParam          = nullptr;
    std::atomic<float>* fmRatioParam            = nullptr;
    std::atomic<float>* fmIndexParam            = nullptr;
    std::atomic<float>* fmDecayMsParam          = nullptr;
    std::atomic<float>* fmMixParam              = nullptr;
    std::atomic<float>* cutoffHzParam           = nullptr;
    std::atomic<float>* resonanceParam          = nullptr;
    std::atomic<float>* envDepthParam           = nullptr;
    std::atomic<float>* filterDecayMsParam      = nullptr;
    std::atomic<float>* ampDecayMsParam         = nullptr;
    std::atomic<float>* ampReleaseMsParam       = nullptr;
    std::atomic<float>* driveParam              = nullptr;
    std::atomic<float>* shaperModeParam         = nullptr;
    std::atomic<float>* masterVolParam          = nullptr;

    std::atomic<int> currentProgramIndex { 0 };

    static constexpr int kUiMidiFifoSize = 64;
    juce::AbstractFifo uiMidiFifo { kUiMidiFifoSize };
    std::array<UiMidiEvent, kUiMidiFifoSize> uiMidiBuffer {};
    std::atomic<bool> pendingAllNotesOff { false };

    std::atomic<float> outputPeak { 0.0f };
    std::atomic<uint64_t> dawNotesLow { 0 };
    std::atomic<uint64_t> dawNotesHigh { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Brute102AudioProcessor)
};
