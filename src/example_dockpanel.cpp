/*
    src/example_dockpanel.cpp -- Example demonstrating DockablePanel widget
    and fixed side/top/bottom panels with TabWidget integration.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/dockablepanel.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/tabwidget.h>
#include <nanogui/graph.h>

#include <iostream>
#include <cmath>

using std::cout;
using std::cerr;
using std::endl;

/**
 * \class DockPanelExample
 *
 * \brief Demonstrates the use of DockablePanel for creating fixed side panels.
 *
 * This example shows:
 * - Left panel with tabs (View, Navigate, Stats)
 * - Right panel with tabs (Add, Edit) - toggleable visibility
 * - Top bar for mode switching
 * - Bottom bar for status display
 * - A 3D viewport in the center (simulated with rotating triangles)
 * - Proper resize handling when the window is resized
 */
class DockPanelExample : public nanogui::Screen
{
public:
    DockPanelExample()
        : nanogui::Screen(
              Eigen::Vector2i(1280, 800),
              "DockablePanel Example",
              true /* resizable */)
    {
        using namespace nanogui;

        // ====================================================================
        // TOP BAR - Mode selector and quick stats
        // ====================================================================
        mTopBar = new DockablePanel(this, "", DockPosition::Top);
        mTopBar->setFixedHeight(35);
        mTopBar->setDockMargin(0);
        mTopBar->setCollapsible(false);  // Top bar should not be collapsible
        mTopBar->setLayout(new BoxLayout(
            Orientation::Horizontal, Alignment::Middle, 8, 15));

        // Mode selector
        mTopBar->add<Label>("Mode:", "sans-bold");
        mModeCombo = mTopBar->add<ComboBox>(
            std::vector<std::string>{"Simulation", "World Edit"});
        mModeCombo->setFixedWidth(120);
        mModeCombo->setCallback(
            [this](int index)
            {
                // Show edit panel in WorldEdit mode
                mRightPanel->setVisible(index == 1);
                performLayout();
            });

        // Separator
        mTopBar->add<Label>("  |  ");

        // Quick stats
        mTimeLabel = mTopBar->add<Label>("Time: 00:00:00");
        mTopBar->add<Label>("  ");
        mFpsLabel = mTopBar->add<Label>("FPS: 60");
        mTopBar->add<Label>("  ");
        mCpuLabel = mTopBar->add<Label>("CPU: 5%");

        // Spacer (push remaining items to the right)
        auto* spacer = mTopBar->add<Widget>();
        spacer->setFixedWidth(400);

        // Right-aligned buttons
        auto* btnMinAll = mTopBar->add<Button>("", ENTYPO_ICON_ALIGN_BOTTOM);
        btnMinAll->setTooltip("Minimize all panels");
        btnMinAll->setCallback(
            [this]()
            {
                mLeftPanel->setCollapsed(true);
                if (mRightPanel->visible())
                {
                    mRightPanel->setCollapsed(true);
                }
            });

        auto* btnMaxAll = mTopBar->add<Button>("", ENTYPO_ICON_BROWSER);
        btnMaxAll->setTooltip("Restore all panels");
        btnMaxAll->setCallback(
            [this]()
            {
                mLeftPanel->setCollapsed(false);
                if (mRightPanel->visible())
                {
                    mRightPanel->setCollapsed(false);
                }
            });

        // ====================================================================
        // LEFT PANEL - View, Navigate, Stats tabs
        // ====================================================================
        mLeftPanel = new DockablePanel(this, "Tools", DockPosition::Left);
        mLeftPanel->setFixedWidth(280);
        mLeftPanel->setDockOffset(40);     // Leave room for top bar
        mLeftPanel->setDockOffsetEnd(30);  // Leave room for bottom bar
        mLeftPanel->setSemiTransparent(true);
        mLeftPanel->setLayout(new BoxLayout(
            Orientation::Vertical, Alignment::Fill, 5, 5));

        // Create tab widget
        auto* leftTabs = mLeftPanel->add<TabWidget>();

        // ----- View Tab -----
        auto* viewTab = leftTabs->createTab("View");
        viewTab->setLayout(new GroupLayout(10, 4, 8, 10));

        viewTab->add<Label>("Camera", "sans-bold");

        viewTab->add<Label>("Follow:");
        auto* followCombo = viewTab->add<ComboBox>(
            std::vector<std::string>{"(none)", "Vehicle 1", "Vehicle 2", "Robot 1"});
        followCombo->setFixedWidth(150);

        auto* orthoCheck = viewTab->add<CheckBox>("Orthogonal view");
        orthoCheck->setChecked(false);

        viewTab->add<Label>("Visualization", "sans-bold");

        viewTab->add<CheckBox>("Enable shadows");
        viewTab->add<CheckBox>("Show forces");
        viewTab->add<CheckBox>("Show sensor points");
        viewTab->add<CheckBox>("Show collision shapes");

        viewTab->add<Label>("Lighting", "sans-bold");

        viewTab->add<Label>("Azimuth:");
        auto* azimuthSlider = viewTab->add<Slider>();
        azimuthSlider->setRange({-3.14159f, 3.14159f});
        azimuthSlider->setValue(0.5f);

        viewTab->add<Label>("Elevation:");
        auto* elevSlider = viewTab->add<Slider>();
        elevSlider->setRange({0.0f, 1.57f});
        elevSlider->setValue(0.8f);

        // ----- Navigate Tab -----
        auto* navTab = leftTabs->createTab("Navigate");
        navTab->setLayout(new GroupLayout(10, 4, 8, 10));

        navTab->add<Label>("Click-to-Navigate", "sans-bold");

        navTab->add<Label>("Target Vehicle:");
        auto* navVehCombo = navTab->add<ComboBox>(
            std::vector<std::string>{"(select)", "Vehicle 1", "Vehicle 2", "Robot 1"});
        navVehCombo->setFixedWidth(150);

        navTab->add<Label>("Click on ground to set target", "sans");

        navTab->add<Label>("Current Target", "sans-bold");
        navTab->add<Label>("Position: (none)");
        navTab->add<Label>("Distance: -");
        navTab->add<Label>("ETA: -");
        navTab->add<Label>("Status: Inactive");

        navTab->add<Label>("Speed Control", "sans-bold");
        navTab->add<Label>("Max Speed:");
        auto* speedSlider = navTab->add<Slider>();
        speedSlider->setRange({0.1f, 3.0f});
        speedSlider->setValue(1.0f);

        navTab->add<CheckBox>("Show path");
        navTab->add<CheckBox>("Show target marker");

        auto* clearBtn = navTab->add<Button>("Clear Target");
        clearBtn->setCallback([]() { cout << "Target cleared" << endl; });

        // ----- Stats Tab -----
        auto* statsTab = leftTabs->createTab("Stats");
        statsTab->setLayout(new GroupLayout(10, 4, 8, 10));

        statsTab->add<Label>("Simulation", "sans-bold");
        statsTab->add<Label>("Time: 00:01:23.456");
        statsTab->add<Label>("CPU Usage: 12.5%");
        statsTab->add<Label>("Sim Rate: 0.98x realtime");
        statsTab->add<Label>("Physics: 1000 Hz");
        statsTab->add<Label>("Render: 60 FPS");

        statsTab->add<Label>("Selected Object", "sans-bold");
        statsTab->add<Label>("(none)");
        statsTab->add<Label>("Position: -");
        statsTab->add<Label>("Velocity: -");

        statsTab->add<Label>("Sensor Rates", "sans-bold");

        // Scrollable sensor list
        auto* sensorScroll = statsTab->add<VScrollPanel>();
        sensorScroll->setFixedHeight(150);
        auto* sensorList = sensorScroll->add<Widget>();
        sensorList->setLayout(new BoxLayout(
            Orientation::Vertical, Alignment::Fill, 2, 2));

        sensorList->add<Label>("lidar_front: 10.0/10.0 Hz");
        sensorList->add<Label>("camera_rgb: 29.8/30.0 Hz");
        sensorList->add<Label>("imu: 99.5/100.0 Hz");
        sensorList->add<Label>("gps: 4.9/5.0 Hz");
        sensorList->add<Label>("depth_cam: 14.8/15.0 Hz");

        leftTabs->setActiveTab(0);

        // ====================================================================
        // RIGHT PANEL - Add, Edit tabs (for World Edit mode)
        // ====================================================================
        mRightPanel = new DockablePanel(this, "Editor", DockPosition::Right);
        mRightPanel->setFixedWidth(290);
        mRightPanel->setDockOffset(40);     // Leave room for top bar
        mRightPanel->setDockOffsetEnd(30);  // Leave room for bottom bar
        mRightPanel->setSemiTransparent(true);
        mRightPanel->setVisible(false);  // Hidden initially (Simulation mode)
        mRightPanel->setLayout(new BoxLayout(
            Orientation::Vertical, Alignment::Fill, 5, 5));

        auto* rightTabs = mRightPanel->add<TabWidget>();

        // ----- Add Tab -----
        auto* addTab = rightTabs->createTab("Add");
        addTab->setLayout(new GroupLayout(10, 4, 8, 10));

        addTab->add<Label>("Add Elements", "sans-bold");

        // Tool buttons in a grid
        auto* toolGrid = addTab->add<Widget>();
        toolGrid->setLayout(new GridLayout(
            Orientation::Horizontal, 3, Alignment::Fill, 2, 2));

        auto createToolBtn = [toolGrid](const std::string& label, int icon)
            -> Button*
        {
            auto* btn = toolGrid->add<Button>(label, icon);
            btn->setFlags(Button::RadioButton);
            btn->setFixedSize(Vector2i(80, 30));
            return btn;
        };

        auto* btnWall = createToolBtn("Wall", ENTYPO_ICON_MINUS);
        btnWall->setPushed(true);
        createToolBtn("Box", ENTYPO_ICON_BOX);
        createToolBtn("Cylinder", ENTYPO_ICON_CIRCLE);
        createToolBtn("Sphere", ENTYPO_ICON_CONTROLLER_RECORD);
        createToolBtn("Ramp", ENTYPO_ICON_TRIANGLE_UP);
        createToolBtn("Door", ENTYPO_ICON_LOGIN);

        addTab->add<Label>("Wall Properties", "sans-bold");

        auto* wallProps = addTab->add<Widget>();
        wallProps->setLayout(new GridLayout(
            Orientation::Horizontal, 2, Alignment::Fill, 2, 2));

        wallProps->add<Label>("Height:");
        auto* heightBox = wallProps->add<TextBox>("2.5");
        heightBox->setEditable(true);
        heightBox->setUnits("m");
        heightBox->setFixedWidth(80);

        wallProps->add<Label>("Thickness:");
        auto* thickBox = wallProps->add<TextBox>("0.15");
        thickBox->setEditable(true);
        thickBox->setUnits("m");
        thickBox->setFixedWidth(80);

        addTab->add<Label>("Options", "sans-bold");
        auto* snapCheck = addTab->add<CheckBox>("Snap to grid (0.5m)");
        snapCheck->setChecked(true);

        // ----- Edit Tab -----
        auto* editTab = rightTabs->createTab("Edit");
        editTab->setLayout(new GroupLayout(10, 4, 8, 10));

        editTab->add<Label>("Selected Object", "sans-bold");
        editTab->add<Label>("(none selected)");

        editTab->add<Label>("Position", "sans-bold");

        auto* posGrid = editTab->add<Widget>();
        posGrid->setLayout(new GridLayout(
            Orientation::Horizontal, 2, Alignment::Fill, 2, 2));

        posGrid->add<Label>("X:");
        auto* posX = posGrid->add<TextBox>("0.00");
        posX->setEditable(true);
        posX->setFixedWidth(80);

        posGrid->add<Label>("Y:");
        auto* posY = posGrid->add<TextBox>("0.00");
        posY->setEditable(true);
        posY->setFixedWidth(80);

        posGrid->add<Label>("Z:");
        auto* posZ = posGrid->add<TextBox>("0.00");
        posZ->setEditable(true);
        posZ->setFixedWidth(80);

        editTab->add<Label>("Rotation", "sans-bold");

        editTab->add<Label>("Yaw:");
        auto* yawSlider = editTab->add<Slider>();
        yawSlider->setRange({-3.14159f, 3.14159f});

        editTab->add<Label>("Actions", "sans-bold");

        auto* actionBtns = editTab->add<Widget>();
        actionBtns->setLayout(new BoxLayout(
            Orientation::Horizontal, Alignment::Fill, 2, 4));

        actionBtns->add<Button>("Delete", ENTYPO_ICON_TRASH);
        actionBtns->add<Button>("Duplicate", ENTYPO_ICON_DOCUMENTS);

        editTab->add<Label>("History", "sans-bold");

        auto* histBtns = editTab->add<Widget>();
        histBtns->setLayout(new BoxLayout(
            Orientation::Horizontal, Alignment::Fill, 2, 4));

        histBtns->add<Button>("Undo", ENTYPO_ICON_CCW);
        histBtns->add<Button>("Redo", ENTYPO_ICON_CW);

        auto* exportBtn = editTab->add<Button>("Export World XML...", ENTYPO_ICON_EXPORT);
        exportBtn->setCallback([]() { cout << "Export clicked" << endl; });

        rightTabs->setActiveTab(0);

        // ====================================================================
        // BOTTOM BAR - Status display
        // ====================================================================
        mBottomBar = new DockablePanel(this, "", DockPosition::Bottom);
        mBottomBar->setFixedHeight(25);
        mBottomBar->setDockMargin(0);
        mBottomBar->setCollapsible(false);
        mBottomBar->setLayout(new BoxLayout(
            Orientation::Horizontal, Alignment::Middle, 8, 20));

        mMouseLabel = mBottomBar->add<Label>("Mouse: (0.00, 0.00)");
        mBottomBar->add<Label>("  |  ");
        mSelectedLabel = mBottomBar->add<Label>("Selected: (none)");
        mBottomBar->add<Label>("  |  ");
        mStatusLabel = mBottomBar->add<Label>("Ready - Resize the window to test panel repositioning!");

        // ====================================================================
        // FLOATING WINDOW - To demonstrate mixing docked and floating
        // ====================================================================
        auto* floatingWin = new Window(this, "Floating Window");
        floatingWin->setPosition(Vector2i(400, 200));
        floatingWin->setLayout(new GroupLayout());

        floatingWin->add<Label>("This is a regular draggable window.");
        floatingWin->add<Label>("It can coexist with docked panels.");
        floatingWin->add<Label>("");
        floatingWin->add<Label>("Try resizing the main window!");
        floatingWin->add<Label>("The docked panels will follow.");

        auto* graphWidget = floatingWin->add<Graph>("CPU History");
        graphWidget->setFixedHeight(80);
        graphWidget->setHeader("15%");
        graphWidget->setFooter("Avg: 8%");
        auto& graphValues = graphWidget->values();
        graphValues.resize(50);
        for (size_t i = 0; i < 50; ++i)
        {
            graphValues[i] = 0.1f + 0.15f * std::sin(
                static_cast<float>(i) * 0.3f);
        }

        // ====================================================================
        // Initialize OpenGL shader for background
        // ====================================================================
        performLayout();

        mShader.init(
            "background_shader",

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 modelViewProj;\n"
            "in vec3 position;\n"
            "in vec3 color;\n"
            "out vec3 frag_color;\n"
            "void main() {\n"
            "    frag_color = color;\n"
            "    gl_Position = modelViewProj * vec4(position, 1.0);\n"
            "}",

            /* Fragment shader */
            "#version 330\n"
            "in vec3 frag_color;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "    color = vec4(frag_color, 1.0);\n"
            "}");

        // Simple colored triangle
        nanogui::MatrixXu indices(3, 1);
        indices.col(0) << 0, 1, 2;

        nanogui::MatrixXf positions(3, 3);
        positions.col(0) << -0.5f, -0.5f, 0.0f;
        positions.col(1) << 0.5f, -0.5f, 0.0f;
        positions.col(2) << 0.0f, 0.5f, 0.0f;

        nanogui::MatrixXf colors(3, 3);
        colors.col(0) << 1.0f, 0.0f, 0.0f;
        colors.col(1) << 0.0f, 1.0f, 0.0f;
        colors.col(2) << 0.0f, 0.0f, 1.0f;

        mShader.bind();
        mShader.uploadIndices(indices);
        mShader.uploadAttrib("position", positions);
        mShader.uploadAttrib("color", colors);
    }

