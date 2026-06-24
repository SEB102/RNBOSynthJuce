#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

// RNBO C++ export header. Provided by the export you generate in RNBO
// (folder "export/", with "Export RNBO C++ Library" enabled).
#include "RNBO.h"

//==============================================================================
// Standalone synth:
//   - On-screen virtual MIDI keyboard (juce::MidiKeyboardComponent)
//   - Audio engine = the exported RNBO patch (cycle~ + line~ ADSR)
//
class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void resized() override;

private:
    juce::MidiKeyboardState     keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;

    // The RNBO patcher. The default constructor loads the patcher that was
    // compiled in from export/rnbo_source.cpp (same pattern as rnbo.example.juce).
    RNBO::CoreObject rnboObject;

    double currentSampleRate = 44100.0;
    int    numRnboOutputs    = 1;
    int    maxBlockSize      = 512;

    // Scratch buffers in RNBO's native sample type (double or float depending
    // on the export). Filled by RNBO, then converted to JUCE float buffers.
    std::vector<RNBO::SampleValue>  scratchData;
    std::vector<RNBO::SampleValue*> scratchPtrs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
