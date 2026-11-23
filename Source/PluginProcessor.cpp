#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

Brute102AudioProcessor::Brute102AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", brute102::createParameterLayout())
{
    retriggerParam          = apvts.getRawParameterValue(brute102::ParamIDs::retrigger);
    glideMsParam            = apvts.getRawParameterValue(brute102::ParamIDs::glideMs);
    subtractiveEnabledParam = apvts.getRawParameterValue(brute102::ParamIDs::subtractiveEnabled);
    osc1WaveParam           = apvts.getRawParameterValue(brute102::ParamIDs::osc1Wave);
    osc2CoarseParam         = apvts.getRawParameterValue(brute102::ParamIDs::osc2Coarse);
    osc2DetuneParam         = apvts.getRawParameterValue(brute102::ParamIDs::osc2Detune);
    osc2LevelParam          = apvts.getRawParameterValue(brute102::ParamIDs::osc2Level);
    subWaveParam            = apvts.getRawParameterValue(brute102::ParamIDs::subWave);
    subLevelParam           = apvts.getRawParameterValue(brute102::ParamIDs::subLevel);
    fmEnabledParam          = apvts.getRawParameterValue(brute102::ParamIDs::fmEnabled);
    fmRatioParam            = apvts.getRawParameterValue(brute102::ParamIDs::fmRatio);
    fmIndexParam            = apvts.getRawParameterValue(brute102::ParamIDs::fmIndex);
    fmDecayMsParam          = apvts.getRawParameterValue(brute102::ParamIDs::fmDecayMs);
    fmMixParam              = apvts.getRawParameterValue(brute102::ParamIDs::fmMix);
    cutoffHzParam           = apvts.getRawParameterValue(brute102::ParamIDs::cutoffHz);
    resonanceParam          = apvts.getRawParameterValue(brute102::ParamIDs::resonance);
    envDepthParam           = apvts.getRawParameterValue(brute102::ParamIDs::envDepth);
    filterDecayMsParam      = apvts.getRawParameterValue(brute102::ParamIDs::filterDecayMs);
    ampDecayMsParam         = apvts.getRawParameterValue(brute102::ParamIDs::ampDecayMs);
    ampReleaseMsParam       = apvts.getRawParameterValue(brute102::ParamIDs::ampReleaseMs);
    driveParam              = apvts.getRawParameterValue(brute102::ParamIDs::drive);
    shaperModeParam         = apvts.getRawParameterValue(brute102::ParamIDs::shaperMode);
    masterVolParam          = apvts.getRawParameterValue(brute102::ParamIDs::masterVol);
}

void Brute102AudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    engine.prepare(sampleRate);
    dawNotesLow.store(0, std::memory_order_relaxed);
    dawNotesHigh.store(0, std::memory_order_relaxed);
    outputPeak.store(0.0f, std::memory_order_relaxed);
}

void Brute102AudioProcessor::releaseResources()
{
    engine.allNotesOff();
}

bool Brute102AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto outSet = layouts.getMainOutputChannelSet();
    return outSet == juce::AudioChannelSet::mono()
        || outSet == juce::AudioChannelSet::stereo();
}

void Brute102AudioProcessor::pushUiMidiEvent(int note, bool isNoteOn) noexcept
{
    const auto writer = uiMidiFifo.write(1);
    if (writer.blockSize1 > 0)
        uiMidiBuffer[static_cast<size_t>(writer.startIndex1)] = { note, isNoteOn };
}

void Brute102AudioProcessor::pushUiAllNotesOff() noexcept
{
    pendingAllNotesOff.store(true, std::memory_order_release);
}

void Brute102AudioProcessor::setDawNoteBit(int note, bool isOn) noexcept
{
    if (note < 0 || note > 127)
        return;

    if (note < 64)
    {
        uint64_t mask = dawNotesLow.load(std::memory_order_relaxed);
        const uint64_t bit = uint64_t(1) << note;
        mask = isOn ? (mask | bit) : (mask & ~bit);
        dawNotesLow.store(mask, std::memory_order_relaxed);
    }
    else
    {
        uint64_t mask = dawNotesHigh.load(std::memory_order_relaxed);
        const uint64_t bit = uint64_t(1) << (note - 64);
        mask = isOn ? (mask | bit) : (mask & ~bit);
        dawNotesHigh.store(mask, std::memory_order_relaxed);
    }
}

