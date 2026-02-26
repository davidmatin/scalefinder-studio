#pragma once
#include <JuceHeader.h>
#include <set>
#include <map>

class AudioAnalyzer : public juce::Thread
{
public:
    AudioAnalyzer();
    ~AudioAnalyzer() override;

    // Start analysis on a background thread
    void analyzeFile (const juce::File& audioFile, double hostSampleRate);

    // Check if analysis is complete (resets flag on read)
    bool isAnalysisComplete();

    // Get results (call after isAnalysisComplete() returns true)
    std::set<int> getDetectedPitchClasses() const;

    // Alternative key suggestions (Circle of Fifths neighbors)
    struct AlternativeKey {
        std::set<int> pitchClasses;
        juce::String name;  // e.g. "B Major"
    };
    std::vector<AlternativeKey> getAlternativeKeys() const;
    juce::String getDetectedKeyName() const;
    float getDetectedBPM() const;

    // Configurable settings
    int fftSize = 8192;                  // FFT size (must be power of 2)
    float amplitudeThreshold = 0.02f;    // RMS threshold to skip silence
    float minCorrelation = 0.3f;         // Minimum Pearson r to accept key detection
    float minFreqHz = 65.0f;            // Ignore frequencies below this (C2)
    float maxFreqHz = 2100.0f;           // Ignore frequencies above this (per Korzeniowski 2017)

private:
    void run() override;

    static int hzToMidi (float hz);
    static int hzToPitchClass (float hz);
    static double pearsonCorrelation (const double* x, const double* y, int n);

    juce::File fileToAnalyze;
    double targetSampleRate = 44100.0;
    std::set<int> detectedPitchClasses;
    juce::String detectedKeyName;
    std::vector<AlternativeKey> alternativeKeys;
    float detectedBPM = 0.0f;
    std::atomic<bool> analysisComplete { false };
    mutable juce::CriticalSection resultLock;
};