    ~DockPanelExample()
    {
        mShader.free();
    }

    virtual void draw(NVGcontext* ctx) override
    {
        // Update dynamic labels
        float time = static_cast<float>(glfwGetTime());
        int hours = static_cast<int>(time / 3600.0f) % 24;
        int mins = static_cast<int>(std::fmod(time, 3600.0f) / 60.0f);
        int secs = static_cast<int>(std::fmod(time, 60.0f));

        char timeStr[32];
        snprintf(timeStr, sizeof(timeStr), "Time: %02d:%02d:%02d", hours, mins, secs);
        mTimeLabel->setCaption(timeStr);

        // Simulated FPS and CPU
        int fps = 58 + static_cast<int>(std::sin(time * 0.5f) * 2.0f);
        int cpu = 5 + static_cast<int>(std::sin(time * 0.3f) * 3.0f);

        char fpsStr[16];
        snprintf(fpsStr, sizeof(fpsStr), "FPS: %d", fps);
        mFpsLabel->setCaption(fpsStr);

        char cpuStr[16];
        snprintf(cpuStr, sizeof(cpuStr), "CPU: %d%%", cpu);
        mCpuLabel->setCaption(cpuStr);

        // Show current window size in status
        char sizeStr[64];
        snprintf(sizeStr, sizeof(sizeStr),
                 "Window: %dx%d", mSize.x(), mSize.y());
        mStatusLabel->setCaption(sizeStr);

        Screen::draw(ctx);
    }

