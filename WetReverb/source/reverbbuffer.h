//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#pragma once

#include <vector>
#include <cstring>
#include <cmath>
#include <random>

namespace Yonie {

//------------------------------------------------------------------------
// OnePoleFilter - Simple 1st-order (6 dB/oct) filter
//------------------------------------------------------------------------
class OnePoleFilter
{
public:
    enum class Type { LowPass, HighPass };
    
    OnePoleFilter() : z1(0.0f), type(Type::LowPass), coefficient(0.0f) {}
    
    void setCoefficients(double sampleRate, double cutoffHz, Type filterType)
    {
        type = filterType;
        double omega = 2.0 * 3.14159265358979323846 * cutoffHz / sampleRate;
        coefficient = static_cast<float>(1.0 - std::exp(-omega));
    }
    
    void setCoefficient(float coef, Type filterType)
    {
        coefficient = coef;
        type = filterType;
    }
    
    float process(float input)
    {
        float output;
        if (type == Type::LowPass)
        {
            output = coefficient * input + (1.0f - coefficient) * z1;
        }
        else
        {
            float lpf = coefficient * input + (1.0f - coefficient) * z1;
            output = input - lpf;
        }
        z1 = output;
        return output;
    }
    
    void reset() { z1 = 0.0f; }
    
private:
    float z1;
    Type type;
    float coefficient;
};

//------------------------------------------------------------------------
// LinearResampler - Same as WetDelay, proven working
//------------------------------------------------------------------------
class LinearResampler
{
public:
    LinearResampler() : phase(0.0), lastSample(0.0f) {}
    
    void reset()
    {
        phase = 0.0;
        lastSample = 0.0f;
    }
    
    // Downsample from higher rate to lower rate
    int downsample(const float* input, int inputSamples,
                   float* output, int maxOutputSamples,
                   double inputRate, double outputRate)
    {
        double ratio = inputRate / outputRate;
        int outCount = 0;
        
        for (int i = 0; i < inputSamples && outCount < maxOutputSamples; ++i)
        {
            float currentSample = input[i];
            
            while (phase < 1.0 && outCount < maxOutputSamples)
            {
                float t = static_cast<float>(phase);
                output[outCount++] = lastSample + t * (currentSample - lastSample);
                phase += ratio;
            }
            
            phase -= 1.0;
            lastSample = currentSample;
        }
        
        return outCount;
    }
    
    // Upsample from lower rate to higher rate
    int upsample(const float* input, int inputSamples,
                 float* output, int outputSamples,
                 double inputRate, double outputRate)
    {
        double ratio = inputRate / outputRate;
        int inIndex = 0;
        
        for (int i = 0; i < outputSamples; ++i)
        {
            float t = static_cast<float>(phase);
            float currentSample = (inIndex < inputSamples) ? input[inIndex] : lastSample;
            output[i] = lastSample + t * (currentSample - lastSample);
            
            phase += ratio;
            while (phase >= 1.0 && inIndex < inputSamples)
            {
                lastSample = input[inIndex++];
                phase -= 1.0;
            }
        }
        
        if (inIndex > 0 && inIndex <= inputSamples)
            lastSample = input[inIndex - 1];
        
        return outputSamples;
    }
    
private:
    double phase;
    float lastSample;
};

//------------------------------------------------------------------------
// DelayLine - Simple circular buffer
//------------------------------------------------------------------------
class DelayLine
{
public:
    DelayLine() : writePos(0), delayLength(1) {}
    
    void prepare(int maxDelaySamples)
    {
        buffer.resize(maxDelaySamples + 1, 0.0f);
        writePos = 0;
    }
    
    void setDelay(int delaySamples)
    {
        delayLength = std::max(1, std::min(delaySamples, static_cast<int>(buffer.size()) - 1));
    }
    
    float read() const
    {
        int readPos = writePos - delayLength;
        if (readPos < 0) readPos += static_cast<int>(buffer.size());
        return buffer[readPos];
    }
    
    void write(float sample)
    {
        buffer[writePos] = sample;
        writePos++;
        if (writePos >= static_cast<int>(buffer.size())) writePos = 0;
    }
    
    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
    
private:
    std::vector<float> buffer;
    int writePos;
    int delayLength;
};

//------------------------------------------------------------------------
// CombFilter - Schroeder comb with LPF in feedback
//------------------------------------------------------------------------
class CombFilter
{
public:
    CombFilter() : writePos(0), delayLength(1), dampingState(0.0f) {}
    
