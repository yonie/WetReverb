//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#include "reverbbuffer.h"
#include <algorithm>
#include <cmath>

namespace Yonie {

static const int PRIMES[] = {
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103,
    107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173
};
static const int NUM_PRIMES = sizeof(PRIMES) / sizeof(PRIMES[0]);

//------------------------------------------------------------------------
ReverbBuffer::ReverbBuffer()
: hostSampleRate(44100.0)
, maxDelaySamples(0)
, rng(std::random_device{}())
, dist(-1.0f, 1.0f)
{
}

//------------------------------------------------------------------------
ReverbBuffer::~ReverbBuffer()
{
}

//------------------------------------------------------------------------
void ReverbBuffer::prepare(double sampleRate, float maxDecaySeconds)
{
    hostSampleRate = sampleRate;
    
    // Clamp sample rate to reasonable range
    if (hostSampleRate < 22050.0) hostSampleRate = 22050.0;
    if (hostSampleRate > 192000.0) hostSampleRate = 192000.0;
    
    maxDelaySamples = static_cast<int>((maxDecaySeconds + 0.1f) * INTERNAL_SAMPLE_RATE);
    
    for (int i = 0; i < MAX_COMBS; i++)
    {
        combsL[i].prepare(maxDelaySamples);
        combsR[i].prepare(maxDelaySamples);
    }
    
    for (int i = 0; i < MAX_ALLPASSES; i++)
    {
        allpassL[i].prepare(static_cast<int>(INTERNAL_SAMPLE_RATE * 0.05));
        allpassR[i].prepare(static_cast<int>(INTERNAL_SAMPLE_RATE * 0.05));
    }
    
    preDelayL.prepare(static_cast<int>(INTERNAL_SAMPLE_RATE * 0.1));
    preDelayR.prepare(static_cast<int>(INTERNAL_SAMPLE_RATE * 0.1));
    
    // Size buffers for max block size
    // tempDownL/R at host rate (8192 samples max)
    // tempInternalL/R, tempOut/LR at internal rate
    int maxHost = 8192;
    int maxInternal = static_cast<int>(maxHost * INTERNAL_SAMPLE_RATE / hostSampleRate) + 64;
    if (maxInternal < 256) maxInternal = 256;
    if (maxInternal > 16384) maxInternal = 16384;
    
    tempDownL.resize(maxHost, 0.0f);
    tempDownR.resize(maxHost, 0.0f);
    tempInternalL.resize(maxInternal, 0.0f);
    tempInternalR.resize(maxInternal, 0.0f);
    tempOutL.resize(maxInternal, 0.0f);
    tempOutR.resize(maxInternal, 0.0f);
    
    antiAliasL.setCoefficients(hostSampleRate, 10000.0, OnePoleFilter::Type::LowPass);
    antiAliasR.setCoefficients(hostSampleRate, 10000.0, OnePoleFilter::Type::LowPass);
    reconstructL.setCoefficients(hostSampleRate, 10000.0, OnePoleFilter::Type::LowPass);
    reconstructR.setCoefficients(hostSampleRate, 10000.0, OnePoleFilter::Type::LowPass);
    highPassL.setCoefficients(INTERNAL_SAMPLE_RATE, 80.0, OnePoleFilter::Type::HighPass);
    highPassR.setCoefficients(INTERNAL_SAMPLE_RATE, 80.0, OnePoleFilter::Type::HighPass);
    // Late output filter (gentle rolloff at ~6kHz for warmth)
    lowPassL.setCoefficients(INTERNAL_SAMPLE_RATE, 6000.0, OnePoleFilter::Type::LowPass);
    lowPassR.setCoefficients(INTERNAL_SAMPLE_RATE, 6000.0, OnePoleFilter::Type::LowPass);
    
    reset();
}

//------------------------------------------------------------------------
void ReverbBuffer::setupCombs(int numCombs, float baseDelayMs)
{
    for (int i = 0; i < numCombs && i < MAX_COMBS; i++)
    {
        int primeIdx = (i * 7 + 3) % NUM_PRIMES;
        float delayMs = baseDelayMs + static_cast<float>(PRIMES[primeIdx]) * 0.25f * (i + 1);
        
        int samplesL = std::max(10, static_cast<int>(delayMs * INTERNAL_SAMPLE_RATE / 1000.0));
        int samplesR = std::max(10, static_cast<int>(delayMs * 1.01f * INTERNAL_SAMPLE_RATE / 1000.0));
        
        samplesL = std::min(samplesL, maxDelaySamples - 1);
        samplesR = std::min(samplesR, maxDelaySamples - 1);
        
        combsL[i].setDelay(samplesL);
        combsR[i].setDelay(samplesR);
    }
}

//------------------------------------------------------------------------
void ReverbBuffer::setupAllpasses()
{
    for (int i = 0; i < MAX_ALLPASSES; i++)
    {
        int primeIdx = (i * 11 + 7) % NUM_PRIMES;
        float delayMs = static_cast<float>(PRIMES[primeIdx]) * 0.05f + 0.3f;
        int samples = std::max(3, static_cast<int>(delayMs * INTERNAL_SAMPLE_RATE / 1000.0));
        
        allpassL[i].setDelay(samples);
        allpassR[i].setDelay(samples + 1);
    }
}

//------------------------------------------------------------------------
void ReverbBuffer::processStereo(float* leftIn, float* leftOut,
                               float* rightIn, float* rightOut,
                               int numSamples,
                               float decaySeconds,
                               float preDelayMs,
                               float hfDampHz,
                               int numCombs,
                               float feedback,
                               float allpassFeedback,
                               float earlyLevel,
                               float lateLevel,
                               float baseDelayMs)
{
    if (tempDownL.empty() || numSamples <= 0) return;
    
    setupCombs(numCombs, baseDelayMs);
    setupAllpasses();
    
    int preDelaySamp = std::max(1, std::min(msToSamples(preDelayMs), static_cast<int>(INTERNAL_SAMPLE_RATE * 0.05)));
    preDelayL.setDelay(preDelaySamp);
    preDelayR.setDelay(preDelaySamp + 1);
    
    float dampCoef = static_cast<float>(1.0 - std::exp(-2.0 * 3.14159265358979323846 * std::min(hfDampHz, 10000.0f) / INTERNAL_SAMPLE_RATE));
    dampCoef = std::max(0.02f, std::min(0.98f, dampCoef));
    
    // Note: dampCoef is used by comb filters in their process() call
    // The output lowPass keeps its fixed cutoff from prepare()
    
    // Step 1: Anti-alias filter at host rate
    for (int i = 0; i < numSamples; ++i)
    {
        tempDownL[i] = antiAliasL.process(leftIn[i]);
        tempDownR[i] = antiAliasR.process(rightIn[i]);
    }
    
    // Step 2: Downsample to 24kHz (into tempInternal buffers)
    int maxInternal = static_cast<int>(tempInternalL.size());
    int internalSamples = downsamplerL.downsample(tempDownL.data(), numSamples,
                                                   tempInternalL.data(), maxInternal,
                                                   hostSampleRate, INTERNAL_SAMPLE_RATE);
    int actualR = downsamplerR.downsample(tempDownR.data(), numSamples,
                                           tempInternalR.data(), maxInternal,
                                           hostSampleRate, INTERNAL_SAMPLE_RATE);
    int actualInternal = std::min(internalSamples, actualR);
    if (actualInternal <= 0) actualInternal = 1;
    
    // Step 3: Process reverb into tempOut buffers
    for (int i = 0; i < actualInternal; ++i)
    {
        // Pre-delay
        preDelayL.write(tempInternalL[i]);
        preDelayR.write(tempInternalR[i]);
        float inL = preDelayL.read();
        float inR = preDelayR.read();
        
        // Mono sum for combs
        float monoIn = (inL + inR) * 0.5f;
        
        // Parallel comb filters
        float combOutL = 0.0f;
        float combOutR = 0.0f;
        for (int c = 0; c < numCombs && c < MAX_COMBS; ++c)
        {
            combOutL += combsL[c].process(monoIn, feedback, dampCoef);
            combOutR += combsR[c].process(monoIn, feedback, dampCoef);
        }
        combOutL /= static_cast<float>(numCombs) * 1.5f;
        combOutR /= static_cast<float>(numCombs) * 1.5f;
        
        // Allpass diffusion
        float outL = combOutL;
        float outR = combOutR;
        for (int a = 0; a < MAX_ALLPASSES; ++a)
        {
            outL = allpassL[a].process(outL, allpassFeedback);
            outR = allpassR[a].process(outR, allpassFeedback);
        }
        
        // Late LPF
        outL = lowPassL.process(outL);
        outR = lowPassR.process(outR);
        
        // Mix early and late
        float earlyL = inL + CROSSTALK * inR;
        float earlyR = inR + CROSSTALK * inL;
        float finalL = earlyLevel * earlyL + lateLevel * outL;
        float finalR = earlyLevel * earlyR + lateLevel * outR;
        
        // Highpass
        finalL = highPassL.process(finalL);
        finalR = highPassR.process(finalR);
        
        // 12-bit dither
        constexpr float LEVELS = 4096.0f;
        constexpr float DITHER = 0.5f / LEVELS;
        float ditherL = (dist(rng) + dist(rng)) * DITHER;
        float ditherR = (dist(rng) + dist(rng)) * DITHER;
        finalL = std::floor((finalL + ditherL) * LEVELS) / LEVELS;
        finalR = std::floor((finalR + ditherR) * LEVELS) / LEVELS;
        
        // Noise floor
        constexpr float NOISE = 0.0001f;
        finalL += dist(rng) * NOISE;
        finalR += dist(rng) * NOISE;
        
        tempOutL[i] = finalL;
        tempOutR[i] = finalR;
    }
    
    // Step 4: Upsample back to host rate
    upsamplerL.upsample(tempOutL.data(), actualInternal,
                       leftOut, numSamples,
                       INTERNAL_SAMPLE_RATE, hostSampleRate);
    upsamplerR.upsample(tempOutR.data(), actualInternal,
                       rightOut, numSamples,
                       INTERNAL_SAMPLE_RATE, hostSampleRate);
    
    // Step 5: Reconstruction filter
    for (int i = 0; i < numSamples; ++i)
    {
        leftOut[i] = reconstructL.process(leftOut[i]);
        rightOut[i] = reconstructR.process(rightOut[i]);
    }
}

//------------------------------------------------------------------------
void ReverbBuffer::reset()
{
    for (int i = 0; i < MAX_COMBS; ++i)
    {
        combsL[i].reset();
        combsR[i].reset();
    }
    for (int i = 0; i < MAX_ALLPASSES; ++i)
    {
        allpassL[i].reset();
        allpassR[i].reset();
    }
    preDelayL.reset();
    preDelayR.reset();
    
    antiAliasL.reset();
    antiAliasR.reset();
    reconstructL.reset();
    reconstructR.reset();
    highPassL.reset();
    highPassR.reset();
    lowPassL.reset();
    lowPassR.reset();
    
    downsamplerL.reset();
    downsamplerR.reset();
    upsamplerL.reset();
    upsamplerR.reset();
    
    std::fill(tempDownL.begin(), tempDownL.end(), 0.0f);
    std::fill(tempDownR.begin(), tempDownR.end(), 0.0f);
    std::fill(tempInternalL.begin(), tempInternalL.end(), 0.0f);
    std::fill(tempInternalR.begin(), tempInternalR.end(), 0.0f);
    std::fill(tempOutL.begin(), tempOutL.end(), 0.0f);
    std::fill(tempOutR.begin(), tempOutR.end(), 0.0f);
}

//------------------------------------------------------------------------
} // namespace Yonie