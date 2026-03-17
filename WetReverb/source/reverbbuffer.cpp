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
// Early reflection tap patterns per program
// Each tap: { delayMs, gainL, gainR }
// L/R gains differ slightly for stereo imaging.
// Gains decay with distance to simulate absorption.
//------------------------------------------------------------------------
const EarlyReflectionPattern EARLY_REFLECTION_PATTERNS[5] = {
    // Room: tight, closely spaced - small room walls close by
    { 6, {
        {  3.1f, 0.42f, 0.36f },
        {  7.3f, 0.35f, 0.40f },
        { 11.7f, 0.30f, 0.27f },
        { 16.1f, 0.22f, 0.25f },
        { 19.8f, 0.16f, 0.14f },
        { 24.3f, 0.10f, 0.12f },
        {  0.0f, 0.00f, 0.00f },
        {  0.0f, 0.00f, 0.00f },
    }},
    // Plate: very dense, nearly uniform - metal plate has close reflections
    { 5, {
        {  1.7f, 0.38f, 0.35f },
        {  4.3f, 0.36f, 0.38f },
        {  7.9f, 0.33f, 0.31f },
        { 11.3f, 0.30f, 0.33f },
        { 16.7f, 0.26f, 0.24f },
        {  0.0f, 0.00f, 0.00f },
        {  0.0f, 0.00f, 0.00f },
        {  0.0f, 0.00f, 0.00f },
    }},
    // Hall: wider spacing, gradual decay - large concert hall
    { 7, {
        {  8.3f, 0.38f, 0.32f },
        { 15.7f, 0.32f, 0.36f },
        { 22.1f, 0.27f, 0.24f },
        { 29.9f, 0.22f, 0.26f },
        { 37.3f, 0.18f, 0.16f },
        { 44.7f, 0.13f, 0.15f },
        { 53.1f, 0.09f, 0.08f },
        {  0.0f, 0.00f, 0.00f },
    }},
    // Cathedral: wide spacing, slow decay - stone walls far apart
    { 8, {
        { 12.1f, 0.34f, 0.28f },
        { 21.7f, 0.30f, 0.33f },
        { 31.3f, 0.25f, 0.22f },
        { 41.9f, 0.21f, 0.24f },
        { 52.7f, 0.17f, 0.15f },
        { 61.3f, 0.13f, 0.15f },
        { 71.9f, 0.09f, 0.08f },
        { 79.3f, 0.06f, 0.07f },
    }},
    // Cosmos: sparse, ethereal, wide gaps
    { 6, {
        { 15.3f, 0.30f, 0.24f },
        { 31.7f, 0.26f, 0.29f },
        { 49.1f, 0.21f, 0.18f },
        { 63.9f, 0.16f, 0.19f },
        { 78.3f, 0.11f, 0.09f },
        { 91.7f, 0.07f, 0.08f },
        {  0.0f, 0.00f, 0.00f },
        {  0.0f, 0.00f, 0.00f },
    }},
};

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
    
    // Early reflections: max 100ms at internal rate
    earlyReflections.prepare(static_cast<int>(INTERNAL_SAMPLE_RATE * 0.1));
    
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
void ReverbBuffer::setupEarlyReflections(int pattern)
{
    int idx = std::max(0, std::min(pattern, 4));
    const EarlyReflectionPattern& pat = EARLY_REFLECTION_PATTERNS[idx];
    earlyReflections.setTaps(pat.taps, pat.numTaps, INTERNAL_SAMPLE_RATE);
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
                               int earlyPattern,
                               float earlyLevel,
                               float lateLevel,
                               float baseDelayMs)
{
    if (tempDownL.empty() || numSamples <= 0) return;
    
    setupCombs(numCombs, baseDelayMs);
    setupAllpasses();
    setupEarlyReflections(earlyPattern);
    
    int preDelaySamp = std::max(1, std::min(msToSamples(preDelayMs), static_cast<int>(INTERNAL_SAMPLE_RATE * 0.05)));
    preDelayL.setDelay(preDelaySamp);
    preDelayR.setDelay(preDelaySamp + 1);
    
    float dampCoef = static_cast<float>(1.0 - std::exp(-2.0 * 3.14159265358979323846 * std::min(hfDampHz, 10000.0f) / INTERNAL_SAMPLE_RATE));
    dampCoef = std::max(0.02f, std::min(0.98f, dampCoef));
    
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
        
        // Mono sum for comb filters and early reflections
        float monoIn = (inL + inR) * 0.5f;
        
        // Early reflections: tapped delay line with stereo output
        earlyReflections.write(monoIn);
        float erL = 0.0f, erR = 0.0f;
        earlyReflections.processToStereo(erL, erR);
        
        // Parallel comb filters (late reverb)
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
        float lateL = combOutL;
        float lateR = combOutR;
        for (int a = 0; a < MAX_ALLPASSES; ++a)
        {
            lateL = allpassL[a].process(lateL, allpassFeedback);
            lateR = allpassR[a].process(lateR, allpassFeedback);
        }
        
        // Late LPF
        lateL = lowPassL.process(lateL);
        lateR = lowPassR.process(lateR);
        
        // Mix early reflections and late reverb
        float finalL = earlyLevel * erL + lateLevel * lateL;
        float finalR = earlyLevel * erR + lateLevel * lateR;
        
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
        constexpr float NOISE = 0.00005f;
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
    earlyReflections.reset();
    
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