    void prepare(int maxDelaySamples)
    {
        buffer.resize(maxDelaySamples + 1, 0.0f);
        writePos = 0;
        dampingState = 0.0f;
    }
    
    void setDelay(int delaySamples)
    {
        delayLength = std::max(1, std::min(delaySamples, static_cast<int>(buffer.size()) - 1));
    }
    
    float process(float input, float feedback, float dampingCoef)
    {
        int readPos = writePos - delayLength;
        if (readPos < 0) readPos += static_cast<int>(buffer.size());
        float delayed = buffer[readPos];
        
        // LPF in feedback path
        dampingState = dampingCoef * delayed + (1.0f - dampingCoef) * dampingState;
        
        // Write to buffer
        buffer[writePos] = input + dampingState * feedback;
        writePos++;
        if (writePos >= static_cast<int>(buffer.size())) writePos = 0;
        
        return delayed;
    }
    
    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        dampingState = 0.0f;
    }
    
private:
    std::vector<float> buffer;
    int writePos;
    int delayLength;
    float dampingState;
};

//------------------------------------------------------------------------
// AllpassFilter - For diffusion
//------------------------------------------------------------------------
class AllpassFilter
{
public:
    AllpassFilter() : writePos(0), delayLength(1) {}
    
    void prepare(int maxDelaySamples)
    {
        buffer.resize(maxDelaySamples + 1, 0.0f);
        writePos = 0;
    }
    
    void setDelay(int delaySamples)
    {
        delayLength = std::max(1, std::min(delaySamples, static_cast<int>(buffer.size()) - 1));
    }
    
    float process(float input, float feedback)
    {
        int readPos = writePos - delayLength;
        if (readPos < 0) readPos += static_cast<int>(buffer.size());
        float delayed = buffer[readPos];
        float output = -feedback * input + delayed;
        buffer[writePos] = input + feedback * output;
        writePos++;
        if (writePos >= static_cast<int>(buffer.size())) writePos = 0;
        return output;
    }
    
    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
    
private:
    std::vector<float> buffer;
    int writePos;
    int delayLength;
};

//------------------------------------------------------------------------
// EarlyReflectionTap - Single tap definition for early reflections
//------------------------------------------------------------------------
struct EarlyReflectionTap
{
    float delayMs;      // Delay time in milliseconds
    float gainL;        // Left channel gain
    float gainR;        // Right channel gain
};

//------------------------------------------------------------------------
// EarlyReflectionBuffer - Multitapped delay line for early reflections
//   (separate from DelayLine since we need multi-tap reads)
//------------------------------------------------------------------------
class EarlyReflectionBuffer
{
public:
    static constexpr int MAX_TAPS = 8;
    
    EarlyReflectionBuffer() : writePos(0), bufferSize(0), numTaps(0) {}
    
    void prepare(int maxDelaySamples)
    {
        bufferSize = maxDelaySamples + 1;
        buffer.resize(bufferSize, 0.0f);
        writePos = 0;
        for (int i = 0; i < MAX_TAPS; i++) {
            tapFilterStateL[i] = 0.0f;
            tapFilterStateR[i] = 0.0f;
        }
    }
    
    void setTaps(const EarlyReflectionTap* taps, int count, double sampleRate)
    {
        numTaps = std::min(count, MAX_TAPS);
        for (int i = 0; i < numTaps; ++i)
        {
            tapDelaySamples[i] = std::max(1, static_cast<int>(taps[i].delayMs * sampleRate / 1000.0));
            tapDelaySamples[i] = std::min(tapDelaySamples[i], bufferSize - 1);
            tapGainL[i] = taps[i].gainL;
            tapGainR[i] = taps[i].gainR;
            // Per-tap LPF: first tap nearly transparent, later taps progressively darker
            // coef=0.95 → ~11kHz, coef=0.55 → ~4kHz at 24kHz sample rate
            tapFilterCoef[i] = 0.95f - 0.05f * static_cast<float>(i);
        }
    }
    
    void write(float sample)
    {
        buffer[writePos] = sample;
        writePos++;
        if (writePos >= bufferSize) writePos = 0;
    }
    