void Brute102AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    // Update DSP parameter targets (lock-free atomic loads)
    brute102::EngineParams p;
    p.retrigger          = retriggerParam->load(std::memory_order_relaxed) >= 0.5f;
    p.glideMs            = glideMsParam->load(std::memory_order_relaxed);
    p.subtractiveEnabled = subtractiveEnabledParam->load(std::memory_order_relaxed) >= 0.5f;
    p.osc1Wave           = juce::roundToInt(osc1WaveParam->load(std::memory_order_relaxed));
    p.osc2Coarse         = osc2CoarseParam->load(std::memory_order_relaxed);
    p.osc2Detune         = osc2DetuneParam->load(std::memory_order_relaxed);
    p.osc2Level          = osc2LevelParam->load(std::memory_order_relaxed);
    p.subWave            = juce::roundToInt(subWaveParam->load(std::memory_order_relaxed));
    p.subLevel           = subLevelParam->load(std::memory_order_relaxed);
    p.fmEnabled          = fmEnabledParam->load(std::memory_order_relaxed) >= 0.5f;
    p.fmRatio            = brute102::fmRatioFromChoice(juce::roundToInt(fmRatioParam->load(std::memory_order_relaxed)));
    p.fmIndex            = fmIndexParam->load(std::memory_order_relaxed);
    p.fmDecayMs          = fmDecayMsParam->load(std::memory_order_relaxed);
    p.fmMix              = fmMixParam->load(std::memory_order_relaxed);
    p.cutoffHz           = cutoffHzParam->load(std::memory_order_relaxed);
    p.resonance          = resonanceParam->load(std::memory_order_relaxed);
    p.envDepth           = envDepthParam->load(std::memory_order_relaxed);
    p.filterDecayMs      = filterDecayMsParam->load(std::memory_order_relaxed);
    p.ampDecayMs         = ampDecayMsParam->load(std::memory_order_relaxed);
    p.ampReleaseMs       = ampReleaseMsParam->load(std::memory_order_relaxed);
    p.drive              = driveParam->load(std::memory_order_relaxed);
    p.shaperMode         = juce::roundToInt(shaperModeParam->load(std::memory_order_relaxed));
    p.masterVol          = masterVolParam->load(std::memory_order_relaxed);
    engine.setParameters(p);

    // Drain lock-free UI keyboard events
    if (pendingAllNotesOff.exchange(false, std::memory_order_acquire))
        engine.allNotesOff();

    const int numReady = uiMidiFifo.getNumReady();
    if (numReady > 0)
    {
        const auto reader = uiMidiFifo.read(numReady);
        for (int i = 0; i < reader.blockSize1; ++i)
        {
            const auto& ev = uiMidiBuffer[static_cast<size_t>(reader.startIndex1 + i)];
            if (ev.isNoteOn) engine.noteOn(ev.note);
            else             engine.noteOff(ev.note);
        }
        for (int i = 0; i < reader.blockSize2; ++i)
        {
            const auto& ev = uiMidiBuffer[static_cast<size_t>(reader.startIndex2 + i)];
            if (ev.isNoteOn) engine.noteOn(ev.note);
            else             engine.noteOff(ev.note);
        }
    }

    // Sample-accurate DAW MIDI traversal
    auto midiIt = midiMessages.cbegin();
    const auto midiEnd = midiMessages.cend();

    float* outL = buffer.getWritePointer(0);
    float* outR = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
    float blockPeak = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        while (midiIt != midiEnd && (*midiIt).samplePosition <= sample)
        {
            const auto msg = (*midiIt).getMessage();
            if (msg.isNoteOn())
            {
                engine.noteOn(msg.getNoteNumber());
                setDawNoteBit(msg.getNoteNumber(), true);
            }
            else if (msg.isNoteOff())
            {
                engine.noteOff(msg.getNoteNumber());
                setDawNoteBit(msg.getNoteNumber(), false);
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                engine.allNotesOff();
                dawNotesLow.store(0, std::memory_order_relaxed);
                dawNotesHigh.store(0, std::memory_order_relaxed);
            }
            ++midiIt;
        }

        const float y = engine.processSample();
        outL[sample] = y;
        if (outR != nullptr)
            outR[sample] = y;

        const float absY = std::abs(y);
        if (absY > blockPeak)
            blockPeak = absY;
    }

    // Process any trailing MIDI events past last sample
    while (midiIt != midiEnd)
    {
        const auto msg = (*midiIt).getMessage();
        if (msg.isNoteOn())
        {
            engine.noteOn(msg.getNoteNumber());
            setDawNoteBit(msg.getNoteNumber(), true);
        }
        else if (msg.isNoteOff())
        {
            engine.noteOff(msg.getNoteNumber());
            setDawNoteBit(msg.getNoteNumber(), false);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            engine.allNotesOff();
            dawNotesLow.store(0, std::memory_order_relaxed);
            dawNotesHigh.store(0, std::memory_order_relaxed);
        }
        ++midiIt;
    }

    for (int ch = 2; ch < numChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    outputPeak.store(blockPeak, std::memory_order_relaxed);
}

int Brute102AudioProcessor::getNumPrograms()
{
    return static_cast<int>(brute102::kPresets.size());
}

int Brute102AudioProcessor::getCurrentProgram()
{
    return currentProgramIndex.load(std::memory_order_relaxed);
}

void Brute102AudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    currentProgramIndex.store(index, std::memory_order_relaxed);
    brute102::applyPresetToAPVTS(apvts, index);
}

const juce::String Brute102AudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < getNumPrograms())
        return brute102::kPresets[static_cast<size_t>(index)].name;
    return {};
}

void Brute102AudioProcessor::changeProgramName(int, const juce::String&) {}

void Brute102AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (std::unique_ptr<juce::XmlElement> xml(state.createXml()); xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void Brute102AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor* Brute102AudioProcessor::createEditor()
{
    return new Brute102AudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Brute102AudioProcessor();
}
