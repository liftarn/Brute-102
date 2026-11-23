#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"
#include <cmath>

namespace
{
    constexpr int kMargin = 12;
    constexpr int kRackW  = 760;
    constexpr int kRackH  = 360;
    constexpr int kHeaderH = 32;
    constexpr int kKeyH   = 96;
    constexpr int kGap    = 8;

    juce::String formatParamDisplay(const juce::String& id, float v)
    {
        if (id == brute102::ParamIDs::cutoffHz)
        {
            if (v >= 1000.0f)
                return juce::String(v / 1000.0f, v >= 10000.0f ? 1 : 2) + "k";
            return juce::String(juce::roundToInt(v)) + "Hz";
        }
        if (id == brute102::ParamIDs::glideMs || id == brute102::ParamIDs::fmDecayMs ||
            id == brute102::ParamIDs::filterDecayMs || id == brute102::ParamIDs::ampDecayMs ||
            id == brute102::ParamIDs::ampReleaseMs)
            return juce::String(juce::roundToInt(v)) + "ms";

        if (id == brute102::ParamIDs::osc2Coarse)
        {
            const int r = juce::roundToInt(v);
            return (r > 0 ? "+" : "") + juce::String(r) + "st";
        }
        if (id == brute102::ParamIDs::osc2Detune)
        {
            const int r = juce::roundToInt(v);
            return (r > 0 ? "+" : "") + juce::String(r) + "ct";
        }
        if (id == brute102::ParamIDs::osc2Level || id == brute102::ParamIDs::subLevel ||
            id == brute102::ParamIDs::fmMix || id == brute102::ParamIDs::envDepth ||
            id == brute102::ParamIDs::masterVol)
            return juce::String(juce::roundToInt(v * 100.0f)) + "%";

        if (id == brute102::ParamIDs::drive)
            return juce::String(v, 1) + "x";

        if (id == brute102::ParamIDs::resonance)
            return juce::String(v, 2);

        if (id == brute102::ParamIDs::fmIndex)
            return juce::String(v, 1);

        return juce::String(v, 2);
    }
}

