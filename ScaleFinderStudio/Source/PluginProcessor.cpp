#include "PluginProcessor.h"
#include "PluginEditor.h"

ScaleFinderProcessor::ScaleFinderProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    recomputeResult();
}

ScaleFinderProcessor::~ScaleFinderProcessor() {}

const juce::String ScaleFinderProcessor::getName() const { return JucePlugin_Name; }

bool ScaleFinderProcessor::acceptsMidi()  const { return true; }
bool ScaleFinderProcessor::producesMidi() const { return false; }
bool ScaleFinderProcessor::isMidiEffect() const { return false; }

double ScaleFinderProcessor::getTailLengthSeconds() const { return 1.0; }
int  ScaleFinderProcessor::getNumPrograms()    { return 1; }
int  ScaleFinderProcessor::getCurrentProgram() { return 0; }
void ScaleFinderProcessor::setCurrentProgram (int) {}
const juce::String ScaleFinderProcessor::getProgramName (int) { return {}; }
void ScaleFinderProcessor::changeProgramName (int, const juce::String&) {}

void ScaleFinderProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pianoSynth.prepareToPlay (sampleRate, samplesPerBlock);
}

void ScaleFinderProcessor::releaseResources() {}

bool ScaleFinderProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

// ── processBlock: fully lock-free ────────────────────────────────────────
void ScaleFinderProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Track EXTERNAL MIDI notes for scale detection (toggle on repeat)
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            int pitchClass = msg.getNoteNumber() % 12;
            uint16_t bit = static_cast<uint16_t> (1 << pitchClass);
            // Atomic toggle: XOR flips the bit (lock-free, O(1))
            accumulatedBits.fetch_xor (bit, std::memory_order_relaxed);
            needsUIUpdate.store (true, std::memory_order_release);
        }
        // NoteOff: no action needed for accumulated pitch tracking
    }

    // Drain GUI→audio MIDI ring buffer (lock-free)
    MidiEvent evt;
    while (guiMidiRing.pop (evt))
    {
        if (evt.velocity > 0.0f)
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, evt.noteNumber, evt.velocity), 0);
        else
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, evt.noteNumber, 0.0f), 0);
    }

    // Render piano audio (includes both external + GUI MIDI)
    pianoSynth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master volume / mute
    float gain = isMuted.load (std::memory_order_relaxed) ? 0.0f
                                                          : masterVolume.load (std::memory_order_relaxed);
    if (gain != 1.0f)
        buffer.applyGain (gain);
}

// ── UI-thread API (all called from message thread) ───────────────────────

void ScaleFinderProcessor::clearNotes()
{
    accumulatedBits.store (0, std::memory_order_release);
    selectedKey = "";
    currentChords.clear();
    recomputeResult();
    needsUIUpdate.store (true, std::memory_order_release);
}

void ScaleFinderProcessor::triggerNoteOn (int noteNumber, float velocity)
{
    guiMidiRing.push (noteNumber, velocity);
}

void ScaleFinderProcessor::triggerNoteOff (int noteNumber)
{
    guiMidiRing.push (noteNumber, 0.0f);
}

void ScaleFinderProcessor::triggerNoteOnMono (int noteNumber, float velocity)
{
    // Stop previous GUI note first (monophonic UI clicks)
    if (lastGUINote >= 0 && lastGUINote != noteNumber)
        guiMidiRing.push (lastGUINote, 0.0f);
    guiMidiRing.push (noteNumber, velocity);
    lastGUINote = noteNumber;
}

void ScaleFinderProcessor::togglePitchClassOn (int pitchClass)
{
    uint16_t bit = static_cast<uint16_t> (1 << pitchClass);
    accumulatedBits.fetch_or (bit, std::memory_order_release);
    recomputeResult();
    needsUIUpdate.store (true, std::memory_order_release);
}

void ScaleFinderProcessor::togglePitchClassOff (int pitchClass)
{
    uint16_t bit = static_cast<uint16_t> (1 << pitchClass);
    accumulatedBits.fetch_and (static_cast<uint16_t> (~bit), std::memory_order_release);
    recomputeResult();
    needsUIUpdate.store (true, std::memory_order_release);
}

void ScaleFinderProcessor::setAccumulatedBits (uint16_t bits)
{
    accumulatedBits.store (bits, std::memory_order_release);
    recomputeResult();
    needsUIUpdate.store (true, std::memory_order_release);
}

std::set<int> ScaleFinderProcessor::getAccumulatedNotes() const
{
    std::set<int> result;
    uint16_t bits = accumulatedBits.load (std::memory_order_acquire);
    for (int i = 0; i < 12; ++i)
        if (bits & (1 << i))
            result.insert (i);
    return result;
}

void ScaleFinderProcessor::setAccumulatedNotes (const std::set<int>& notes)
{
    uint16_t bits = 0;
    for (int n : notes)
        bits |= static_cast<uint16_t> (1 << n);
    setAccumulatedBits (bits);
}

KeyResult ScaleFinderProcessor::getCurrentResult() const
{
    return currentResult;
}

void ScaleFinderProcessor::recomputeResult()
{
    auto notes = getAccumulatedNotes();
    currentResult = MusicTheory::getPossibleKeys (notes);
}

double ScaleFinderProcessor::getAnalysisSampleRate() const
{
    double sr = getSampleRate();
    return sr > 0 ? sr : 44100.0;
}

bool ScaleFinderProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* ScaleFinderProcessor::createEditor()
{
    return new ScaleFinderEditor (*this);
}

void ScaleFinderProcessor::getStateInformation (juce::MemoryBlock&) {}
void ScaleFinderProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScaleFinderProcessor();
}
