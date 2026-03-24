//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#include "wetreverbcontroller.h"
#include "wetreverbcids.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstreamer.h"
#include "customviewcreator.h"

using namespace Steinberg;

namespace Yonie {

//------------------------------------------------------------------------
// WetReverbProcessorController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorController::initialize (FUnknown* context)
{
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	registerCustomViews();

	// Register reverb mode parameter (0-4 for 5 positions)
	Vst::StringListParameter* reverbParam = new Vst::StringListParameter(
		STR16("Reverb Mode"),
		kReverbModeParam,
		nullptr,
		Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList
	);

	reverbParam->appendString(STR16("Room"));
	reverbParam->appendString(STR16("Plate"));
	reverbParam->appendString(STR16("Hall"));
	reverbParam->appendString(STR16("Cathedral"));
	reverbParam->appendString(STR16("Cosmos"));

	parameters.addParameter(reverbParam);

	// Register read-only meter parameters for UI display
	parameters.addParameter(STR16("Input Meter L"), nullptr, 0, 0,
		Vst::ParameterInfo::kIsReadOnly, kInputMeterL);
	parameters.addParameter(STR16("Input Meter R"), nullptr, 0, 0,
		Vst::ParameterInfo::kIsReadOnly, kInputMeterR);
	parameters.addParameter(STR16("Output Meter L"), nullptr, 0, 0,
		Vst::ParameterInfo::kIsReadOnly, kOutputMeterL);
	parameters.addParameter(STR16("Output Meter R"), nullptr, 0, 0,
		Vst::ParameterInfo::kIsReadOnly, kOutputMeterR);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorController::terminate ()
{
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorController::setComponentState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	int32 savedReverbMode = 0;
	if (streamer.readInt32(savedReverbMode) == kResultTrue)
	{
		currentReverbMode = savedReverbMode;
		setParamNormalized(kReverbModeParam, savedReverbMode / 4.0);
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorController::setState (IBStream* state)
{
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorController::getState (IBStream* state)
{
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WetReverbProcessorController::notify (Vst::IMessage* message)
{
	if (!message)
		return kInvalidArgument;

	if (strcmp(message->getMessageID(), kMeterDataMessage) == 0)
	{
		const void* data = nullptr;
		uint32 size = 0;

		if (message->getAttributes()->getBinary("data", data, size) == kResultOk)
		{
			if (size == 4 * sizeof(float))
			{
				const float* meterData = static_cast<const float*>(data);

				setParamNormalized(kInputMeterL, meterData[0]);
				setParamNormalized(kInputMeterR, meterData[1]);
				setParamNormalized(kOutputMeterL, meterData[2]);
				setParamNormalized(kOutputMeterR, meterData[3]);

				return kResultOk;
			}
		}
	}

	return EditControllerEx1::notify(message);
}

//------------------------------------------------------------------------
void WetReverbProcessorController::setReverbModeFromUI(int index)
{
	if (index >= 0 && index <= 4)
	{
		currentReverbMode = index;
		Vst::ParamValue normalizedValue = index / 4.0;
		setParamNormalized(kReverbModeParam, normalizedValue);

		if (componentHandler)
		{
			componentHandler->beginEdit(kReverbModeParam);
			componentHandler->performEdit(kReverbModeParam, normalizedValue);
			componentHandler->endEdit(kReverbModeParam);
		}
	}
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API WetReverbProcessorController::createView (FIDString name)
{
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		auto* view = new VSTGUI::VST3Editor (this, "view", "wetreverbeditor.uidesc");
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
// ReverbButtonController Implementation
//------------------------------------------------------------------------
ReverbButtonController::ReverbButtonController(VSTGUI::IController* parentController,
                                               WetReverbProcessorController* mainCtrl)
    : DelegationController(parentController)
    , mainController(mainCtrl)
{
}

//------------------------------------------------------------------------
VSTGUI::CView* ReverbButtonController::createView(const VSTGUI::UIAttributes& attributes,
                                                   const VSTGUI::IUIDescription* description)
{
    return DelegationController::createView(attributes, description);
}

//------------------------------------------------------------------------
VSTGUI::CView* ReverbButtonController::verifyView(VSTGUI::CView* view,
                                                   const VSTGUI::UIAttributes& attributes,
                                                   const VSTGUI::IUIDescription* description)
{
    return DelegationController::verifyView(view, attributes, description);
}

//------------------------------------------------------------------------
void ReverbButtonController::valueChanged(VSTGUI::CControl* control)
{
    DelegationController::valueChanged(control);
}

//------------------------------------------------------------------------
void ReverbButtonController::updateLEDIndicators(int selectedIndex)
{
}

//------------------------------------------------------------------------
} // namespace Yonie