    virtual void drawContents() override
    {
        using namespace nanogui;

        // Draw rotating triangle in the background
        mShader.bind();

        Matrix4f mvp;
        mvp.setIdentity();
        float angle = static_cast<float>(glfwGetTime()) * 0.5f;
        mvp.topLeftCorner<3, 3>() = Matrix3f(
            Eigen::AngleAxisf(angle, Vector3f::UnitZ())) * 0.3f;
        mvp.row(0) *= static_cast<float>(mSize.y()) /
                      static_cast<float>(mSize.x());

        mShader.setUniform("modelViewProj", mvp);
        mShader.drawIndexed(GL_TRIANGLES, 0, 1);
    }

    virtual bool keyboardEvent(
        int key, int scancode, int action, int modifiers) override
    {
        if (Screen::keyboardEvent(key, scancode, action, modifiers))
        {
            return true;
        }
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            setVisible(false);
            return true;
        }
        // Toggle right panel with 'E' key
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            mRightPanel->setVisible(!mRightPanel->visible());
            if (mRightPanel->visible())
            {
                mModeCombo->setSelectedIndex(1);
            }
            else
            {
                mModeCombo->setSelectedIndex(0);
            }
            performLayout();
            return true;
        }
        // Toggle panel collapse with '[' and ']'
        if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS)
        {
            mLeftPanel->toggleCollapsed();
            return true;
        }
        if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS)
        {
            if (mRightPanel->visible())
            {
                mRightPanel->toggleCollapsed();
            }
            return true;
        }
        return false;
    }

