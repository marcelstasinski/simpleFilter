/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EasyFilterAudioProcessor::EasyFilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

EasyFilterAudioProcessor::~EasyFilterAudioProcessor()
{
}

//==============================================================================
const juce::String EasyFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EasyFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EasyFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EasyFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EasyFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EasyFilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EasyFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EasyFilterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EasyFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void EasyFilterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EasyFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

	juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = 1;
	spec.sampleRate = sampleRate;

	leftChain.prepare(spec);
	rightChain.prepare(spec);

	updateFilters();

}

void EasyFilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EasyFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EasyFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	updateFilters();

	juce::dsp::AudioBlock<float> block(buffer);

	auto leftBlock = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
	juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

	leftChain.process(leftContext);
	rightChain.process(rightContext);

}

//==============================================================================
bool EasyFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EasyFilterAudioProcessor::createEditor()
{
	//return new juce::GenericAudioProcessorEditor(*this);
    return new EasyFilterAudioProcessorEditor (*this);
}

//==============================================================================
void EasyFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void EasyFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
	ChainSettings settings;

	settings.lowCutFreq = apvts.getRawParameterValue("LowCutFreq")->load();
	settings.highCutFreq = apvts.getRawParameterValue("HighCutFreq")->load();
	settings.lowCutSlope = static_cast<Slope>(static_cast<int>(apvts.getRawParameterValue("LowCutSlope")->load()));
	settings.highCutSlope = static_cast<Slope>(static_cast<int>(apvts.getRawParameterValue("HighCutSlope")->load()));

	return settings;
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
	*old = *replacements;
}

void EasyFilterAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
	auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
	
	auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
	auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

	updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
	updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void EasyFilterAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
	auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

	auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
	auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

	updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
	updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void EasyFilterAudioProcessor::updateFilters()
{
	auto chainSettings = getChainSettings(apvts);

	updateLowCutFilters(chainSettings);
	updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout EasyFilterAudioProcessor::createParameterLayout()
{
	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	layout.add(std::make_unique<juce::AudioParameterFloat>("LowCutFreq", "LowCutFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("HighCutFreq", "HighCutFreq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));

	juce::StringArray stringArray;
	for (int i = 0; i < 4; ++i)
	{
		juce::String str;
		str << (12 + i * 12);
		str << " db/Oct";
		stringArray.add(str);
	}

	layout.add(std::make_unique<juce::AudioParameterChoice>("LowCutSlope", "LowCutSlope", stringArray, 0));
	layout.add(std::make_unique<juce::AudioParameterChoice>("HighCutSlope", "HighCutSlope", stringArray, 0));

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EasyFilterAudioProcessor();
}