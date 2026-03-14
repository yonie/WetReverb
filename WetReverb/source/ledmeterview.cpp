//------------------------------------------------------------------------
// Copyright(c) 2026 Yonie.
//------------------------------------------------------------------------

#include "ledmeterview.h"

using namespace VSTGUI;

namespace Yonie {

//------------------------------------------------------------------------
// LEDMeterView Implementation
//------------------------------------------------------------------------
LEDMeterView::LEDMeterView(const CRect& size)
    : CControl(size, nullptr, -1)
{
    setWantsFocus(false);
}

//------------------------------------------------------------------------
void LEDMeterView::draw(CDrawContext* context)
{
    // Get normalized value (0.0 to 1.0)
    float level = getValueNormalized();
    
    // Calculate how many segments should be lit
    int litCount = static_cast<int>(level * numSegments + 0.5f);
    if (litCount > numSegments) litCount = numSegments;
    if (litCount < 0) litCount = 0;
    
    // Draw each segment
    for (int i = 0; i < numSegments; i++)
    {
        CRect segRect = calculateSegmentRect(i);
        CColor color = (i < litCount) ? getLitColor(i) : getDarkColor(i);
        
        context->setFillColor(color);
        context->drawRect(segRect, kDrawFilled);
    }
    
    // Draw frame around entire meter (optional, subtle)
    // context->setFrameColor(CColor(60, 60, 60, 255));
    // context->setLineWidth(1);
    // context->drawRect(getViewSize(), kDrawStroked);
    
    setDirty(false);
}

//------------------------------------------------------------------------
void LEDMeterView::setValueNormalized(float val)
{
    if (val != getValueNormalized())
    {
        CControl::setValueNormalized(val);
        invalid();  // Request redraw
    }
}

//------------------------------------------------------------------------
CRect LEDMeterView::calculateSegmentRect(int segmentIndex) const
{
    CRect viewSize = getViewSize();
    CCoord totalWidth = viewSize.getWidth();
    CCoord totalHeight = viewSize.getHeight();
    
    if (isHorizontal)
    {
        // Horizontal meter: segments arranged left to right
        CCoord segmentWidth = (totalWidth - (numSegments - 1) * segmentGap) / numSegments;
        CCoord x = viewSize.left + segmentIndex * (segmentWidth + segmentGap);
        
        return CRect(x, viewSize.top, x + segmentWidth, viewSize.bottom);
    }
    else
    {
        // Vertical meter: segments arranged bottom to top
        CCoord segmentHeight = (totalHeight - (numSegments - 1) * segmentGap) / numSegments;
        CCoord y = viewSize.bottom - (segmentIndex + 1) * segmentHeight - segmentIndex * segmentGap;
        
        return CRect(viewSize.left, y, viewSize.right, y + segmentHeight);
    }
}

//------------------------------------------------------------------------
CColor LEDMeterView::getLitColor(int segmentIndex) const
{
    // 9 segments: 0-5 green, 6-7 yellow, 8 red
    if (segmentIndex < 6)  // Green zone (segments 0-5)
    {
        return greenLit;
    }
    else if (segmentIndex < 8)  // Yellow zone (segments 6-7)
    {
        return yellowLit;
    }
    else  // Red zone (segment 8)
    {
        return redLit;
    }
}

//------------------------------------------------------------------------
CColor LEDMeterView::getDarkColor(int segmentIndex) const
{
    // Same zones as lit colors, but darker
    if (segmentIndex < 6)  // Green zone
    {
        return greenDark;
    }
    else if (segmentIndex < 8)  // Yellow zone
    {
        return yellowDark;
    }
    else  // Red zone
    {
        return redDark;
    }
}

//------------------------------------------------------------------------
} // namespace Yonie