private:
    nanogui::DockablePanel* mLeftPanel = nullptr;
    nanogui::DockablePanel* mRightPanel = nullptr;
    nanogui::DockablePanel* mTopBar = nullptr;
    nanogui::DockablePanel* mBottomBar = nullptr;

    nanogui::ComboBox* mModeCombo = nullptr;
    nanogui::Label* mTimeLabel = nullptr;
    nanogui::Label* mFpsLabel = nullptr;
    nanogui::Label* mCpuLabel = nullptr;
    nanogui::Label* mMouseLabel = nullptr;
    nanogui::Label* mSelectedLabel = nullptr;
    nanogui::Label* mStatusLabel = nullptr;

    nanogui::GLShader mShader;
};

int main(int /* argc */, char** /* argv */)
{
    try
    {
        nanogui::init();

        /* scoped variables */
        {
            nanogui::ref<DockPanelExample> app = new DockPanelExample();
            app->drawAll();
            app->setVisible(true);
            nanogui::mainloop();
        }

        nanogui::shutdown();
    }
    catch (const std::runtime_error& e)
    {
        std::string error_msg = std::string("Caught a fatal error: ") +
                                std::string(e.what());
#if defined(_WIN32)
        MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
        std::cerr << error_msg << endl;
#endif
        return -1;
    }

    return 0;
}