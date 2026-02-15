#include "AudioAnalyzer.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ── Krumhansl key profiles (from "Cognitive Foundations of Musical Pitch") ──
// These represent the perceptual weight of each pitch class in major/minor keys.
// Profile index 0 = tonic, rotated for each root note.
static const double KRUMHANSL_MAJOR[12] = {
    6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88
};
static const double KRUMHANSL_MINOR[12] = {
    6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17
};

// Scale intervals for building pitch class sets from detected key
static const int MAJOR_INTERVALS[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int MINOR_INTERVALS[7] = { 0, 2, 3, 5, 7, 8, 10 };

AudioAnalyzer::AudioAnalyzer() : Thread ("AudioAnalyzer") {}

AudioAnalyzer::~AudioAnalyzer()
{
    stopThread (5000);
}

void AudioAnalyzer::analyzeFile (const juce::File& audioFile, double hostSampleRate)
{
    // Stop any running analysis
    stopThread (3000);

    fileToAnalyze = audioFile;
    targetSampleRate = hostSampleRate > 0 ? hostSampleRate : 44100.0;
    analysisComplete.store (false);

    {
        const juce::ScopedLock sl (resultLock);
        detectedPitchClasses.clear();
    }

    startThread();
}

bool AudioAnalyzer::isAnalysisComplete()
{
    return analysisComplete.exchange (false);
}

std::set<int> AudioAnalyzer::getDetectedPitchClasses() const
{
    const juce::ScopedLock sl (resultLock);
    return detectedPitchClasses;
}

int AudioAnalyzer::hzToMidi (float hz)
{
    if (hz <= 0.0f) return -1;
    return (int) std::round (69.0 + 12.0 * std::log2 ((double) hz / 440.0));
}

int AudioAnalyzer::hzToPitchClass (float hz)
{
    int midi = hzToMidi (hz);
    if (midi < 0) return -1;
    return midi % 12;
}

double AudioAnalyzer::pearsonCorrelation (const double* x, const double* y, int n)
{
    double sumX = 0, sumY = 0;
    for (int i = 0; i < n; ++i) { sumX += x[i]; sumY += y[i]; }
    double meanX = sumX / n;
    double meanY = sumY / n;

    double num = 0, denX = 0, denY = 0;
    for (int i = 0; i < n; ++i)
    {
        double dx = x[i] - meanX;
        double dy = y[i] - meanY;
        num += dx * dy;
        denX += dx * dx;
        denY += dy * dy;
    }
    if (denX <= 0.0 || denY <= 0.0) return 0.0;
    return num / (std::sqrt (denX) * std::sqrt (denY));
}

void AudioAnalyzer::run()
{
    // ── 1. Load audio file ───────────────────────────────────────────────
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();  // WAV, AIFF, FLAC, (+ MP3/OGG if available)

    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager.createReaderFor (fileToAnalyze));

    if (reader == nullptr)
    {
        DBG ("AudioAnalyzer: Could not read file: " + fileToAnalyze.getFullPathName());
        analysisComplete.store (true);
        return;
    }

    if (threadShouldExit()) return;

    // ── 2. Read into buffer ──────────────────────────────────────────────
    auto numSamples = (int) reader->lengthInSamples;
    auto numChannels = (int) reader->numChannels;
    double fileSampleRate = reader->sampleRate;

    juce::AudioBuffer<float> fileBuffer (numChannels, numSamples);
    reader->read (&fileBuffer, 0, numSamples, 0, true, true);

    if (threadShouldExit()) return;

    // ── 3. Convert to mono ───────────────────────────────────────────────
    juce::AudioBuffer<float> monoBuffer (1, numSamples);

    if (numChannels == 1)
    {
        monoBuffer.copyFrom (0, 0, fileBuffer, 0, 0, numSamples);
    }
    else
    {
        // Average all channels
        monoBuffer.clear();
        for (int ch = 0; ch < numChannels; ++ch)
            monoBuffer.addFrom (0, 0, fileBuffer, ch, 0, numSamples, 1.0f / numChannels);
    }

    if (threadShouldExit()) return;

    // ── 4. Resample if needed ────────────────────────────────────────────
    const float* analysisSamples = monoBuffer.getReadPointer (0);
    int analysisSampleCount = numSamples;
    juce::AudioBuffer<float> resampledBuffer;

    if (std::abs (fileSampleRate - targetSampleRate) > 1.0)
    {
        double ratio = fileSampleRate / targetSampleRate;
        int outputLength = (int) ((double) numSamples / ratio) + 1;
        resampledBuffer.setSize (1, outputLength);

        juce::LagrangeInterpolator interpolator;
        int produced = interpolator.process (ratio,
                                              monoBuffer.getReadPointer (0),
                                              resampledBuffer.getWritePointer (0),
                                              outputLength);

        analysisSamples = resampledBuffer.getReadPointer (0);
        analysisSampleCount = produced;

        DBG ("AudioAnalyzer: Resampled from " + juce::String (fileSampleRate)
             + " to " + juce::String (targetSampleRate)
             + " (" + juce::String (analysisSampleCount) + " samples)");
    }

    if (threadShouldExit()) return;

    // ── 5. Chromagram computation via FFT ────────────────────────────────
    audiofft::AudioFFT fft;
    fft.init ((size_t) fftSize);

    size_t complexSize = audiofft::AudioFFT::ComplexSize ((size_t) fftSize);
    std::vector<float> windowedBuf ((size_t) fftSize);
    std::vector<float> re (complexSize);
    std::vector<float> im (complexSize);

    // Pre-compute Hann window
    std::vector<float> hannWindow ((size_t) fftSize);
    for (int i = 0; i < fftSize; ++i)
        hannWindow[(size_t) i] = 0.5f * (1.0f - std::cos (2.0f * (float) M_PI * (float) i / (float) (fftSize - 1)));

    // Chromagram accumulator (12 pitch classes)
    double chroma[12] = {};

    int hopSize = fftSize / 2;
    int minBin = (int) std::ceil ((double) minFreqHz * fftSize / targetSampleRate);
    int maxBin = (int) std::floor ((double) maxFreqHz * fftSize / targetSampleRate);
    maxBin = juce::jmin (maxBin, (int) complexSize - 1);

    for (int pos = 0; pos + fftSize <= analysisSampleCount; pos += hopSize)
    {
        if (threadShouldExit()) return;

        const float* chunk = analysisSamples + pos;

        // Check RMS amplitude — skip silence
        float sumSq = 0.0f;
        for (int i = 0; i < fftSize; ++i)
            sumSq += chunk[i] * chunk[i];
        float rms = std::sqrt (sumSq / (float) fftSize);

        if (rms < amplitudeThreshold)
            continue;

        // Apply Hann window
        for (int i = 0; i < fftSize; ++i)
            windowedBuf[(size_t) i] = chunk[i] * hannWindow[(size_t) i];

        // Compute FFT
        fft.fft (windowedBuf.data(), re.data(), im.data());

        // Map FFT bins to pitch classes and accumulate magnitude
        for (int bin = minBin; bin <= maxBin; ++bin)
        {
            float magnitude = std::sqrt (re[(size_t) bin] * re[(size_t) bin]
                                       + im[(size_t) bin] * im[(size_t) bin]);

            float freq = (float) bin * (float) targetSampleRate / (float) fftSize;
            int midiNote = (int) std::round (69.0 + 12.0 * std::log2 ((double) freq / 440.0));
            int pc = ((midiNote % 12) + 12) % 12;

            chroma[pc] += (double) magnitude;
        }
    }

    // ── 6. Krumhansl-Schmuckler key profile matching ────────────────────
    // Correlate chromagram against all 24 key profiles (12 major + 12 minor)
    const char* noteNames[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    double bestCorr = -2.0;
    int bestRoot = 0;
    bool bestIsMajor = true;

    DBG ("AudioAnalyzer: Key correlations:");
    for (int root = 0; root < 12; ++root)
    {
        // Rotate profile: for key with this root, profile[i] maps to chroma[(i + root) % 12]
        double rotatedMajor[12], rotatedMinor[12];
        for (int i = 0; i < 12; ++i)
        {
            rotatedMajor[(i + root) % 12] = KRUMHANSL_MAJOR[i];
            rotatedMinor[(i + root) % 12] = KRUMHANSL_MINOR[i];
        }

        double corrMaj = pearsonCorrelation (chroma, rotatedMajor, 12);
        double corrMin = pearsonCorrelation (chroma, rotatedMinor, 12);

        DBG ("  " + juce::String (noteNames[root]) + " Major: " + juce::String (corrMaj, 3)
             + "  |  " + juce::String (noteNames[root]) + " Minor: " + juce::String (corrMin, 3));

        if (corrMaj > bestCorr) { bestCorr = corrMaj; bestRoot = root; bestIsMajor = true; }
        if (corrMin > bestCorr) { bestCorr = corrMin; bestRoot = root; bestIsMajor = false; }
    }

    std::set<int> result;
    if (bestCorr >= (double) minCorrelation)
    {
        const int* intervals = bestIsMajor ? MAJOR_INTERVALS : MINOR_INTERVALS;
        for (int i = 0; i < 7; ++i)
            result.insert ((bestRoot + intervals[i]) % 12);

        DBG ("AudioAnalyzer: Detected key = " + juce::String (noteNames[bestRoot])
             + (bestIsMajor ? " Major" : " Minor") + " (r=" + juce::String (bestCorr, 3) + ")");
    }
    else
    {
        DBG ("AudioAnalyzer: No confident key detection (best r=" + juce::String (bestCorr, 3) + ")");
    }

    // ── 7. Store results ─────────────────────────────────────────────────
    {
        const juce::ScopedLock sl (resultLock);
        detectedPitchClasses = result;
    }

    DBG ("AudioAnalyzer: Detected " + juce::String ((int) result.size()) + " pitch classes from "
         + fileToAnalyze.getFileName());

    analysisComplete.store (true);
}
