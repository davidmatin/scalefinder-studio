#include "AudioAnalyzer.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ── Albrecht & Shanahan (2013) key profiles ──────────────────────────────
// Derived from large corpus analysis of real music. Best empirical accuracy
// in benchmarks; significantly better than Krumhansl (1990) and Temperley (1999).
// Profile index 0 = tonic, rotated for each root note.
static const double KEY_PROFILE_MAJOR[12] = {
    0.238, 0.006, 0.111, 0.006, 0.137, 0.094, 0.016, 0.214, 0.009, 0.080, 0.008, 0.081
};
static const double KEY_PROFILE_MINOR[12] = {
    0.220, 0.006, 0.104, 0.123, 0.019, 0.103, 0.012, 0.214, 0.062, 0.022, 0.061, 0.052
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

    // ── 5. Chromagram via semitone filterbank ───────────────────────────
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

    // Pre-compute magnitudes buffer
    std::vector<float> magnitudes (complexSize, 0.0f);

    // ── Pre-compute semitone filterbank ──
    // For each MIDI note (C1=24 to B7=107), find the FFT bin range
    // covering ±0.5 semitones around the note's center frequency.
    // All octaves fold into 12 pitch classes.
    // Semitone filterbank: for each MIDI note, store the FFT bin range
    // covering ±0.5 semitones. All octaves fold into 12 pitch classes.
    struct ChromaBand { int lowBin; int highBin; int pitchClass; };
    std::vector<ChromaBand> filterbank;

    for (int midi = 24; midi <= 107; ++midi)
    {
        double centerFreq = 440.0 * std::pow (2.0, (double) (midi - 69) / 12.0);
        double lowFreq  = centerFreq * std::pow (2.0, -1.0 / 24.0);
        double highFreq = centerFreq * std::pow (2.0,  1.0 / 24.0);

        int lo = (int) std::ceil  (lowFreq  * (double) fftSize / targetSampleRate);
        int hi = (int) std::floor (highFreq * (double) fftSize / targetSampleRate);

        if (hi < minBin || lo > maxBin) continue;
        lo = juce::jmax (lo, minBin);
        hi = juce::jmin (hi, maxBin);

        if (lo <= hi)
            filterbank.push_back ({ lo, hi, midi % 12 });
    }

    DBG ("AudioAnalyzer: Filterbank has " + juce::String ((int) filterbank.size())
         + " bands across " + juce::String (minFreqHz, 0) + "-" + juce::String (maxFreqHz, 0) + " Hz");

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

        // Pre-compute all bin magnitudes
        for (int bin = 0; bin < (int) complexSize; ++bin)
            magnitudes[(size_t) bin] = std::sqrt (re[(size_t) bin] * re[(size_t) bin]
                                                + im[(size_t) bin] * im[(size_t) bin]);

        // ── Percussive frame filtering via spectral flatness ──
        // High flatness = energy spread evenly = noise/percussion → skip
        {
            double logSum = 0.0, linSum = 0.0;
            int flatCount = 0;
            for (int bin = minBin; bin <= maxBin; ++bin)
            {
                float mag = magnitudes[(size_t) bin];
                if (mag > 0.0f) { logSum += std::log ((double) mag); flatCount++; }
                linSum += (double) mag;
            }
            if (flatCount > 0)
            {
                double geoMean = std::exp (logSum / flatCount);
                double ariMean = linSum / flatCount;
                double flatness = (ariMean > 0.0) ? geoMean / ariMean : 0.0;
                if (flatness > 0.8)
                    continue;  // skip percussive/noisy frame
            }
        }

        // ── Peak-picked filterbank chromagram (HPCP-style) ──
        // Only accumulate spectral peaks (local maxima) into pitch classes.
        // This focuses on tonal content and removes broadband energy that
        // can bias the chromagram toward non-tonic pitch classes.
        double frameChroma[12] = {};

        for (const auto& band : filterbank)
        {
            double peakSum = 0.0;
            for (int bin = band.lowBin; bin <= band.highBin; ++bin)
            {
                float mag = magnitudes[(size_t) bin];
                // Only count this bin if it's a local maximum (spectral peak)
                bool isPeak = (bin > 0 && bin < (int) complexSize - 1
                               && mag > magnitudes[(size_t) (bin - 1)]
                               && mag >= magnitudes[(size_t) (bin + 1)]);
                if (isPeak)
                {
                    // Weight by prominence: how much peak rises above neighbors.
                    // Fundamentals have sharp peaks; harmonics are broader/weaker.
                    double prominence = (double) mag
                        - juce::jmax ((double) magnitudes[(size_t) (bin - 1)],
                                      (double) magnitudes[(size_t) (bin + 1)]);
                    peakSum += (double) mag * prominence;
                }
            }
            frameChroma[band.pitchClass] += peakSum;
        }

        // Log compression — reduces dynamic range so loud partials
        // don't dominate the pitch class distribution
        for (int i = 0; i < 12; ++i)
            frameChroma[i] = std::log1p (frameChroma[i]);

        // ── Per-frame L2 normalization ──
        // Every frame contributes equally regardless of volume
        double norm = 0.0;
        for (int i = 0; i < 12; ++i)
            norm += frameChroma[i] * frameChroma[i];
        norm = std::sqrt (norm);

        if (norm > 0.0)
        {
            for (int i = 0; i < 12; ++i)
                chroma[i] += frameChroma[i] / norm;
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
            rotatedMajor[(i + root) % 12] = KEY_PROFILE_MAJOR[i];
            rotatedMinor[(i + root) % 12] = KEY_PROFILE_MINOR[i];
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
