//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/uidescription/delegationcontroller.h"
#include "wetreverbcids.h"

namespace Yonie {

// Forward declarations
class ReverbButtonController;

//------------------------------------------------------------------------
//  WetReverbProcessorController
//------------------------------------------------------------------------
class WetReverbProcessorController : public Steinberg::Vst::EditControllerEx1
{
public:
//------------------------------------------------------------------------
	WetReverbProcessorController () = default;
	~WetReverbProcessorController () SMTG_OVERRIDE = default;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/)
	{
		return (Steinberg::Vst::IEditController*)new WetReverbProcessorController;
	}

	//--- from IPluginBase -----------------------------------------------
	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	//--- from EditController --------------------------------------------
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	
	//--- from IConnectionPoint (for messages from processor) ------------
	Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;
	
	// Get current reverb mode for UI updates
	int getCurrentReverbMode() const { return currentReverbMode; }
	void setReverbModeFromUI(int index);

	//---Interface---------
	DEFINE_INTERFACES
		// Here you can add more supported VST3 interfaces
		// DEF_INTERFACE (Vst::IXXX)
	END_DEFINE_INTERFACES (EditController)
    DELEGATE_REFCOUNT (EditController)

//------------------------------------------------------------------------
protected:
	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer;
	int currentReverbMode = 0;
	
	friend class ReverbButtonController;
};

//------------------------------------------------------------------------
// ReverbButtonController - Sub-controller for reverb button radio behavior
//------------------------------------------------------------------------
class ReverbButtonController : public VSTGUI::DelegationController
{
public:
    ReverbButtonController(VSTGUI::IController* parentController,
                           WetReverbProcessorController* mainController);
    ~ReverbButtonController() override = default;
    
    // IController overrides
    VSTGUI::CView* createView(const VSTGUI::UIAttributes& attributes,
                              const VSTGUI::IUIDescription* description) override;
    VSTGUI::CView* verifyView(VSTGUI::CView* view,
                              const VSTGUI::UIAttributes& attributes,
                              const VSTGUI::IUIDescription* description) override;
    
    // IControlListener (inherited from DelegationController)
    void valueChanged(VSTGUI::CControl* control) override;
    
    // Update LED indicators
    void updateLEDIndicators(int selectedIndex);
    
protected:
    WetReverbProcessorController* mainController = nullptr;
};

//------------------------------------------------------------------------
} // namespace Yonie