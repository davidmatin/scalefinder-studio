#pragma once
#include <JuceHeader.h>
#include "MusicTheory.h"
#include "PianoSynth.h"
#include <atomic>
#include <set>

// ── Lock-free MIDI event ring buffer (SPSC: GUI writes, audio reads) ────
struct MidiEvent
{
    int noteNumber;
    float velocity;   // > 0 = noteOn, 0 = noteOff
};

class MidiRingBuffer
{
public:
    static constexpr int capacity = 64;

    void push (int noteNumber, float velocity)
    {
        int w = writePos.load (std::memory_order_relaxed);
        int next = (w + 1) % capacity;
        if (next == readPos.load (std::memory_order_acquire))
            return; // Full — drop event (extremely unlikely at 64 slots)
        events[w] = { noteNumber, velocity };
        writePos.store (next, std::memory_order_release);
    }

    bool pop (MidiEvent& out)
    {
        int r = readPos.load (std::memory_order_relaxed);
        if (r == writePos.load (std::memory_order_acquire))
            return false; // Empty
        out = events[r];
        readPos.store ((r + 1) % capacity, std::memory_order_release);
        return true;
    }

private:
    MidiEvent events[capacity] {};
    std::atomic<int> readPos { 0 };
    std::atomic<int> writePos { 0 };
};

// ── Processor ────────────────────────────────────────────────────────────
class ScaleFinderProcessor : public juce::AudioProcessor
{
public:
    ScaleFinderProcessor();
    ~ScaleFinderProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ── Public API for editor (all called from UI thread) ────────────────
    void clearNotes();
    void triggerNoteOn (int noteNumber, float velocity);
    void triggerNoteOff (int noteNumber);
    void triggerNoteOnMono (int noteNumber, float velocity);

    // Toggle API for piano UI clicks (select/deselect pitch classes)
    void togglePitchClassOn (int pitchClass);
    void togglePitchClassOff (int pitchClass);

    // ── Lock-free bitmask accessors ──────────────────────────────────────
    uint16_t getAccumulatedBits() const { return accumulatedBits.load (std::memory_order_acquire); }
    void     setAccumulatedBits (uint16_t bits);

    // Convenience: convert bitmask to std::set<int> (call from UI thread only)
    std::set<int> getAccumulatedNotes() const;
    void setAccumulatedNotes (const std::set<int>& notes);

    // Key result — computed on UI thread, read on UI thread (no lock needed)
    KeyResult getCurrentResult() const;
    void      recomputeResult();   // Called by editor's timer on UI thread
    double    getAnalysisSampleRate() const;

    juce::String selectedKey;
    std::vector<ChordInfo> currentChords;
    std::atomic<bool> needsUIUpdate { false };

    // Volume control (read by audio thread, written by UI thread)
    std::atomic<float> masterVolume { 0.75f };
    std::atomic<bool>  isMuted { false };

    // Instrument selection (0=Synth, 1=Piano, 2=E-Piano, 3=Guitar)
    std::atomic<int> currentInstrument { 0 };

private:
    // Lock-free pitch class tracking (12-bit bitmask, bit N = pitch class N active)
    std::atomic<uint16_t> accumulatedBits { 0 };

    // Key detection result (only written/read on UI thread after Change 2)
    KeyResult currentResult;

    PianoSynth pianoSynth;

    // Lock-free GUI→audio MIDI ring buffer
    MidiRingBuffer guiMidiRing;
    int lastGUINote = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleFinderProcessor)
};
