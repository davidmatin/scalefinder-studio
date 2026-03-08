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
        alternativeKeys.clear();
        detectedBPM = 0.0f;
        detectedBPMConfidence = 0.0f;
        songTitle.clear();
        songArtist.clear();
        coverArt = {};
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

std::vector<AudioAnalyzer::AlternativeKey> AudioAnalyzer::getAlternativeKeys() const
{
    const juce::ScopedLock sl (resultLock);
    return alternativeKeys;
}

juce::String AudioAnalyzer::getDetectedKeyName() const
{
    const juce::ScopedLock sl (resultLock);
    return detectedKeyName;
}

float AudioAnalyzer::getDetectedBPM() const
{
    const juce::ScopedLock sl (resultLock);
    return detectedBPM;
}

float AudioAnalyzer::getDetectedBPMConfidence() const
{
    const juce::ScopedLock sl (resultLock);
    return detectedBPMConfidence;
}

juce::String AudioAnalyzer::getSongTitle() const
{
    const juce::ScopedLock sl (resultLock);
    return songTitle;
}

juce::String AudioAnalyzer::getSongArtist() const
{
    const juce::ScopedLock sl (resultLock);
    return songArtist;
}

juce::Image AudioAnalyzer::getCoverArt() const
{
    const juce::ScopedLock sl (resultLock);
    return coverArt;
}

