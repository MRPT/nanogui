/*
    nanogui/dockablepanel.h -- Panel widget that can dock to screen edges

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/** \file */

#pragma once

#include <nanogui/window.h>

NAMESPACE_BEGIN(nanogui)

/**
 * \enum DockPosition dockablepanel.h nanogui/dockablepanel.h
 *
 * \brief Specifies where a DockablePanel should be anchored.
 */
enum class DockPosition
{
    Left,    ///< Dock to the left edge of the screen
    Right,   ///< Dock to the right edge of the screen
    Top,     ///< Dock to the top edge of the screen
    Bottom,  ///< Dock to the bottom edge of the screen
    Float    ///< Not docked, behaves like a regular Window
};

/**
 * \class DockablePanel dockablepanel.h nanogui/dockablepanel.h
 *
 * \brief A panel that can be docked to screen edges.
 *
 * DockablePanel extends Window with the ability to anchor itself to
 * the edges of the screen. When docked, the panel:
 * - Cannot be dragged (unless in Float mode)
 * - Automatically repositions on window resize
 * - Can optionally be collapsible (minimize to edge)
 *
 * The panel automatically registers itself with the parent Screen to
 * receive resize notifications and update its position accordingly.
 *
 * Example usage:
 * \code
 *     auto panel = new DockablePanel(screen, "Tools", DockPosition::Left);
 *     panel->setFixedWidth(280);
 *     panel->setLayout(new BoxLayout(Orientation::Vertical));
 *     new Button(panel, "Tool 1");
 * \endcode
 */
class NANOGUI_EXPORT DockablePanel : public Window
{
public:
    /**
     * \brief Construct a dockable panel.
     *
     * \param parent
     *     The parent widget (typically a Screen).
     *
     * \param title
     *     The panel title.
     *
     * \param position
     *     Where to dock the panel (default: DockPosition::Left).
     */
    DockablePanel(
        Widget* parent,
        const std::string& title = "Panel",
        DockPosition position = DockPosition::Left);

    /// Destructor - unregisters from parent Screen
    virtual ~DockablePanel();

    /// Get the current dock position
    DockPosition dockPosition() const { return mDockPosition; }

    /// Set the dock position and update layout
    void setDockPosition(DockPosition position);

    /// Get whether the panel is currently collapsed
    bool collapsed() const { return mCollapsed; }

    /// Set the collapsed state
    void setCollapsed(bool collapsed);

    /// Toggle the collapsed state
    void toggleCollapsed() { setCollapsed(!mCollapsed); }

    /// Get whether the panel can be collapsed
    bool collapsible() const { return mCollapsible; }

    /// Set whether the panel can be collapsed
    void setCollapsible(bool collapsible) { mCollapsible = collapsible; }

    /// Get the margin from the screen edge when docked
    int dockMargin() const { return mDockMargin; }

    /// Set the margin from the screen edge when docked
    void setDockMargin(int margin) { mDockMargin = margin; }

    /// Get the offset from adjacent panels or screen corners
    int dockOffset() const { return mDockOffset; }

    /**
     * \brief Set the offset (useful for multiple panels, or leaving room for
     *        top/bottom bars).
     *
     * For Left/Right panels: offset from top of screen
     * For Top/Bottom panels: offset from left of screen
     */
    void setDockOffset(int offset) { mDockOffset = offset; }

    /**
     * \brief Set the secondary offset (from the opposite edge).
     *
     * For Left/Right panels: offset from bottom of screen
     * For Top/Bottom panels: offset from right of screen
     *
     * If set to 0, the panel will use its fixed size or stretch.
     */
    int dockOffsetEnd() const { return mDockOffsetEnd; }
    void setDockOffsetEnd(int offset) { mDockOffsetEnd = offset; }

    /// Get the size when collapsed (width for left/right, height for top/bottom)
    int collapsedSize() const { return mCollapsedSize; }

    /// Set the size when collapsed
    void setCollapsedSize(int size) { mCollapsedSize = size; }

    /// Get whether the panel draws a semi-transparent background
    bool semiTransparent() const { return mSemiTransparent; }

    /// Set whether the panel draws a semi-transparent background
    void setSemiTransparent(bool transparent) { mSemiTransparent = transparent; }

    /// Get the semi-transparent alpha value (0-255)
    int semiTransparentAlpha() const { return mSemiTransparentAlpha; }

    /// Set the semi-transparent alpha value (0-255)
    void setSemiTransparentAlpha(int alpha) { mSemiTransparentAlpha = alpha; }

    /// Override to update position when parent size changes
    virtual void performLayout(NVGcontext* ctx) override;

    /// Override to draw with optional semi-transparency and collapse state
    virtual void draw(NVGcontext* ctx) override;

    /// Override to handle collapse button
    virtual bool mouseButtonEvent(
        const Vector2i& p, int button, bool down, int modifiers) override;

    /**
     * \brief Called when the parent Screen is resized.
     *
     * This method is called automatically by the Screen when registered.
     * It updates the panel's position and size based on the new screen size.
     *
     * \param newSize The new screen size.
     */
    void onParentResized(const Vector2i& newSize);

protected:
    /// Update the panel position based on dock position and parent size
    void updateDockPosition();

    /// Draw the collapse/expand button
    void drawCollapseButton(NVGcontext* ctx);

    /// Register this panel with the parent Screen for resize notifications
    void registerWithScreen();

    /// Unregister this panel from the parent Screen
    void unregisterFromScreen();

    DockPosition mDockPosition;
    bool mCollapsed = false;
    bool mCollapsible = true;
    int mDockMargin = 0;       ///< Margin from screen edge
    int mDockOffset = 0;       ///< Offset from top/left of dock area
    int mDockOffsetEnd = 0;    ///< Offset from bottom/right of dock area
    int mCollapsedSize = 25;   ///< Size when collapsed
    bool mSemiTransparent = false;
    int mSemiTransparentAlpha = 200;  ///< Alpha value for semi-transparent mode
    int mExpandedSize = 0;     ///< Cached size before collapse
    bool mRegisteredWithScreen = false;

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

NAMESPACE_END(nanogui)