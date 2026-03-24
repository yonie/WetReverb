//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Yonie {
//------------------------------------------------------------------------
static const Steinberg::FUID kWetReverbProcessorProcessorUID (0xC8D4E21F, 0x8A72B45A, 0x9F18C23D, 0x5B6E4F01);
static const Steinberg::FUID kWetReverbProcessorControllerUID (0xD7F8A32B, 0x45C19E67, 0xA23F8B40, 0x1E9D5C72);

#define WetReverbProcessorVST3Category "Fx|Reverb"

// Message ID for meter data communication between processor and controller
inline const char* kMeterDataMessage = "MeterData";

//------------------------------------------------------------------------
// Parameter IDs
//------------------------------------------------------------------------
enum WetReverbParams : Steinberg::Vst::ParamID
{
	kReverbModeParam = 0,  // 0-4 for 5 reverb modes

	// Output-only meter parameters for UI display
	kInputMeterL = 1,
	kInputMeterR = 2,
	kOutputMeterL = 3,
	kOutputMeterR = 4,

	kParamCount = 5
};

// Button control tags (100-104 for the 5 reverb mode buttons)
enum ButtonTags
{
	kReverbButton0 = 100,
	kReverbButton1 = 101,
	kReverbButton2 = 102,
	kReverbButton3 = 103,
	kReverbButton4 = 104
};

//------------------------------------------------------------------------
// Reverb preset configurations (shared between processor and controller)
//------------------------------------------------------------------------
struct ReverbPreset {
	float preDelayMs;
	float hfDampHz;
	int numCombs;
	float feedback;
	float allpassFeedback;
	int earlyPattern;
	float earlyLevel;
	float lateLevel;
	float baseDelayMs;
};

static const ReverbPreset REVERB_PRESETS[5] = {
	// Room: prominent early reflections, short tail, intimate space
	{ 17.0f, 7500.0f, 4, 0.62f, 0.64f, 0, 0.26f, 0.33f, 7.0f },
	// Plate: dense shimmer, tight base delay, no pre-delay
	{ 0.0f, 7100.0f, 8, 0.77f, 0.75f, 1, 0.10f, 0.55f, 8.5f },
	// Hall: spacious, balanced, smooth diffusion
	{ 25.0f, 7500.0f, 10, 0.82f, 0.72f, 2, 0.22f, 0.39f, 18.0f },
	// Cathedral: long diffuse tail, wide pre-delay
	{ 36.5f, 7000.0f, 10, 0.91f, 0.75f, 3, 0.15f, 0.66f, 25.0f },
	// Cosmos: ethereal infinite wash, bright, long pre-delay
	{ 65.0f, 9500.0f, 12, 0.91f, 0.78f, 4, 0.20f, 0.60f, 32.0f }
};

//------------------------------------------------------------------------
} // namespace Yonie