Brute102AudioProcessorEditor::Brute102AudioProcessorEditor(Brute102AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(kRackW + kMargin * 2, kMargin + kHeaderH + kGap + kRackH + kGap + kKeyH + kMargin);

    // Header
    addAndMakeVisible(brandLogo);

    for (size_t i = 0; i < brute102::kPresets.size(); ++i)
    {
        const auto num = juce::String(static_cast<int>(i + 1)).paddedLeft('0', 2);
        presetComboBox.addItem(num + "  " + juce::String(juce::CharPointer_UTF8(brute102::kPresets[i].name)),
                               static_cast<int>(i + 1));
    }

    lastObservedProgram = processorRef.getCurrentProgram();
    presetComboBox.setSelectedId(lastObservedProgram + 1, juce::dontSendNotification);
    presetComboBox.onChange = [this]
    {
        const int idx = presetComboBox.getSelectedId() - 1;
        if (idx >= 0)
        {
            lastObservedProgram = idx;
            processorRef.setCurrentProgram(idx);
            syncDiscreteButtonsFromAPVTS();
            repaint();
        }
    };
    addAndMakeVisible(presetComboBox);

    prevPresetButton.onClick = [this]
    {
        const int count = static_cast<int>(brute102::kPresets.size());
        const int next = (presetComboBox.getSelectedId() - 1 - 1 + count) % count;
        presetComboBox.setSelectedId(next + 1, juce::sendNotification);
    };
    nextPresetButton.onClick = [this]
    {
        const int count = static_cast<int>(brute102::kPresets.size());
        const int next = (presetComboBox.getSelectedId() - 1 + 1) % count;
        presetComboBox.setSelectedId(next + 1, juce::sendNotification);
    };
    addAndMakeVisible(prevPresetButton);
    addAndMakeVisible(nextPresetButton);

    auto& apvts = processorRef.getAPVTS();

    // Power switches
    subPowerButton.setClickingTogglesState(true);
    subPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, brute102::ParamIDs::subtractiveEnabled, subPowerButton);
    addAndMakeVisible(subPowerButton);

    fmPowerButton.setClickingTogglesState(true);
    fmPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, brute102::ParamIDs::fmEnabled, fmPowerButton);
    addAndMakeVisible(fmPowerButton);

    // Choice button groups
    auto setChoiceParam = [&apvts](const char* id, int choiceIdx)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(choiceIdx)));
    };

    osc1SawButton.onClick = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::osc1Wave, 0); syncDiscreteButtonsFromAPVTS(); };
    osc1SqrButton.onClick = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::osc1Wave, 1); syncDiscreteButtonsFromAPVTS(); };
    subSqrButton.onClick  = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::subWave, 0);  syncDiscreteButtonsFromAPVTS(); };
    subSinButton.onClick  = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::subWave, 1);  syncDiscreteButtonsFromAPVTS(); };

    for (auto* b : { &osc1SawButton, &osc1SqrButton, &subSqrButton, &subSinButton })
        addAndMakeVisible(*b);

    const char* ratioLabels[4] = { "0.5", "1.0", "2.0", "3.0" };
    for (int i = 0; i < 4; ++i)
    {
        fmRatioButtons[static_cast<size_t>(i)].setButtonText(ratioLabels[i]);
        fmRatioButtons[static_cast<size_t>(i)].onClick = [this, setChoiceParam, i]
        {
            setChoiceParam(brute102::ParamIDs::fmRatio, i);
            syncDiscreteButtonsFromAPVTS();
        };
        addAndMakeVisible(fmRatioButtons[static_cast<size_t>(i)]);
    }

    softClipButton.onClick = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::shaperMode, 0); syncDiscreteButtonsFromAPVTS(); repaint(); };
    hardClipButton.onClick = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::shaperMode, 1); syncDiscreteButtonsFromAPVTS(); repaint(); };
    addAndMakeVisible(softClipButton);
    addAndMakeVisible(hardClipButton);

    retrigOnButton.onClick  = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::retrigger, 1); syncDiscreteButtonsFromAPVTS(); };
    retrigOffButton.onClick = [this, setChoiceParam] { setChoiceParam(brute102::ParamIDs::retrigger, 0); syncDiscreteButtonsFromAPVTS(); };
    addAndMakeVisible(retrigOnButton);
    addAndMakeVisible(retrigOffButton);

    // Rotary controls
    setupRotary(osc2CoarseCtrl,  brute102::ParamIDs::osc2Coarse,    "O2 COARSE");
    setupRotary(osc2DetuneCtrl,  brute102::ParamIDs::osc2Detune,    "DETUNE");
    setupRotary(osc2LevelCtrl,   brute102::ParamIDs::osc2Level,     "OSC2 LVL");
    setupRotary(subLevelCtrl,    brute102::ParamIDs::subLevel,      "SUB -12");

    setupRotary(fmIndexCtrl,     brute102::ParamIDs::fmIndex,       "MOD INDEX");
    setupRotary(fmDecayCtrl,     brute102::ParamIDs::fmDecayMs,     "FM DECAY");
    setupRotary(fmMixCtrl,       brute102::ParamIDs::fmMix,         "FM MIX");

    setupRotary(cutoffCtrl,      brute102::ParamIDs::cutoffHz,      "CUTOFF");
    setupRotary(resonanceCtrl,   brute102::ParamIDs::resonance,     "RESONANCE");
    setupRotary(envDepthCtrl,    brute102::ParamIDs::envDepth,      "ENV DEPTH");

    setupRotary(filterDecayCtrl, brute102::ParamIDs::filterDecayMs, "FILT DECAY");
    setupRotary(ampDecayCtrl,    brute102::ParamIDs::ampDecayMs,    "AMP DECAY");
    setupRotary(ampReleaseCtrl,  brute102::ParamIDs::ampReleaseMs,  "AMP REL");

    setupRotary(driveCtrl,       brute102::ParamIDs::drive,         "DRIVE");
    setupRotary(glideCtrl,       brute102::ParamIDs::glideMs,       "GLIDE");
    setupRotary(masterCtrl,      brute102::ParamIDs::masterVol,     "MASTER");

    addAndMakeVisible(levelMeter);

    // Standard JUCE MidiKeyboardComponent
    keyboardState.addListener(this);
    midiKeyboard.setAvailableRange(24, 96);
    midiKeyboard.setLowestVisibleKey(24);
    midiKeyboard.setKeyWidth(32.9f);
    midiKeyboard.setScrollButtonsVisible(true);
    addAndMakeVisible(midiKeyboard);

    syncDiscreteButtonsFromAPVTS();
    startTimerHz(30);
}

Brute102AudioProcessorEditor::~Brute102AudioProcessorEditor()
{
    stopTimer();
    keyboardState.removeListener(this);
    processorRef.pushUiAllNotesOff();
    setLookAndFeel(nullptr);
}

