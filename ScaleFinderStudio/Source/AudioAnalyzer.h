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

    // Configurable settings
    int fftSize = 4096;                  // FFT size (must be power of 2)
    float amplitudeThreshold = 0.02f;    // RMS threshold to skip silence
    float minCorrelation = 0.3f;         // Minimum Pearson r to accept key detection
    float minFreqHz = 80.0f;            // Ignore frequencies below this
    float maxFreqHz = 5000.0f;           // Ignore frequencies above this

private:
    void run() override;

    static int hzToMidi (float hz);
    static int hzToPitchClass (float hz);
    static double pearsonCorrelation (const double* x, const double* y, int n);

    juce::File fileToAnalyze;
    double targetSampleRate = 44100.0;
    std::set<int> detectedPitchClasses;
    std::atomic<bool> analysisComplete { false };
    mutable juce::CriticalSection resultLock;
};
