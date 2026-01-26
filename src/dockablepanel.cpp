/*
    src/dockablepanel.cpp -- Panel widget that can dock to screen edges

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/dockablepanel.h>
#include <nanogui/entypo.h>
#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/theme.h>

#include <algorithm>
#include <map>

NAMESPACE_BEGIN(nanogui)

// ============================================================================
// Static registry for tracking DockablePanels per Screen
// This allows us to notify panels when the Screen is resized.
// ============================================================================
namespace
{
// Map from Screen pointer to list of DockablePanels attached to it
std::map<Screen*, std::vector<DockablePanel*>> sDockablePanels;

void registerPanel(Screen* screen, DockablePanel* panel)
{
    if (!screen || !panel)
    {
        return;
    }
    auto& panels = sDockablePanels[screen];
    if (std::find(panels.begin(), panels.end(), panel) == panels.end())
    {
        panels.push_back(panel);
    }
}

void unregisterPanel(Screen* screen, DockablePanel* panel)
{
    if (!screen)
    {
        return;
    }
    auto it = sDockablePanels.find(screen);
    if (it != sDockablePanels.end())
    {
        auto& panels = it->second;
        panels.erase(
            std::remove(panels.begin(), panels.end(), panel),
            panels.end());
        if (panels.empty())
        {
            sDockablePanels.erase(it);
        }
    }
}

}  // namespace

// ============================================================================
// Public function to notify all DockablePanels of a Screen resize.
// Call this from your Screen's resizeEvent() or resize callback.
// ============================================================================
void notifyDockablePanelsOfResize(Screen* screen, const Vector2i& newSize)
{
    auto it = sDockablePanels.find(screen);
    if (it != sDockablePanels.end())
    {
        for (auto* panel : it->second)
        {
            panel->onParentResized(newSize);
        }
    }
}

// ============================================================================
// DockablePanel Implementation
// ============================================================================

DockablePanel::DockablePanel(
    Widget* parent,
    const std::string& title,
    DockPosition position)
    : Window(parent, title), mDockPosition(position)
{
    // Docked panels are not draggable by default (unless floating)
    setDraggable(position == DockPosition::Float);

    // Register with parent Screen
    registerWithScreen();
}

DockablePanel::~DockablePanel()
{
    unregisterFromScreen();
}

void DockablePanel::registerWithScreen()
{
    if (mRegisteredWithScreen)
    {
        return;
    }

    // Find the parent Screen
    Widget* widget = parent();
    while (widget)
    {
        if (auto* screen = dynamic_cast<Screen*>(widget))
        {
            registerPanel(screen, this);
            mRegisteredWithScreen = true;

            // Install resize callback if not already present
            // We wrap any existing callback to also notify dockable panels
            auto existingCallback = screen->resizeCallback();
            screen->setResizeCallback(
                [screen, existingCallback](Vector2i size)
                {
                    // Call existing callback first
                    if (existingCallback)
                    {
                        existingCallback(size);
                    }
                    // Notify all dockable panels
                    notifyDockablePanelsOfResize(screen, size);
                });

            break;
        }
        widget = widget->parent();
    }
}

void DockablePanel::unregisterFromScreen()
{
    if (!mRegisteredWithScreen)
    {
        return;
    }

    Widget* widget = parent();
    while (widget)
    {
        if (auto* screen = dynamic_cast<Screen*>(widget))
        {
            unregisterPanel(screen, this);
            break;
        }
        widget = widget->parent();
    }
    mRegisteredWithScreen = false;
}

void DockablePanel::setDockPosition(DockPosition position)
{
    mDockPosition = position;
    setDraggable(position == DockPosition::Float);
    updateDockPosition();
}

void DockablePanel::setCollapsed(bool collapsed)
{
    if (mCollapsed == collapsed || !mCollapsible)
    {
        return;
    }

    if (collapsed)
    {
        // Save current size before collapsing
        if (mDockPosition == DockPosition::Left ||
            mDockPosition == DockPosition::Right)
        {
            mExpandedSize = mSize.x();
        }
        else
        {
            mExpandedSize = mSize.y();
        }
    }

    mCollapsed = collapsed;

    // Hide/show children when collapsed/expanded
    for (auto* child : mChildren)
    {
        // Keep the button panel visible if it exists
        if (child != mButtonPanel)
        {
            child->setVisible(!collapsed);
        }
    }

    updateDockPosition();
}

void DockablePanel::onParentResized(const Vector2i& newSize)
{
    (void)newSize;  // The parent size is retrieved in updateDockPosition
    updateDockPosition();
}

void DockablePanel::updateDockPosition()
{
    if (!parent())
    {
        return;
    }

    Vector2i parentSize = parent()->size();
    int w = mSize.x();
    int h = mSize.y();

    // Determine the size based on fixed size, collapse state, and dock position
    switch (mDockPosition)
    {
        case DockPosition::Left:
        case DockPosition::Right:
        {
            // Width: use fixedSize if set, otherwise keep current
            if (mFixedSize.x() > 0)
            {
                w = mCollapsed ? mCollapsedSize : mFixedSize.x();
            }
            else if (mCollapsed)
            {
                w = mCollapsedSize;
            }

            // Height: stretch from dockOffset to (parentHeight - dockOffsetEnd)
            if (mFixedSize.y() > 0)
            {
                h = mFixedSize.y();
            }
            else
            {
                h = parentSize.y() - mDockOffset - mDockOffsetEnd;
                if (h < 50)
                {
                    h = 50;  // Minimum height
                }
            }
            break;
        }

        case DockPosition::Top:
        case DockPosition::Bottom:
        {
            // Height: use fixedSize if set, otherwise keep current
            if (mFixedSize.y() > 0)
            {
                h = mCollapsed ? mCollapsedSize : mFixedSize.y();
            }
            else if (mCollapsed)
            {
                h = mCollapsedSize;
            }

            // Width: stretch from dockOffset to (parentWidth - dockOffsetEnd)
            if (mFixedSize.x() > 0)
            {
                w = mFixedSize.x();
            }
            else
            {
                w = parentSize.x() - mDockOffset - mDockOffsetEnd;
                if (w < 50)
                {
                    w = 50;  // Minimum width
                }
            }
            break;
        }

        case DockPosition::Float:
            // Don't modify size for floating panels
            return;
    }

    // Calculate position based on dock position
    int x = 0;
    int y = 0;

    switch (mDockPosition)
    {
        case DockPosition::Left:
            x = mDockMargin;
            y = mDockOffset;
            break;

        case DockPosition::Right:
            x = parentSize.x() - w - mDockMargin;
            y = mDockOffset;
            break;

        case DockPosition::Top:
            x = mDockOffset;
            y = mDockMargin;
            break;

        case DockPosition::Bottom:
            x = mDockOffset;
            y = parentSize.y() - h - mDockMargin;
            break;

        case DockPosition::Float:
            // Don't modify position for floating panels
            return;
    }

    // Apply position and size
    mPos = Vector2i(x, y);
    mSize = Vector2i(w, h);
}

void DockablePanel::performLayout(NVGcontext* ctx)
{
    // Update dock position first (handles parent resize)
    updateDockPosition();

    // Then perform normal layout for children
    Window::performLayout(ctx);
}

void DockablePanel::draw(NVGcontext* ctx)
{
    if (mSemiTransparent)
    {
        // Save current theme colors
        Color origUnfocused = mTheme->mWindowFillUnfocused;
        Color origFocused = mTheme->mWindowFillFocused;

        // Create semi-transparent colors
        float alpha = static_cast<float>(mSemiTransparentAlpha) / 255.0f;
        mTheme->mWindowFillUnfocused = Color(
            origUnfocused.r(), origUnfocused.g(), origUnfocused.b(),
            alpha * 0.85f);
        mTheme->mWindowFillFocused = Color(
            origFocused.r(), origFocused.g(), origFocused.b(),
            alpha);

        Window::draw(ctx);

        // Restore original colors
        mTheme->mWindowFillUnfocused = origUnfocused;
        mTheme->mWindowFillFocused = origFocused;
    }
    else
    {
        Window::draw(ctx);
    }

    // Draw collapse button if collapsible and docked
    if (mCollapsible && mDockPosition != DockPosition::Float)
    {
        drawCollapseButton(ctx);
    }
}

void DockablePanel::drawCollapseButton(NVGcontext* ctx)
{
    int hh = mTheme->mWindowHeaderHeight;
    float btnSize = static_cast<float>(hh - 8);
    float btnX = 0.0f;
    float btnY = static_cast<float>(mPos.y()) + 4.0f;

    // Position button based on dock position
    switch (mDockPosition)
    {
        case DockPosition::Left:
            btnX = static_cast<float>(mPos.x() + mSize.x()) - btnSize - 4.0f;
            break;
        case DockPosition::Right:
            btnX = static_cast<float>(mPos.x()) + 4.0f;
            break;
        case DockPosition::Top:
        case DockPosition::Bottom:
            btnX = static_cast<float>(mPos.x() + mSize.x()) - btnSize - 4.0f;
            break;
        default:
            return;
    }

    // Draw button background
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, btnX, btnY, btnSize, btnSize, 2.0f);
    nvgFillColor(ctx, Color(60, 60, 60, 200));
    nvgFill(ctx);

    // Draw collapse/expand icon
    nvgFontSize(ctx, btnSize * 0.8f);
    nvgFontFace(ctx, "icons");
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, Color(200, 200, 200, 255));

    int icon = 0;
    if (mCollapsed)
    {
        // Expand icons (pointing outward)
        switch (mDockPosition)
        {
            case DockPosition::Left:
                icon = ENTYPO_ICON_CHEVRON_RIGHT;
                break;
            case DockPosition::Right:
                icon = ENTYPO_ICON_CHEVRON_LEFT;
                break;
            case DockPosition::Top:
                icon = ENTYPO_ICON_CHEVRON_DOWN;
                break;
            case DockPosition::Bottom:
                icon = ENTYPO_ICON_CHEVRON_UP;
                break;
            default:
                break;
        }
    }
    else
    {
        // Collapse icons (pointing toward edge)
        switch (mDockPosition)
        {
            case DockPosition::Left:
                icon = ENTYPO_ICON_CHEVRON_LEFT;
                break;
            case DockPosition::Right:
                icon = ENTYPO_ICON_CHEVRON_RIGHT;
                break;
            case DockPosition::Top:
                icon = ENTYPO_ICON_CHEVRON_UP;
                break;
            case DockPosition::Bottom:
                icon = ENTYPO_ICON_CHEVRON_DOWN;
                break;
            default:
                break;
        }
    }

    // Convert icon to UTF-8 string
    char iconStr[8];
    int iconCodepoint = icon;
    if (iconCodepoint < 0x80)
    {
        iconStr[0] = static_cast<char>(iconCodepoint);
        iconStr[1] = '\0';
    }
    else if (iconCodepoint < 0x800)
    {
        iconStr[0] = static_cast<char>(0xC0 | (iconCodepoint >> 6));
        iconStr[1] = static_cast<char>(0x80 | (iconCodepoint & 0x3F));
        iconStr[2] = '\0';
    }
    else
    {
        iconStr[0] = static_cast<char>(0xE0 | (iconCodepoint >> 12));
        iconStr[1] = static_cast<char>(0x80 | ((iconCodepoint >> 6) & 0x3F));
        iconStr[2] = static_cast<char>(0x80 | (iconCodepoint & 0x3F));
        iconStr[3] = '\0';
    }

    nvgText(ctx, btnX + btnSize / 2.0f, btnY + btnSize / 2.0f, iconStr, nullptr);
}

bool DockablePanel::mouseButtonEvent(
    const Vector2i& p, int button, bool down, int modifiers)
{
    // Check if click is on collapse button
    if (mCollapsible && button == GLFW_MOUSE_BUTTON_1 && down &&
        mDockPosition != DockPosition::Float)
    {
        int hh = mTheme->mWindowHeaderHeight;
        float btnSize = static_cast<float>(hh - 8);
        float btnX = 0.0f;
        float btnY = static_cast<float>(mPos.y()) + 4.0f;

        switch (mDockPosition)
        {
            case DockPosition::Left:
                btnX = static_cast<float>(mPos.x() + mSize.x()) - btnSize - 4.0f;
                break;
            case DockPosition::Right:
                btnX = static_cast<float>(mPos.x()) + 4.0f;
                break;
            case DockPosition::Top:
            case DockPosition::Bottom:
                btnX = static_cast<float>(mPos.x() + mSize.x()) - btnSize - 4.0f;
                break;
            default:
                break;
        }

        // Check if click is within button bounds
        float px = static_cast<float>(p.x());
        float py = static_cast<float>(p.y());
        if (px >= btnX && px <= btnX + btnSize &&
            py >= btnY && py <= btnY + btnSize)
        {
            toggleCollapsed();
            return true;
        }
    }

    return Window::mouseButtonEvent(p, button, down, modifiers);
}

NAMESPACE_END(nanogui)