void Brute102AudioProcessorEditor::setupRotary(RotaryControl& ctrl, const juce::String& paramId, const juce::String& labelText)
{
    ctrl.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ctrl.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    ctrl.slider.onValueChange = [this, &ctrl, paramId]
    {
        updateValueLabel(ctrl, paramId);
        if (paramId == brute102::ParamIDs::cutoffHz || paramId == brute102::ParamIDs::resonance ||
            paramId == brute102::ParamIDs::drive)
            repaint();
    };
    addAndMakeVisible(ctrl.slider);

    ctrl.titleLabel.setText(labelText, juce::dontSendNotification);
    ctrl.titleLabel.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    ctrl.titleLabel.setColour(juce::Label::textColourId, brute102::Colors::textMuted);
    ctrl.titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(ctrl.titleLabel);

    ctrl.valueLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    ctrl.valueLabel.setColour(juce::Label::textColourId, brute102::Colors::amber);
    ctrl.valueLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(ctrl.valueLabel);

    ctrl.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), paramId, ctrl.slider);
    updateValueLabel(ctrl, paramId);
}

void Brute102AudioProcessorEditor::updateValueLabel(RotaryControl& ctrl, const juce::String& paramId)
{
    ctrl.valueLabel.setText(formatParamDisplay(paramId, static_cast<float>(ctrl.slider.getValue())),
                            juce::dontSendNotification);
}

void Brute102AudioProcessorEditor::syncDiscreteButtonsFromAPVTS()
{
    auto& apvts = processorRef.getAPVTS();
    const bool subOn = apvts.getRawParameterValue(brute102::ParamIDs::subtractiveEnabled)->load() >= 0.5f;
    const bool fmOn  = apvts.getRawParameterValue(brute102::ParamIDs::fmEnabled)->load() >= 0.5f;
    subPowerButton.setButtonText(subOn ? "ON" : "OFF");
    fmPowerButton.setButtonText(fmOn ? "ON" : "OFF");

    const int osc1 = juce::roundToInt(apvts.getRawParameterValue(brute102::ParamIDs::osc1Wave)->load());
    osc1SawButton.setToggleState(osc1 == 0, juce::dontSendNotification);
    osc1SqrButton.setToggleState(osc1 == 1, juce::dontSendNotification);

    const int subW = juce::roundToInt(apvts.getRawParameterValue(brute102::ParamIDs::subWave)->load());
    subSqrButton.setToggleState(subW == 0, juce::dontSendNotification);
    subSinButton.setToggleState(subW == 1, juce::dontSendNotification);

    const int ratioIdx = juce::roundToInt(apvts.getRawParameterValue(brute102::ParamIDs::fmRatio)->load());
    for (int i = 0; i < 4; ++i)
        fmRatioButtons[static_cast<size_t>(i)].setToggleState(i == ratioIdx, juce::dontSendNotification);

    const int shaper = juce::roundToInt(apvts.getRawParameterValue(brute102::ParamIDs::shaperMode)->load());
    softClipButton.setToggleState(shaper == 0, juce::dontSendNotification);
    hardClipButton.setToggleState(shaper == 1, juce::dontSendNotification);

    const bool retrig = apvts.getRawParameterValue(brute102::ParamIDs::retrigger)->load() >= 0.5f;
    retrigOnButton.setToggleState(retrig, juce::dontSendNotification);
    retrigOffButton.setToggleState(!retrig, juce::dontSendNotification);
}

void Brute102AudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float)
{
    if (midiChannel == 1)
        processorRef.pushUiMidiEvent(midiNoteNumber, true);
}

void Brute102AudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float)
{
    if (midiChannel == 1)
        processorRef.pushUiMidiEvent(midiNoteNumber, false);
}

