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
// WetReverbProcessorProcessor
//------------------------------------------------------------------------
WetReverbProcessorProcessor::WetReverbProcessorProcessor ()
{
	setControllerClass (kWetReverbProcessorControllerUID);
}

//------------------------------------------------------------------------
WetReverbProcessorProcessor::~WetReverbProcessorProcessor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::initialize (FUnknown* context)
{
	tresult result = AudioEffect::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
	addEventInput (STR16 ("Event In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::terminate ()
{
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::setActive (TBool state)
{
	if (state)
	{
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
					if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
					{
						int newMode = static_cast<int>(value * 4.0 + 0.5);
						if (newMode < 0) newMode = 0;
						if (newMode > 4) newMode = 4;

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
		Vst::AudioBusBuffers& input = data.inputs[0];
		Vst::AudioBusBuffers& output = data.outputs[0];

		if (input.numChannels >= 2 && output.numChannels >= 2)
		{
			float* inputL = input.channelBuffers32[0];
			float* inputR = input.channelBuffers32[1];
			float* outputL = output.channelBuffers32[0];
			float* outputR = output.channelBuffers32[1];

			for (int32 i = 0; i < data.numSamples; i++)
			{
				updatePeak(inputL[i], inputPeakL);
				updatePeak(inputR[i], inputPeakR);
			}

			const ReverbPreset& preset = REVERB_PRESETS[currentReverbMode];
			reverbBuffer.processStereo(inputL, outputL, inputR, outputR,
			                           data.numSamples,
			                           preset.preDelayMs,
			                           preset.hfDampHz,
			                           preset.numCombs,
			                           preset.feedback,
			                           preset.allpassFeedback,
			                           preset.earlyPattern,
			                           preset.earlyLevel,
			                           preset.lateLevel,
			                           preset.baseDelayMs);

			for (int32 i = 0; i < data.numSamples; i++)
			{
				updatePeak(outputL[i], outputPeakL);
				updatePeak(outputR[i], outputPeakR);
			}

			samplesSinceLastMeterUpdate += data.numSamples;
			if (samplesSinceLastMeterUpdate >= meterUpdateInterval)
			{
				samplesSinceLastMeterUpdate = 0;
				sendMeterData();
			}

			output.silenceFlags = 0;
		}
		else
		{
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
	if (auto message = owned(allocateMessage()))
	{
		message->setMessageID(kMeterDataMessage);

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
	reverbBuffer.prepare(newSetup.sampleRate, 10.0f);
	meterUpdateInterval = static_cast<int32>(newSetup.sampleRate / 60.0);

	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorProcessor::setState (IBStream* state)
{
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
		peak.store(absSample);
	}
	else
	{
		peak.store(currentPeak * METER_DECAY);
	}
}

//------------------------------------------------------------------------
} // namespace Yonie