// ── ID3v2 tag parser ─────────────────────────────────────────────────────
AudioAnalyzer::SongMetadata AudioAnalyzer::parseID3v2Tags (const juce::File& file)
{
    SongMetadata meta;
    auto stream = file.createInputStream();
    if (stream == nullptr) return meta;

    // Check for "ID3" header
    char header[10];
    if (stream->read (header, 10) != 10) return meta;
    if (header[0] != 'I' || header[1] != 'D' || header[2] != '3') return meta;

    int versionMajor = (unsigned char) header[3];  // 3 or 4
    // Synchsafe tag size
    uint32_t tagSize = ((uint32_t)(unsigned char)header[6] << 21)
                     | ((uint32_t)(unsigned char)header[7] << 14)
                     | ((uint32_t)(unsigned char)header[8] << 7)
                     | ((uint32_t)(unsigned char)header[9]);

    int64_t tagEnd = 10 + (int64_t) tagSize;

    while (stream->getPosition() < tagEnd)
    {
        // Frame header: 4-byte ID, 4-byte size, 2-byte flags
        char frameHeader[10];
        if (stream->read (frameHeader, 10) != 10) break;

        juce::String frameId (frameHeader, 4);
        uint32_t frameSize;
        if (versionMajor >= 4)
        {
            // v2.4: synchsafe frame size
            frameSize = ((uint32_t)(unsigned char)frameHeader[4] << 21)
                      | ((uint32_t)(unsigned char)frameHeader[5] << 14)
                      | ((uint32_t)(unsigned char)frameHeader[6] << 7)
                      | ((uint32_t)(unsigned char)frameHeader[7]);
        }
        else
        {
            // v2.3: regular big-endian
            frameSize = ((uint32_t)(unsigned char)frameHeader[4] << 24)
                      | ((uint32_t)(unsigned char)frameHeader[5] << 16)
                      | ((uint32_t)(unsigned char)frameHeader[6] << 8)
                      | ((uint32_t)(unsigned char)frameHeader[7]);
        }

        if (frameSize == 0 || frameSize > 10000000) break;  // sanity check

        if (frameId == "TIT2" || frameId == "TPE1")
        {
            // Text frame: 1 byte encoding + text data
            std::vector<char> data ((size_t) frameSize);
            stream->read (data.data(), (int) frameSize);

            unsigned char encoding = (unsigned char) data[0];
            juce::String text;
            if (encoding == 0) // ISO-8859-1
                text = juce::String (data.data() + 1, frameSize - 1);
            else if (encoding == 3) // UTF-8
                text = juce::String::fromUTF8 (data.data() + 1, (int) frameSize - 1);
            else if (encoding == 1 || encoding == 2) // UTF-16
                text = juce::String (juce::CharPointer_UTF8 (data.data() + 1)); // best effort

            if (frameId == "TIT2") meta.title = text.trim();
            else                   meta.artist = text.trim();
        }
        else if (frameId == "APIC" && ! meta.artwork.isValid())
        {
            // Picture frame: encoding, MIME (null-terminated), pic type, description (null-terminated), image data
            std::vector<uint8_t> data ((size_t) frameSize);
            stream->read (data.data(), (int) frameSize);

            size_t pos = 1; // skip encoding byte
            // Skip MIME type string
            while (pos < data.size() && data[pos] != 0) ++pos;
            ++pos; // skip null
            ++pos; // skip picture type byte
            // Skip description
            while (pos < data.size() && data[pos] != 0) ++pos;
            ++pos; // skip null

            if (pos < data.size())
            {
                auto img = juce::ImageFileFormat::loadFrom (data.data() + pos,
                                                            data.size() - pos);
                if (img.isValid())
                    meta.artwork = img.rescaled (72, 72, juce::Graphics::mediumResamplingQuality);
            }
        }
        else
        {
            stream->setPosition (stream->getPosition() + (int64_t) frameSize);
        }
    }

    return meta;
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

    // ── 1b. Extract song metadata (ID3 tags) ────────────────────────────
    {
        auto meta = parseID3v2Tags (fileToAnalyze);

        // Fallback: try JUCE reader metadata (works for WAV/AIFF/OGG)
        if (meta.title.isEmpty())
            meta.title = reader->metadataValues.getValue ("id3title", "");
        if (meta.artist.isEmpty())
            meta.artist = reader->metadataValues.getValue ("id3artist", "");
        // Also try generic keys
        if (meta.title.isEmpty())
            meta.title = reader->metadataValues.getValue ("Title", "");
        if (meta.artist.isEmpty())
            meta.artist = reader->metadataValues.getValue ("Artist", "");

        // Fallback: use filename as title
        if (meta.title.isEmpty())
            meta.title = fileToAnalyze.getFileNameWithoutExtension();

        const juce::ScopedLock sl (resultLock);
        songTitle  = meta.title;
        songArtist = meta.artist;
        coverArt   = meta.artwork;
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
    juce::String keyName;
    if (bestCorr >= (double) minCorrelation)
    {
        const int* intervals = bestIsMajor ? MAJOR_INTERVALS : MINOR_INTERVALS;
        for (int i = 0; i < 7; ++i)
            result.insert ((bestRoot + intervals[i]) % 12);

        keyName = juce::String (noteNames[bestRoot]) + (bestIsMajor ? " Major" : " Minor");
        DBG ("AudioAnalyzer: Detected key = " + keyName + " (r=" + juce::String (bestCorr, 3) + ")");
    }
    else
    {
        DBG ("AudioAnalyzer: No confident key detection (best r=" + juce::String (bestCorr, 3) + ")");
    }

    // ── 7. Compute alternative keys (Circle of Fifths neighbors) ────────
    std::vector<AlternativeKey> alts;
    if (bestCorr >= (double) minCorrelation)
    {
        // Alt 1: Subdominant (5th below = 5 semitones up from root)
        // Catches: detected dominant instead of real tonic
        int subRoot = (bestRoot + 5) % 12;
        {
            AlternativeKey alt;
            const int* ints = bestIsMajor ? MAJOR_INTERVALS : MINOR_INTERVALS;
            for (int i = 0; i < 7; ++i)
                alt.pitchClasses.insert ((subRoot + ints[i]) % 12);
            alt.name = juce::String (noteNames[subRoot]) + (bestIsMajor ? " Major" : " Minor");
            alts.push_back (alt);
        }

        // Alt 2: Dominant (5th above = 7 semitones up from root)
        // Catches: detected subdominant instead of real tonic
        int domRoot = (bestRoot + 7) % 12;
        {
            AlternativeKey alt;
            const int* ints = bestIsMajor ? MAJOR_INTERVALS : MINOR_INTERVALS;
            for (int i = 0; i < 7; ++i)
                alt.pitchClasses.insert ((domRoot + ints[i]) % 12);
            alt.name = juce::String (noteNames[domRoot]) + (bestIsMajor ? " Major" : " Minor");
            alts.push_back (alt);
        }

        DBG ("AudioAnalyzer: Alt 1 (subdominant): " + alts[0].name);
        DBG ("AudioAnalyzer: Alt 2 (dominant): " + alts[1].name);
    }

    // ── 8. Store results ─────────────────────────────────────────────────
    {
        const juce::ScopedLock sl (resultLock);
        detectedPitchClasses = result;
        detectedKeyName = keyName;
        alternativeKeys = alts;
    }

    DBG ("AudioAnalyzer: Detected " + juce::String ((int) result.size()) + " pitch classes from "
         + fileToAnalyze.getFileName());

    // ── 8.5. BPM detection: multi-band spectral flux ODF + fractional-lag AC ─
    // Based on Scheirer (1998): run the same analysis on three frequency bands
    // (full spectrum, bass 50-300 Hz, mid 300-2000 Hz) and vote across bands.
    //
    // Per-band algorithm:
    //  1. Spectral flux ODF (log-compressed magnitude diff, per-band FFT bins)
    //  2. Moving-average subtraction to remove DC / slow amplitude envelope
    //  3. Fractional-lag AC (0.25 BPM steps) with 4-harmonic weighting:
    //       AC(T) + 0.5*AC(2T) + 0.25*AC(3T) + 0.125*AC(4T)
    //  4. Extended BPM range 50-220; improved octave correction (checks ×0.5,
    //     ×2 candidates and prefers 70-155 BPM natural range)
    //
    // Confidence: 1.0 = all 3 bands agree; 0.65 = 2 agree; 0.4 = bass only;
    //             0.0 = no result
    float bpm = 0.0f;
    float bpmConfidence = 0.0f;
    {
        const int bpmFftSize = 2048;
        const int bpmHop     = 512;
        int numFrames = analysisSampleCount / bpmHop;

        if (numFrames > 32 && !threadShouldExit())
        {
            audiofft::AudioFFT bpmFft;
            bpmFft.init ((size_t) bpmFftSize);
            size_t bpmComplexSize = audiofft::AudioFFT::ComplexSize ((size_t) bpmFftSize);

            // Pre-compute Hann window
            std::vector<float> bpmWin ((size_t) bpmFftSize);
            for (int i = 0; i < bpmFftSize; ++i)
                bpmWin[(size_t) i] = 0.5f * (1.0f - std::cos (2.0f * (float) M_PI * i / (bpmFftSize - 1)));

            std::vector<float> bpmBuf     ((size_t) bpmFftSize);
            std::vector<float> bpmRe      ((size_t) bpmComplexSize);
            std::vector<float> bpmIm      ((size_t) bpmComplexSize);
            std::vector<float> prevLogMag ((size_t) bpmComplexSize, 0.0f);
            std::vector<float> currLogMag ((size_t) bpmComplexSize);

            // Frequency band boundaries (FFT bin indices)
            // Bass: 50-300 Hz — contains kick drum / bass guitar (best BPM indicator)
            // Mid:  300-2000 Hz — contains snare, hi-hat
            int bassLow  = std::max (1,    (int) std::ceil  (50.0   * bpmFftSize / targetSampleRate));
            int bassHigh = std::min ((int) bpmComplexSize - 1,
                                     (int) std::floor (300.0  * bpmFftSize / targetSampleRate));
            int midLow   =           (int) std::ceil  (300.0  * bpmFftSize / targetSampleRate);
            int midHigh  = std::min ((int) bpmComplexSize - 1,
                                     (int) std::floor (2000.0 * bpmFftSize / targetSampleRate));

            // ── Stage 1: spectral flux per band (single FFT pass) ─────────────
            std::vector<float> onsetFull ((size_t) numFrames, 0.0f);
            std::vector<float> onsetBass ((size_t) numFrames, 0.0f);
            std::vector<float> onsetMid  ((size_t) numFrames, 0.0f);
            const float kLog = 1000.0f;

            for (int f = 0; f < numFrames && !threadShouldExit(); ++f)
            {
                int offset    = f * bpmHop;
                int available = std::min (bpmFftSize, analysisSampleCount - offset);

                for (int i = 0; i < available; ++i)
                    bpmBuf[(size_t) i] = analysisSamples[offset + i] * bpmWin[(size_t) i];
                for (int i = available; i < bpmFftSize; ++i)
                    bpmBuf[(size_t) i] = 0.0f;

                bpmFft.fft (bpmBuf.data(), bpmRe.data(), bpmIm.data());

                float fluxFull = 0.0f, fluxBass = 0.0f, fluxMid = 0.0f;
                for (int b = 0; b < (int) bpmComplexSize; ++b)
                {
                    float mag    = std::sqrt (bpmRe[(size_t) b] * bpmRe[(size_t) b]
                                           + bpmIm[(size_t) b] * bpmIm[(size_t) b]);
                    float logMag = std::log (1.0f + kLog * mag);
                    currLogMag[(size_t) b] = logMag;
                    float diff = logMag - prevLogMag[(size_t) b];
                    if (diff > 0.0f)
                    {
                        fluxFull += diff;
                        if (b >= bassLow && b <= bassHigh) fluxBass += diff;
                        if (b >= midLow  && b <= midHigh)  fluxMid  += diff;
                    }
                }
                onsetFull[(size_t) f] = fluxFull;
                onsetBass[(size_t) f] = fluxBass;
                onsetMid [(size_t) f] = fluxMid;
                std::swap (currLogMag, prevLogMag);
            }

            if (!threadShouldExit())
            {
                double sr = targetSampleRate;
                // lagMaxF: lag in frames for 50 BPM (extended lower bound)
                float lagMaxF = (float) (60.0 * sr / ((double) bpmHop * 50.0));

                // ── Stage 2+3+4 encapsulated as a lambda (reused per band) ──
                // Takes onset envelope by value (modified in-place by Stage 2).
                // Returns {finalBPM, peakSharpness 0-1}.
                auto computeBPMFromEnv = [&] (std::vector<float> env) -> std::pair<float, float>
                {
                    int n = (int) env.size();

                    // Stage 2: moving-average subtraction (prefix-sum O(n))
                    int halfWin = std::max (1, (int) (targetSampleRate / bpmHop) / 2);
                    std::vector<float> prefix ((size_t) (n + 1), 0.0f);
                    for (int f2 = 0; f2 < n; ++f2)
                        prefix[(size_t) (f2 + 1)] = prefix[(size_t) f2] + env[(size_t) f2];
                    for (int f2 = 0; f2 < n; ++f2)
                    {
                        int lo = std::max (0, f2 - halfWin);
                        int hi = std::min (n - 1, f2 + halfWin);
                        float mn = (prefix[(size_t) (hi + 1)] - prefix[(size_t) lo])
                                   / (float) (hi - lo + 1);
                        env[(size_t) f2] = std::max (0.0f, env[(size_t) f2] - mn);
                    }

                    // Stage 3: fractional-lag AC with 4-harmonic weighting
                    // Interpolates envelope at fractional lag → eliminates quantisation error.
                    auto computeACfrac = [&] (float lagF) -> float
                    {
                        if (lagF < 1.0f || lagF >= (float) (n - 1)) return 0.0f;
                        int   lagI  = (int) lagF;
                        float frac  = lagF - (float) lagI;
                        int   count = n - lagI - 1;
                        if (count <= 0) return 0.0f;
                        float ac = 0.0f;
                        for (int i = 0; i < count; ++i)
                        {
                            float shifted = env[(size_t) (i + lagI)]     * (1.0f - frac)
                                          + env[(size_t) (i + lagI + 1)] * frac;
                            ac += env[(size_t) i] * shifted;
                        }
                        return ac / (float) count;
                    };

                    // Compute 4-harmonic score for any BPM
                    auto scoreForBPM = [&] (float b) -> float
                    {
                        if (b < 50.0f || b > 220.0f) return 0.0f;
                        float lagF = (float) (60.0 * sr / ((double) bpmHop * (double) b));
                        float s = computeACfrac (lagF);
                        if (lagF * 2.0f <= lagMaxF) s += 0.5f   * computeACfrac (lagF * 2.0f);
                        if (lagF * 3.0f <= lagMaxF) s += 0.25f  * computeACfrac (lagF * 3.0f);
                        if (lagF * 4.0f <= lagMaxF) s += 0.125f * computeACfrac (lagF * 4.0f);
                        return s;
                    };

                    float bestScore  = 0.0f;
                    float secondBest = 0.0f;
                    float bestBPM    = 0.0f;

                    // Extended search range: 50-220 BPM at 0.25 BPM steps
                    for (float cBPM = 50.0f; cBPM <= 220.0f && !threadShouldExit(); cBPM += 0.25f)
                    {
                        float score = scoreForBPM (cBPM);
                        if (score > bestScore)
                        {
                            secondBest = bestScore;
                            bestScore  = score;
                            bestBPM    = cBPM;
                        }
                        else if (score > secondBest)
                        {
                            secondBest = score;
                        }
                    }

                    if (bestBPM == 0.0f || bestScore == 0.0f) return { 0.0f, 0.0f };

                    // Peak sharpness: how much best score exceeds second-best (0-1)
                    float sharpness = (secondBest > 0.0f)
                        ? juce::jmin (1.0f, (bestScore - secondBest) / bestScore)
                        : 1.0f;

                    // Stage 4: improved octave correction
                    // Check ×2 and ×0.5 candidates; prefer whichever falls in
                    // the 70-155 BPM "natural" range without sacrificing accuracy.
                    float candidateBPMs[3] = { bestBPM, bestBPM * 2.0f, bestBPM * 0.5f };
                    float naturalBPM   = 0.0f;
                    float naturalScore = 0.0f;

                    for (int ci = 0; ci < 3; ++ci)
                    {
                        float cb = candidateBPMs[ci];
                        if (cb >= 70.0f && cb <= 155.0f)
                        {
                            float cs = scoreForBPM (cb);
                            if (cs > naturalScore) { naturalScore = cs; naturalBPM = cb; }
                        }
                    }

                    float finalBPM = bestBPM;
                    if (naturalBPM > 0.0f && naturalScore >= 0.45f * bestScore)
                        finalBPM = naturalBPM;

                    if (finalBPM < 50.0f || finalBPM > 220.0f) finalBPM = 0.0f;

                    return { finalBPM, sharpness };
                };

                // Run analysis on all 3 bands
                auto resultFull = computeBPMFromEnv (onsetFull);
                auto resultBass = computeBPMFromEnv (onsetBass);
                auto resultMid  = computeBPMFromEnv (onsetMid);

                float bpmFull  = resultFull.first,  confFull  = resultFull.second;
                float bpmBass  = resultBass.first,  confBass  = resultBass.second;
                float bpmMid   = resultMid.first,   confMid   = resultMid.second;

                DBG ("AudioAnalyzer: BPM estimates — Full:" + juce::String (bpmFull, 1)
                     + " Bass:" + juce::String (bpmBass, 1)
                     + " Mid:"  + juce::String (bpmMid,  1));

                // ── Multi-band voting (agree = within 3%) ──────────────────
                auto bpmMatch = [] (float a, float b2) -> bool
                {
                    if (a <= 0.0f || b2 <= 0.0f) return false;
                    return std::abs (a - b2) / ((a + b2) * 0.5f) < 0.03f;
                };

                bool fullBassAgree = bpmMatch (bpmFull, bpmBass);
                bool fullMidAgree  = bpmMatch (bpmFull, bpmMid);
                bool bassMidAgree  = bpmMatch (bpmBass, bpmMid);
                int  agreements    = (fullBassAgree ? 1 : 0) + (fullMidAgree ? 1 : 0) + (bassMidAgree ? 1 : 0);

                if (agreements == 3)
                {
                    // All agree — weighted average by sharpness
                    float totalConf = confFull + confBass + confMid;
                    bpm = totalConf > 0.0f
                        ? (bpmFull * confFull + bpmBass * confBass + bpmMid * confMid) / totalConf
                        : (bpmFull + bpmBass + bpmMid) / 3.0f;
                    bpmConfidence = 1.0f;
                }
                else if (fullBassAgree)
                {
                    bpm = (bpmFull + bpmBass) * 0.5f;
                    bpmConfidence = 0.70f;
                }
                else if (fullMidAgree)
                {
                    bpm = (bpmFull + bpmMid) * 0.5f;
                    bpmConfidence = 0.65f;
                }
                else if (bassMidAgree)
                {
                    bpm = (bpmBass + bpmMid) * 0.5f;
                    bpmConfidence = 0.65f;
                }
                else if (bpmBass > 0.0f)
                {
                    // Bass band most reliable when bands disagree
                    bpm = bpmBass;
                    bpmConfidence = confBass * 0.45f;
                }
                else if (bpmFull > 0.0f)
                {
                    bpm = bpmFull;
                    bpmConfidence = confFull * 0.40f;
                }

                // Round to nearest 0.5 BPM (avoid spurious sub-integer precision)
                if (bpm > 0.0f)
                    bpm = std::round (bpm * 2.0f) / 2.0f;
            }
        }

        {
            const juce::ScopedLock sl (resultLock);
            detectedBPM = bpm;
            detectedBPMConfidence = bpmConfidence;
        }

        DBG ("AudioAnalyzer: BPM = " + juce::String (bpm, 1)
             + " (confidence " + juce::String (bpmConfidence, 2) + ")");
    }

    analysisComplete.store (true);
}
