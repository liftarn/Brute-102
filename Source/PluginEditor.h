#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include "PluginProcessor.h"
#include "UI/BruteLookAndFeel.h"
#include "UI/BruteLogoComponent.h"
#include "UI/LevelMeterComponent.h"

class Brute102AudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::MidiKeyboardState::Listener,
                                     private juce::Timer
{
public:
    explicit Brute102AudioProcessorEditor(Brute102AudioProcessor&);
    ~Brute102AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct RotaryControl
    {
        juce::Slider slider;
        juce::Label titleLabel;
        juce::Label valueLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupRotary(RotaryControl& ctrl, const juce::String& paramId, const juce::String& labelText);
    void updateValueLabel(RotaryControl& ctrl, const juce::String& paramId);
    void syncDiscreteButtonsFromAPVTS();

    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void timerCallback() override;

    Brute102AudioProcessor& processorRef;
    brute102::BruteLookAndFeel lookAndFeel;

    // Header
    brute102::BruteLogoComponent brandLogo;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::ComboBox presetComboBox;

    // Module 1: Subtractive Oscs
    juce::TextButton subPowerButton { "ON" };
    juce::TextButton osc1SawButton { "SAW" };
    juce::TextButton osc1SqrButton { "SQR" };
    juce::TextButton subSqrButton  { "SQR" };
    juce::TextButton subSinButton  { "SIN" };
    RotaryControl osc2CoarseCtrl, osc2DetuneCtrl, osc2LevelCtrl, subLevelCtrl;

    // Module 2: 2-Op FM Layer
    juce::TextButton fmPowerButton { "ON" };
    std::array<juce::TextButton, 4> fmRatioButtons;
    RotaryControl fmIndexCtrl, fmDecayCtrl, fmMixCtrl;

    // Module 3: 24dB Ladder Filter
    RotaryControl cutoffCtrl, resonanceCtrl, envDepthCtrl;

    // Module 4: Envelopes
    RotaryControl filterDecayCtrl, ampDecayCtrl, ampReleaseCtrl;

    // Module 5: Grit & Drive
    juce::TextButton softClipButton { "SOFT TANH" };
    juce::TextButton hardClipButton { "HARD CLAMP" };
    RotaryControl driveCtrl;

    // Module 6: Voice & Output
    juce::TextButton retrigOnButton  { "ON" };
    juce::TextButton retrigOffButton { "OFF" };
    RotaryControl glideCtrl, masterCtrl;
    brute102::LevelMeterComponent levelMeter;

    // Standard JUCE Keyboard
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent midiKeyboard { keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };
    uint64_t prevDawLow = 0;
    uint64_t prevDawHigh = 0;
    int lastObservedProgram = 0;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> subPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fmPowerAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Brute102AudioProcessorEditor)
};
