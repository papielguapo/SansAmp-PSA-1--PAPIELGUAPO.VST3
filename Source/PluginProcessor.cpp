#include "PluginProcessor.h"
#include "PluginEditor.h"

SansAmpPSA1AudioProcessor::SansAmpPSA1AudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SansAmpPSA1Parameters", createParameterLayout())
{
    for (int i = 0; i < 8; ++i)
        paramPtrs[i] = apvts.getRawParameterValue (kParamIDs[i]);

    bypassParamPtr = apvts.getRawParameterValue ("bypass");
    bypassParam = apvts.getParameter ("bypass");

    coeffs.updateSampleRate (currentSampleRate);
    updateDSPCoefficients();
}

juce::AudioProcessorValueTreeState::ParameterLayout SansAmpPSA1AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < 8; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { kParamIDs[i], 1 },
            kParamNames[i],
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
            50.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return juce::String (v, 1); })));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 },
        "Master Bypass",
        false));

    return { params.begin(), params.end() };
}

void SansAmpPSA1AudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = static_cast<float> (sampleRate > 0.0 ? sampleRate : 44100.0);
    coeffs.updateSampleRate (currentSampleRate);
    updateDSPCoefficients();

    for (auto& ch : channels)
        ch.reset();

    setLatencySamples (coeffs.latencySamples);
}

void SansAmpPSA1AudioProcessor::releaseResources()
{
}

bool SansAmpPSA1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut && !(mainIn == juce::AudioChannelSet::mono() && mainOut == juce::AudioChannelSet::stereo()))
        return false;

    return true;
}

void SansAmpPSA1AudioProcessor::updateDSPCoefficients() noexcept
{
    coeffs.updateParams (
        paramPtrs[0]->load (std::memory_order_relaxed),
        paramPtrs[1]->load (std::memory_order_relaxed),
        paramPtrs[2]->load (std::memory_order_relaxed),
        paramPtrs[3]->load (std::memory_order_relaxed),
        paramPtrs[4]->load (std::memory_order_relaxed),
        paramPtrs[5]->load (std::memory_order_relaxed),
        paramPtrs[6]->load (std::memory_order_relaxed),
        paramPtrs[7]->load (std::memory_order_relaxed),
        currentSampleRate);
}

void SansAmpPSA1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    updateDSPCoefficients();
    const bool bypassed = bypassParamPtr != nullptr && bypassParamPtr->load (std::memory_order_relaxed) >= 0.5f;

    if (totalNumInputChannels == 1 && totalNumOutputChannels == 2)
    {
        const float* in = buffer.getReadPointer (0);
        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getWritePointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            const float y = channels[0].processSample (in[n], coeffs, bypassed);
            outL[n] = y;
            outR[n] = y;
        }
        return;
    }

    const int chCount = juce::jmin (totalNumInputChannels, 2);
    for (int ch = 0; ch < chCount; ++ch)
    {
        const float* in = buffer.getReadPointer (ch);
        float* out = buffer.getWritePointer (ch);
        auto& state = channels[ch];

        for (int n = 0; n < numSamples; ++n)
            out[n] = state.processSample (in[n], coeffs, bypassed);
    }
}

void SansAmpPSA1AudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    updateDSPCoefficients();

    const int chCount = juce::jmin (totalNumInputChannels, 2);
    for (int ch = 0; ch < chCount; ++ch)
    {
        const float* in = buffer.getReadPointer (ch);
        float* out = buffer.getWritePointer (ch);
        auto& state = channels[ch];

        for (int n = 0; n < numSamples; ++n)
            out[n] = state.processSample (in[n], coeffs, true);
    }
}

void SansAmpPSA1AudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= kNumFactoryPresets)
        return;

    currentProgram = index;
    const auto& preset = kFactoryPresets[index];

    for (int i = 0; i < 8; ++i)
    {
        if (auto* p = apvts.getParameter (kParamIDs[i]))
            p->setValueNotifyingHost (p->convertTo0to1 (preset.params[i]));
    }
}

const juce::String SansAmpPSA1AudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= kNumFactoryPresets)
        return {};

    const auto& preset = kFactoryPresets[index];
    return juce::String (preset.category) + " - " + juce::String (preset.name);
}

void SansAmpPSA1AudioProcessor::changeProgramName (int, const juce::String&)
{
}

void SansAmpPSA1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (std::unique_ptr<juce::XmlElement> xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SansAmpPSA1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (std::unique_ptr<juce::XmlElement> xmlState = getXmlFromBinary (data, sizeInBytes))
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xmlState);
            currentProgram = tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* SansAmpPSA1AudioProcessor::createEditor()
{
    return new SansAmpPSA1AudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SansAmpPSA1AudioProcessor();
}
