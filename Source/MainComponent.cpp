#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : keyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (keyboardComponent);
    keyboardComponent.setAvailableRange (36, 96);      // C2 -> C7
    keyboardComponent.setLowestVisibleKey (48);        // start around C3

    setSize (820, 260);

    // 0 audio inputs (avoids the microphone permission prompt), 2 outputs.
    setAudioChannels (0, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    maxBlockSize      = samplesPerBlockExpected;

    rnboObject.prepareToProcess (sampleRate, (size_t) samplesPerBlockExpected);

    numRnboOutputs = juce::jmax (1, (int) rnboObject.getNumOutputChannels());

    scratchData.assign ((size_t) numRnboOutputs * (size_t) samplesPerBlockExpected, RNBO::SampleValue (0));
    scratchPtrs.resize ((size_t) numRnboOutputs, nullptr);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    auto* buffer        = bufferToFill.buffer;
    const int numSamples = bufferToFill.numSamples;
    jassert (numSamples <= maxBlockSize);

    //------------------------------------------------------------------
    // 1. Collect MIDI from the on-screen keyboard and feed it to RNBO.
    //------------------------------------------------------------------
    juce::MidiBuffer incomingMidi;
    keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample, numSamples, true);

    const RNBO::MillisecondTime blockStart = rnboObject.getCurrentTime();

    for (const auto metadata : incomingMidi)
    {
        const auto msg = metadata.getMessage();
        const auto offsetMs = (1000.0 * metadata.samplePosition) / currentSampleRate;

        RNBO::MidiEvent event (blockStart + offsetMs,
                               0, // port
                               msg.getRawData(),
                               (size_t) msg.getRawDataSize());
        rnboObject.scheduleEvent (event);
    }

    //------------------------------------------------------------------
    // 2. Render the RNBO patch into the scratch buffers.
    //------------------------------------------------------------------
    for (int ch = 0; ch < numRnboOutputs; ++ch)
        scratchPtrs[(size_t) ch] = scratchData.data() + (size_t) ch * (size_t) maxBlockSize;

    RNBO::SampleValue* const* nullInputs = nullptr; // typed null (no audio inputs)
    rnboObject.process (nullInputs, 0,
                        scratchPtrs.data(), (RNBO::Index) numRnboOutputs,
                        (size_t) numSamples);

    //------------------------------------------------------------------
    // 3. Convert RNBO output -> JUCE float buffer (mono patch -> all channels).
    //------------------------------------------------------------------
    const int numOutChannels = buffer->getNumChannels();
    for (int ch = 0; ch < numOutChannels; ++ch)
    {
        auto* dst = buffer->getWritePointer (ch, bufferToFill.startSample);
        const auto* src = scratchPtrs[(size_t) juce::jmin (ch, numRnboOutputs - 1)];

        for (int i = 0; i < numSamples; ++i)
            dst[i] = (float) src[i];
    }
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::resized()
{
    keyboardComponent.setBounds (getLocalBounds().reduced (8));
}
