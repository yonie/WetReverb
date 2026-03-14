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
	kReverbModeParam = 0,  // 0-5 for 6 reverb modes
	
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
} // namespace Yonie