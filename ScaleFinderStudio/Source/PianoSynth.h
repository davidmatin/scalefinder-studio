#pragma once
#include <JuceHeader.h>

// ═══════════════════════════════════════════════════════════════════════════
// Simple piano-like voice using sine harmonics + ADSR
// ═══════════════════════════════════════════════════════════════════════════

class PianoSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class PianoVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<PianoSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int) override
    {
        noteFreq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        level = velocity * 0.3f;
        phase = 0.0;
        phase2 = 0.0;
        phase3 = 0.0;
        phase4 = 0.0;

        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
            clearCurrentNote();
    }

    void prepareToPlay (double sampleRate, int, int)
    {
        adsr.setSampleRate (sampleRate);

        // Piano-like envelope: fast attack, medium decay, sustain, release
        juce::ADSR::Parameters params;
        params.attack  = 0.005f;
        params.decay   = 0.8f;
        params.sustain = 0.3f;
        params.release = 0.5f;
        adsr.setParameters (params);
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override
    {
        if (! adsr.isActive())
            return;

        auto sr = getSampleRate();
        auto phaseInc  = noteFreq / sr;
        auto phaseInc2 = noteFreq * 2.0 / sr;
        auto phaseInc3 = noteFreq * 3.0 / sr;
        auto phaseInc4 = noteFreq * 4.0 / sr;

        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            // Fundamental + harmonics (piano-like timbre)
            auto sample = std::sin (phase * juce::MathConstants<double>::twoPi) * 1.0
                        + std::sin (phase2 * juce::MathConstants<double>::twoPi) * 0.5
                        + std::sin (phase3 * juce::MathConstants<double>::twoPi) * 0.15
                        + std::sin (phase4 * juce::MathConstants<double>::twoPi) * 0.05;

            auto env = adsr.getNextSample();
            auto out = static_cast<float> (sample * level * env);

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                outputBuffer.addSample (ch, i, out);

            phase  += phaseInc;
            phase2 += phaseInc2;
            phase3 += phaseInc3;
            phase4 += phaseInc4;

            if (phase  >= 1.0) phase  -= 1.0;
            if (phase2 >= 1.0) phase2 -= 1.0;
            if (phase3 >= 1.0) phase3 -= 1.0;
            if (phase4 >= 1.0) phase4 -= 1.0;
        }

        if (! adsr.isActive())
            clearCurrentNote();
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

private:
    double noteFreq = 440.0;
    float level = 0.0f;
    double phase = 0.0, phase2 = 0.0, phase3 = 0.0, phase4 = 0.0;
    juce::ADSR adsr;
};

// ═══════════════════════════════════════════════════════════════════════════
// Polyphonic piano synth (12 voices)
// ═══════════════════════════════════════════════════════════════════════════

class PianoSynth
{
public:
    PianoSynth()
    {
        synth.addSound (new PianoSound());
        for (int i = 0; i < 12; ++i)
            synth.addVoice (new PianoVoice());
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock)
    {
        synth.setCurrentPlaybackSampleRate (sampleRate);
        for (int i = 0; i < synth.getNumVoices(); ++i)
            if (auto* voice = dynamic_cast<PianoVoice*> (synth.getVoice (i)))
                voice->prepareToPlay (sampleRate, samplesPerBlock, 2);
    }

    void renderNextBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                          int startSample, int numSamples)
    {
        synth.renderNextBlock (buffer, midi, startSample, numSamples);
    }

    void noteOn (int channel, int noteNumber, float velocity)
    {
        synth.noteOn (channel, noteNumber, velocity);
    }

    void noteOff (int channel, int noteNumber, float velocity)
    {
        synth.noteOff (channel, noteNumber, velocity, true);
    }

    void allNotesOff()
    {
        synth.allNotesOff (0, true);
    }

private:
    juce::Synthesiser synth;
};