    void processToStereo(float& outL, float& outR)
    {
        outL = 0.0f;
        outR = 0.0f;
        for (int i = 0; i < numTaps; ++i)
        {
            int readPos = writePos - tapDelaySamples[i];
            if (readPos < 0) readPos += bufferSize;
            float sample = buffer[readPos];
            
            float filteredL = tapFilterCoef[i] * sample + (1.0f - tapFilterCoef[i]) * tapFilterStateL[i];
            float filteredR = tapFilterCoef[i] * sample + (1.0f - tapFilterCoef[i]) * tapFilterStateR[i];
            tapFilterStateL[i] = filteredL;
            tapFilterStateR[i] = filteredR;
            
            outL += filteredL * tapGainL[i];
            outR += filteredR * tapGainR[i];
        }
    }
    
    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        for (int i = 0; i < MAX_TAPS; i++) {
            tapFilterStateL[i] = 0.0f;
            tapFilterStateR[i] = 0.0f;
        }
    }
    
private:
    std::vector<float> buffer;
    int writePos;
    int bufferSize;
    int numTaps;
    int tapDelaySamples[MAX_TAPS];
    float tapGainL[MAX_TAPS];
    float tapGainR[MAX_TAPS];
    float tapFilterCoef[MAX_TAPS];
    float tapFilterStateL[MAX_TAPS];
    float tapFilterStateR[MAX_TAPS];
};

//------------------------------------------------------------------------
// Early reflection patterns for each reverb program
//------------------------------------------------------------------------
enum class ReverbProgram { Room = 0, Plate, Hall, Cathedral, Cosmos, Count };

struct EarlyReflectionPattern
{
    int numTaps;
    EarlyReflectionTap taps[EarlyReflectionBuffer::MAX_TAPS];
};

// Defined in reverbbuffer.cpp
extern const EarlyReflectionPattern EARLY_REFLECTION_PATTERNS[5];

//------------------------------------------------------------------------
// ReverbBuffer - Stereo reverb using WetDelay's proven resampling
//------------------------------------------------------------------------
class ReverbBuffer
{
public:
    ReverbBuffer();
    ~ReverbBuffer();
    
    void prepare(double sampleRate, float maxDecaySeconds);
    
    void processStereo(float* leftIn, float* leftOut,
                       float* rightIn, float* rightOut,
                       int numSamples,
                       float decaySeconds,
                       float preDelayMs,
                       float hfDampHz,
                       int numCombs,
                       float feedback,
                       float allpassFeedback,
                       int earlyPattern,
                       float earlyLevel,
                       float lateLevel,
                       float baseDelayMs);
    
    void reset();
    
private:
    static constexpr double INTERNAL_SAMPLE_RATE = 24000.0;
    static constexpr int MAX_COMBS = 6;
    static constexpr int MAX_ALLPASSES = 4;
    static constexpr float CROSSTALK = 0.01f;
    
    CombFilter combsL[MAX_COMBS];
    CombFilter combsR[MAX_COMBS];
    AllpassFilter allpassL[MAX_ALLPASSES];
    AllpassFilter allpassR[MAX_ALLPASSES];
    DelayLine preDelayL;
    DelayLine preDelayR;
    EarlyReflectionBuffer earlyReflections;
    
    LinearResampler downsamplerL;
    LinearResampler downsamplerR;
    LinearResampler upsamplerL;
    LinearResampler upsamplerR;
    
    OnePoleFilter antiAliasL;
    OnePoleFilter antiAliasR;
    OnePoleFilter reconstructL;
    OnePoleFilter reconstructR;
    OnePoleFilter lowPassL;
    OnePoleFilter lowPassR;
    OnePoleFilter highPassL;
    OnePoleFilter highPassR;
    
    std::vector<float> tempDownL;
    std::vector<float> tempDownR;
    std::vector<float> tempInternalL;   // Reverb input (from downsample)
    std::vector<float> tempInternalR;
    std::vector<float> tempOutL;        // Reverb output (to upsample)
    std::vector<float> tempOutR;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
    
    double hostSampleRate;
    int maxDelaySamples;
    
    void setupCombs(int numCombs, float baseDelayMs);
    void setupAllpasses();
    void setupEarlyReflections(int pattern);
    
    int msToSamples(float ms) const { return static_cast<int>(ms * INTERNAL_SAMPLE_RATE / 1000.0); }
};

//------------------------------------------------------------------------
} // namespace Yonie