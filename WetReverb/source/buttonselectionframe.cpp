//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#include "buttonselectionframe.h"
#include "vstgui/lib/cgraphicspath.h"

using namespace VSTGUI;

namespace Yonie {

// Hardcoded button X positions (relative to view left at x=54)
// 5 reverb modes: Room, Plate, Hall, Cathedral, Cosmos
// Absolute X positions: 54, 173, 294, 415, 537
// Each button is 110x110 pixels
static const CCoord kButtonOffsets[5] = { 0, 119, 240, 361, 483 };
static const CCoord kButtonWidth = 110;
static const int kNumButtons = 5;

//------------------------------------------------------------------------
// ButtonSelectionFrame Implementation
//------------------------------------------------------------------------
ButtonSelectionFrame::ButtonSelectionFrame(const CRect& size)
    : CControl(size, nullptr, -1)
{
    setMouseEnabled(false);  // Click through to button beneath
    setWantsFocus(false);
}

//------------------------------------------------------------------------
void ButtonSelectionFrame::draw(CDrawContext* context)
{
    CRect viewSize = getViewSize();
    
    // Calculate which button is selected (0 to 4)
    float val = getValueNormalized();
    int selectedIndex = static_cast<int>(val * 4.0f + 0.5f);
    if (selectedIndex < 0) selectedIndex = 0;
    if (selectedIndex > 4) selectedIndex = 4;
    
    // LED radius settings
    CCoord ledRadius = 8;      // Main LED radius
    CCoord glowRadius = 14;    // Glow radius for selected button
    
    context->setDrawMode(kAntiAliasing);
    
    // Draw LED indicator for each button
    for (int i = 0; i < kNumButtons; i++)
    {
        // Calculate button position and center
        CCoord buttonX = viewSize.left + kButtonOffsets[i];
        CCoord buttonCenterX = buttonX + kButtonWidth / 2;
        CCoord buttonCenterY = viewSize.top + 35;  // Upper half of 110px tall button
        
        bool isSelected = (i == selectedIndex);
        
        // Draw glow effect for selected button
        if (isSelected)
        {
            CRect glowRect(buttonCenterX - glowRadius, buttonCenterY - glowRadius,
                          buttonCenterX + glowRadius, buttonCenterY + glowRadius);
            auto glowPath = owned(context->createGraphicsPath());
            if (glowPath)
            {
                glowPath->addEllipse(glowRect);
                CColor glowColor = ledOnColor;
                glowColor.alpha = 80;
                context->setFillColor(glowColor);
                context->drawGraphicsPath(glowPath, CDrawContext::kPathFilled);
            }
        }
        
        // Draw main LED circle using CGraphicsPath
        CRect ledRect(buttonCenterX - ledRadius, buttonCenterY - ledRadius,
                     buttonCenterX + ledRadius, buttonCenterY + ledRadius);
        auto ledPath = owned(context->createGraphicsPath());
        if (ledPath)
        {
            ledPath->addEllipse(ledRect);
            CColor color = isSelected ? ledOnColor : ledOffColor;
            context->setFillColor(color);
            context->drawGraphicsPath(ledPath, CDrawContext::kPathFilled);
        }
    }
    
    setDirty(false);
}

//------------------------------------------------------------------------
void ButtonSelectionFrame::setValueNormalized(float val)
{
    if (val != getValueNormalized())
    {
        CControl::setValueNormalized(val);
        invalid();  // Request redraw
    }
}

//------------------------------------------------------------------------
} // namespace Yonie