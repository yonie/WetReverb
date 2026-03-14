//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#include "wetreverbprocessor.h"
#include "wetreverbcids.h"
#include "wetreverbcontroller.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <cmath>

using namespace Steinberg;

namespace Yonie {

//------------------------------------------------------------------------
// Reverb preset configurations
// Inspired by Yamaha R1000, EHX Holy Grail, Roland DEP-5
struct ReverbPreset {
    float decaySeconds;
    float preDelayMs;
    float hfDampHz;
    int numCombs;
    float feedback;
    float allpassFeedback;
    float earlyLevel;
    float lateLevel;
    float baseDelayMs;
};

static const ReverbPreset REVERB_PRESETS[5] = {
    // Room: tight, grainy, dark - RT60 ~0.6s
    { 0.6f, 0.0f, 3500.0f, 4, 0.62f, 0.50f, 0.25f, 0.75f, 15.0f },
    // Plate: denser, metallic - RT60 ~1.6s
    { 1.6f, 0.0f, 4500.0f, 4, 0.78f, 0.50f, 0.18f, 0.82f, 22.0f },
    // Hall: smooth, warm - RT60 ~2.2s
    { 2.2f, 20.0f, 4500.0f, 5, 0.84f, 0.50f, 0.15f, 0.85f, 28.0f },
    // Cathedral: lush, long - RT60 ~3.0s
    { 3.0f, 30.0f, 5000.0f, 5, 0.88f, 0.50f, 0.12f, 0.88f, 35.0f },
    // Cosmos: ethereal infinite - RT60 ~4.5s
    { 4.5f, 40.0f, 5000.0f, 5, 0.91f, 0.50f, 0.10f, 0.90f, 45.0f }
};

//------------------------------------------------------------------------
// WetReverbProcessorProcessor
//------------------------------------------------------------------------
WetReverbProcessorProcessor::WetReverbProcessorProcessor ()
{
	//--- set the wanted controller for our processor
	setControllerClass (kWetReverbProcessorControllerUID);
}

//------------------------------------------------------------------------
WetReverbProcessorProcessor::~WetReverbProcessorProcessor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated
	
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//--- create Audio IO ------
	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	/* If you don't need an event bus, you can remove the next line */
	addEventInput (STR16 ("Event In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
	
	//---do not forget to call parent ------
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::setActive (TBool state)
{
	//--- called when the Plug-in is enable/disable (On/Off) -----
	if (state)
	{
		// Reset meters when activated
		inputPeakL = 0.0f;
		inputPeakR = 0.0f;
		outputPeakL = 0.0f;
		outputPeakR = 0.0f;
		samplesSinceLastMeterUpdate = 0;
	}
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::process (Vst::ProcessData& data)
{
	//--- Read parameter changes -----------
	if (data.inputParameterChanges)
	{
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			if (auto* paramQueue = data.inputParameterChanges->getParameterData (index))
			{
				Vst::ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount ();
				
				if (paramQueue->getParameterId () == kReverbModeParam && numPoints > 0)
				{
					// Get last parameter change
					if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
					{
						// Convert normalized value (0.0-1.0) to index (0-4)
						int newMode = static_cast<int>(value * 4.0 + 0.5);
						if (newMode < 0) newMode = 0;
						if (newMode > 4) newMode = 4;
						
						// Clear reverb buffers on mode change to avoid artifacts
						if (newMode != currentReverbMode)
						{
							reverbBuffer.reset();
							currentReverbMode = newMode;
						}
					}
				}
			}
		}
	}
	
	//--- Process audio -----------
	if (data.numInputs == 0 || data.numOutputs == 0)
		return kResultOk;
		
	if (data.numSamples > 0)
	{
		// Get input and output buffers
		Vst::AudioBusBuffers& input = data.inputs[0];
		Vst::AudioBusBuffers& output = data.outputs[0];
		
		// Ensure we have stereo
		if (input.numChannels >= 2 && output.numChannels >= 2)
		{
			float* inputL = input.channelBuffers32[0];
			float* inputR = input.channelBuffers32[1];
			float* outputL = output.channelBuffers32[0];
			float* outputR = output.channelBuffers32[1];
			
			// Measure input levels
			for (int32 i = 0; i < data.numSamples; i++)
			{
				updatePeak(inputL[i], inputPeakL);
				updatePeak(inputR[i], inputPeakR);
			}
			
			// Process reverb (100% wet)
			const ReverbPreset& preset = REVERB_PRESETS[currentReverbMode];
			reverbBuffer.processStereo(inputL, outputL, inputR, outputR,
			                           data.numSamples,
			                           preset.decaySeconds,
			                           preset.preDelayMs,
			                           preset.hfDampHz,
			                           preset.numCombs,
			                           preset.feedback,
			                           preset.allpassFeedback,
			                           preset.earlyLevel,
			                           preset.lateLevel,
			                           preset.baseDelayMs);
			
			// Measure output levels
			for (int32 i = 0; i < data.numSamples; i++)
			{
				updatePeak(outputL[i], outputPeakL);
				updatePeak(outputR[i], outputPeakR);
			}
			
			// Send meter data via message every ~16.67ms for 60fps (at 44100Hz that's ~735 samples)
			samplesSinceLastMeterUpdate += data.numSamples;
			if (samplesSinceLastMeterUpdate >= meterUpdateInterval)
			{
				samplesSinceLastMeterUpdate = 0;
				sendMeterData();
			}
			
			// Output is not silent
			output.silenceFlags = 0;
		}
		else
		{
			// Clear output if not stereo
			for (int32 c = 0; c < output.numChannels; c++)
			{
				memset(output.channelBuffers32[c], 0,
				       data.numSamples * sizeof(Vst::Sample32));
			}
			output.silenceFlags = ((uint64)1 << output.numChannels) - 1;
		}
	}

	return kResultOk;
}

//------------------------------------------------------------------------
void WetReverbProcessorProcessor::sendMeterData()
{
	// Create message to send meter data to controller
	if (auto message = owned(allocateMessage()))
	{
		message->setMessageID(kMeterDataMessage);
		
		// Pack the four meter values
		float meterData[4] = {
			inputPeakL.load(),
			inputPeakR.load(),
			outputPeakL.load(),
			outputPeakR.load()
		};
		
		message->getAttributes()->setBinary("data", meterData, sizeof(meterData));
		sendMessage(message);
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::setupProcessing (Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	// Initialize reverb buffer with max reverb time (4.5s for Cosmos mode)
	reverbBuffer.prepare(newSetup.sampleRate, 4.5f);
	
	// Calculate meter update interval (~16.67ms worth of samples for 60fps)
	meterUpdateInterval = static_cast<int32>(newSetup.sampleRate / 60.0);
	
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::setState (IBStream* state)
{
	// called when we load a preset, the model has to be reloaded
	IBStreamer streamer (state, kLittleEndian);
	
	int32 savedReverbMode = 0;
	if (streamer.readInt32(savedReverbMode) == kResultTrue)
	{
		currentReverbMode = savedReverbMode;
	}
	
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::getState (IBStream* state)
{
	// here we need to save the model
	IBStreamer streamer (state, kLittleEndian);
	
	streamer.writeInt32(currentReverbMode);

	return kResultOk;
}

//------------------------------------------------------------------------
void WetReverbProcessorProcessor::updatePeak(float sample, std::atomic<float>& peak)
{
	float absSample = std::abs(sample);
	float currentPeak = peak.load();
	
	if (absSample > currentPeak)
	{
		// Attack: instant
		peak.store(absSample);
	}
	else
	{
		// Decay: exponential
		peak.store(currentPeak * METER_DECAY);
	}
}

//------------------------------------------------------------------------
} // namespace Yonie