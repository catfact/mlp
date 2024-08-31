#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), editorInput(p.mlp, mlpGui), editorOutput(p.mlp)
{
    juce::ignoreUnused (processorRef);

    addAndMakeVisible(mlpGui);
    setSize (1200, 800);
    mlpGui.SetOutput(&editorOutput);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void AudioPluginAudioProcessorEditor::resized()
{
    mlpGui.setBounds (getBoundsInParent());
}