void Brute102AudioProcessorEditor::timerCallback()
{
    levelMeter.updatePeak(processorRef.getOutputPeak());
    syncDiscreteButtonsFromAPVTS();

    const int currentProg = processorRef.getCurrentProgram();
    if (currentProg != lastObservedProgram && !presetComboBox.isPopupActive())
    {
        lastObservedProgram = currentProg;
        presetComboBox.setSelectedId(currentProg + 1, juce::dontSendNotification);
        repaint();
    }

    // Mirror DAW MIDI note activity on channel 2 without triggering UI->Audio feedback
    const uint64_t low  = processorRef.getDawNotesLow();
    const uint64_t high = processorRef.getDawNotesHigh();

    if (low != prevDawLow)
    {
        for (int n = 0; n < 64; ++n)
        {
            const uint64_t bit = uint64_t(1) << n;
            const bool nowOn  = (low & bit) != 0;
            const bool wasOn  = (prevDawLow & bit) != 0;
            if (nowOn != wasOn)
            {
                if (nowOn) keyboardState.noteOn(2, n, 1.0f);
                else       keyboardState.noteOff(2, n, 0.0f);
            }
        }
        prevDawLow = low;
    }

    if (high != prevDawHigh)
    {
        for (int n = 0; n < 64; ++n)
        {
            const uint64_t bit = uint64_t(1) << n;
            const bool nowOn  = (high & bit) != 0;
            const bool wasOn  = (prevDawHigh & bit) != 0;
            if (nowOn != wasOn)
            {
                if (nowOn) keyboardState.noteOn(2, n + 64, 1.0f);
                else       keyboardState.noteOff(2, n + 64, 0.0f);
            }
        }
        prevDawHigh = high;
    }
}

void Brute102AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(brute102::Colors::background);

    const auto rackBounds = juce::Rectangle<int>(kMargin, kMargin + kHeaderH + kGap, kRackW, kRackH);
    g.setColour(brute102::Colors::chassis);
    g.fillRoundedRectangle(rackBounds.toFloat(), 6.0f);
    g.setColour(brute102::Colors::borderStrong);
    g.drawRoundedRectangle(rackBounds.toFloat(), 6.0f, 1.0f);

    const int modW = kRackW / 3;
    const int modH = kRackH / 2;

    // 3x2 grid dividers
    g.setColour(brute102::Colors::border);
    g.drawVerticalLine(rackBounds.getX() + modW, static_cast<float>(rackBounds.getY()), static_cast<float>(rackBounds.getBottom()));
    g.drawVerticalLine(rackBounds.getX() + modW * 2, static_cast<float>(rackBounds.getY()), static_cast<float>(rackBounds.getBottom()));
    g.drawHorizontalLine(rackBounds.getY() + modH, static_cast<float>(rackBounds.getX()), static_cast<float>(rackBounds.getRight()));

    const char* titles[6] = {
        "SUBTRACTIVE OSCS", "2-OP FM LAYER", "24DB LADDER FILTER",
        "ENVELOPES", "GRIT & DRIVE", "VOICE & OUTPUT"
    };

    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            const auto cell = juce::Rectangle<int>(rackBounds.getX() + col * modW,
                                                     rackBounds.getY() + row * modH,
                                                     modW, modH).reduced(12, 8);
            g.setColour(brute102::Colors::textPrimary);
            g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
            g.drawText(titles[row * 3 + col], cell.getX(), cell.getY(), cell.getWidth() - 40, 18,
                       juce::Justification::centredLeft);

            g.setColour(brute102::Colors::border);
            g.drawHorizontalLine(cell.getY() + 21, static_cast<float>(cell.getX()), static_cast<float>(cell.getRight()));
        }
    }

    // Silkscreen switch labels (OSC1, SUB, RETRIG)
    g.setColour(brute102::Colors::textMuted);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    {
        const auto m1 = juce::Rectangle<int>(rackBounds.getX(), rackBounds.getY(), modW, modH).reduced(12, 8);
        g.drawText("OSC1", m1.getX(), m1.getY() + 29, 32, 18, juce::Justification::centredLeft);
        g.drawText("SUB",  m1.getX() + 114, m1.getY() + 29, 26, 18, juce::Justification::centredLeft);

        const auto m6 = juce::Rectangle<int>(rackBounds.getX() + modW * 2, rackBounds.getY() + modH, modW, modH).reduced(12, 8);
        g.drawText("RETRIG", m6.getX(), m6.getY() + 28, 52, 18, juce::Justification::centredLeft);
    }

    // Filter frequency response mini preview (Module 3)
    {
        const auto filtCell = juce::Rectangle<int>(rackBounds.getX() + modW * 2, rackBounds.getY(), modW, modH).reduced(12, 8);
        const auto graph = juce::Rectangle<float>(static_cast<float>(filtCell.getX()),
                                                  static_cast<float>(filtCell.getY() + 28),
                                                  static_cast<float>(filtCell.getWidth()), 32.0f);
        g.setColour(brute102::Colors::recess);
        g.fillRoundedRectangle(graph, 3.0f);
        g.setColour(brute102::Colors::border);
        g.drawRoundedRectangle(graph, 3.0f, 1.0f);

        const float cutHz = processorRef.getAPVTS().getRawParameterValue(brute102::ParamIDs::cutoffHz)->load();
        const float reso  = processorRef.getAPVTS().getRawParameterValue(brute102::ParamIDs::resonance)->load();
        const float normCut = std::log(cutHz / 20.0f) / std::log(18000.0f / 20.0f);
        const float cutX = graph.getX() + juce::jlimit(12.0f, graph.getWidth() - 14.0f, normCut * (graph.getWidth() - 20.0f) + 10.0f);
        const float baseY = graph.getY() + graph.getHeight() * 0.5f;
        const float peakY = graph.getY() + std::max(3.0f, 14.0f - reso * 12.0f);

        juce::Path p;
        p.startNewSubPath(graph.getX() + 4.0f, baseY);
        p.lineTo(std::max(graph.getX() + 4.0f, cutX - 18.0f), baseY);
        p.quadraticTo(cutX - 4.0f, peakY, cutX, peakY + 2.0f);
        p.lineTo(std::min(graph.getRight() - 4.0f, cutX + 26.0f), graph.getBottom() - 2.0f);
        p.lineTo(graph.getRight() - 4.0f, graph.getBottom() - 2.0f);

        g.setColour(brute102::Colors::amber);
        g.strokePath(p, juce::PathStrokeType(1.75f));
    }

    // Grit & Drive waveshaper transfer curve preview (Module 5)
    {
        const auto gritCell = juce::Rectangle<int>(rackBounds.getX() + modW, rackBounds.getY() + modH, modW, modH).reduced(12, 8);
        const auto graph = juce::Rectangle<float>(static_cast<float>(gritCell.getRight() - 94),
                                                  static_cast<float>(gritCell.getBottom() - 68),
                                                  90.0f, 60.0f);
        g.setColour(brute102::Colors::recess);
        g.fillRoundedRectangle(graph, 3.0f);
        g.setColour(brute102::Colors::border);
        g.drawRoundedRectangle(graph, 3.0f, 1.0f);

        g.drawHorizontalLine(juce::roundToInt(graph.getCentreY()), graph.getX(), graph.getRight());
        g.drawVerticalLine(juce::roundToInt(graph.getCentreX()), graph.getY(), graph.getBottom());

        const float drv = processorRef.getAPVTS().getRawParameterValue(brute102::ParamIDs::drive)->load();
        const int mode  = juce::roundToInt(processorRef.getAPVTS().getRawParameterValue(brute102::ParamIDs::shaperMode)->load());

        juce::Path p;
        constexpr int samples = 36;
        for (int i = 0; i <= samples; ++i)
        {
            const float nx = (static_cast<float>(i) / samples) * 2.0f - 1.0f;
            const float driven = nx * drv;
            const float yVal = (mode == 0) ? std::tanh(driven) : juce::jlimit(-1.0f, 1.0f, driven);
            const float px = graph.getX() + 4.0f + (static_cast<float>(i) / samples) * (graph.getWidth() - 8.0f);
            const float py = graph.getCentreY() - yVal * (graph.getHeight() * 0.40f);
            if (i == 0) p.startNewSubPath(px, py);
            else        p.lineTo(px, py);
        }

        g.setColour(brute102::Colors::amber);
        g.strokePath(p, juce::PathStrokeType(1.75f));
    }
}

void Brute102AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(kMargin);

    // Header: Brute-102 logo badge on upper-left, Presets on upper-right
    auto header = area.removeFromTop(kHeaderH);
    brandLogo.setBounds(header.removeFromLeft(168));

    auto presetArea = header.removeFromRight(290);
    prevPresetButton.setBounds(presetArea.removeFromLeft(26).reduced(0, 2));
    nextPresetButton.setBounds(presetArea.removeFromRight(26).reduced(0, 2));
    presetComboBox.setBounds(presetArea.reduced(4, 2));

    area.removeFromTop(kGap);

    // 760x360 6-Module Rack
    const auto rack = area.removeFromTop(kRackH);
    const int modW = rack.getWidth() / 3;
    const int modH = rack.getHeight() / 2;

    auto layoutRotary = [](RotaryControl& c, juce::Rectangle<int> slot)
    {
        c.valueLabel.setBounds(slot.removeFromBottom(12));
        c.titleLabel.setBounds(slot.removeFromBottom(11));
        c.slider.setBounds(slot.reduced(2));
    };

    // Module 1: Subtractive Oscs
    {
        auto cell = rack.withWidth(modW).withHeight(modH).reduced(12, 8);
        subPowerButton.setBounds(cell.getRight() - 34, cell.getY(), 34, 17);
        cell.removeFromTop(27);

        auto toggleRow = cell.removeFromTop(20);
        toggleRow.removeFromLeft(30); // OSC1 label space
        auto leftGroup = toggleRow.removeFromLeft(78);
        osc1SawButton.setBounds(leftGroup.removeFromLeft(38));
        osc1SqrButton.setBounds(leftGroup.removeFromRight(38));

        auto rightGroup = toggleRow.removeFromRight(78);
        subSqrButton.setBounds(rightGroup.removeFromLeft(38));
        subSinButton.setBounds(rightGroup.removeFromRight(38));

        cell.removeFromTop(6);
        const int kw = cell.getWidth() / 4;
        layoutRotary(osc2CoarseCtrl, cell.removeFromLeft(kw));
        layoutRotary(osc2DetuneCtrl, cell.removeFromLeft(kw));
        layoutRotary(osc2LevelCtrl,  cell.removeFromLeft(kw));
        layoutRotary(subLevelCtrl,   cell);
    }

    // Module 2: 2-Op FM Layer
    {
        auto cell = rack.withX(rack.getX() + modW).withWidth(modW).withHeight(modH).reduced(12, 8);
        fmPowerButton.setBounds(cell.getRight() - 34, cell.getY(), 34, 17);
        cell.removeFromTop(27);

        auto ratioRow = cell.removeFromTop(20);
        const int rw = ratioRow.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
            fmRatioButtons[static_cast<size_t>(i)].setBounds(ratioRow.removeFromLeft(rw).reduced(1, 0));

        cell.removeFromTop(6);
        const int kw = cell.getWidth() / 3;
        layoutRotary(fmIndexCtrl, cell.removeFromLeft(kw));
        layoutRotary(fmDecayCtrl, cell.removeFromLeft(kw));
        layoutRotary(fmMixCtrl,   cell);
    }

    // Module 3: 24dB Ladder Filter
    {
        auto cell = rack.withX(rack.getX() + modW * 2).withWidth(modW).withHeight(modH).reduced(12, 8);
        cell.removeFromTop(64);
        const int kw = cell.getWidth() / 3;
        layoutRotary(cutoffCtrl,    cell.removeFromLeft(kw));
        layoutRotary(resonanceCtrl, cell.removeFromLeft(kw));
        layoutRotary(envDepthCtrl,  cell);
    }

    // Module 4: Envelopes
    {
        auto cell = rack.withY(rack.getY() + modH).withWidth(modW).withHeight(modH).reduced(12, 8);
        cell.removeFromTop(42);
        const int kw = cell.getWidth() / 3;
        layoutRotary(filterDecayCtrl, cell.removeFromLeft(kw));
        layoutRotary(ampDecayCtrl,    cell.removeFromLeft(kw));
        layoutRotary(ampReleaseCtrl,  cell);
    }

    // Module 5: Grit & Drive
    {
        auto cell = rack.withX(rack.getX() + modW).withY(rack.getY() + modH).withWidth(modW).withHeight(modH).reduced(12, 8);
        cell.removeFromTop(27);
        auto clipRow = cell.removeFromTop(20);
        const int halfW = (clipRow.getWidth() - 4) / 2;
        softClipButton.setBounds(clipRow.removeFromLeft(halfW));
        hardClipButton.setBounds(clipRow.removeFromRight(halfW));

        cell.removeFromTop(8);
        layoutRotary(driveCtrl, cell.removeFromLeft(110));
    }

    // Module 6: Voice & Output
    {
        auto cell = rack.withX(rack.getX() + modW * 2).withY(rack.getY() + modH).withWidth(modW).withHeight(modH).reduced(12, 8);
        cell.removeFromTop(27);

        auto meterArea = cell.removeFromRight(36);
        levelMeter.setBounds(meterArea);
        cell.removeFromRight(8);

        auto retrigRow = cell.removeFromTop(20);
        auto retrigBtns = retrigRow.removeFromRight(88);
        retrigOnButton.setBounds(retrigBtns.removeFromLeft(42));
        retrigOffButton.setBounds(retrigBtns.removeFromRight(42));

        cell.removeFromTop(8);
        const int kw = cell.getWidth() / 2;
        layoutRotary(glideCtrl,  cell.removeFromLeft(kw));
        layoutRotary(masterCtrl, cell);
    }

    area.removeFromTop(kGap);
    midiKeyboard.setBounds(area.removeFromTop(kKeyH));
}
