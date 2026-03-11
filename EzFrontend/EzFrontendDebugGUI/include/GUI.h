/**
 * @file GUI.h
 * @brief Singleton that owns the ImGui / DirectX 11 rendering loop and
 *        the set of debug views (editor, output, menu bar, …).
 *
 * Call Gui::get().initialize() to create the transparent overlay window,
 * then run the main render loop.  Views are registered as IView subclasses
 * and rendered each frame.
 */
#ifndef EZPACKER_GUI_H
#define EZPACKER_GUI_H

#include "EzFrontendDebugGUICommon.h"
#include "Views/Editor.h"
#include "Views/Output.h"
#include "Views/IView.h"
#include "Views/MainMenuBar.h"

class Gui
{
  public:
    /**
     * Returns the only instance of this class.
     * @return Gui &
     */
    static Gui &get();

    /**
     * Initializes the GUI creating a transparent window and setting ImGui to draw over it and creating a console window
     * to write log (if given logger is not nullptr).
     * @param logger
     */
    bool initialize(std::shared_ptr<Logger> logger);

    /**
     * Returns true if the Gui is initialized.
     * @return bool
     */
    bool isInitialized() const;

    /**
     * Uninitializes the GUI, destroying window gui and console window, returns true if succeeded.
     * @return bool
     */
    bool uninitialize();

    /**
     * Handles window resize event.
     * @param width
     * @param height
     */
    void onResize(int width, int height);

    /**
     * Executes the main loop of the GUI.
     */
    void loop();

  private:
    // Enforce singleton
    /**
     * Creates the GUI object with default values.
     */
    Gui();

    Gui(const Gui &copy) = default;

#ifdef _WIN32
    /**
     * Creates the D3D device for the current windows and returns true if succeeded.
     * @return bool
     */
    bool createDeviceD3D();

    /**
     * Creates the window to draw the GUI and returns true if succeeded.
     * @return bool
     */
    bool createGuiWindow();

    /**
     * Creates a render target for the given device and swapchain, returns true if succeeded.
     * @return bool
     */
    bool createRenderTarget();

    /**
     * Destroysd the D3D device for the current window and returns true if succeeded.
     * @return bool
     */
    bool destroyDeviceD3D();

    /**
     * Destroys the current GUI window and returns true if succeeded.
     * @return bool
     */
    bool destroyGuiWindow();

    /**
     * Destroys the render target and returns true if succeeded.
     * @return bool
     */
    bool destroyRenderTarget();

    /**
     * The WndProc callback that will pass input to GUI.
     * @param hwnd
     * @param code
     * @param msg
     * @param lparam
     * @return LRESULT
     */
    static LRESULT WndProc(HWND hwnd, UINT code, WPARAM msg, LPARAM lparam);

#elif defined(__linux__)

    bool createGlfwWindow();
    bool destroyGlfwWindow();

#endif

    /**
     * Attaches ImGui to the created window and D3D device and returns true if succeeded.
     * @return bool
     */
    bool createImGuiContext();

    /**
     * Destroys the current imgui context attached to the windows and d3d device and returns true if succeeded.
     * @return bool
     */
    bool destroyImGuiContext();

  private:
    bool m_initialized;
    ImVec2 m_windowPos;
    ImVec2 m_windowSize;

#ifdef _WIN32
    HWND m_guiWindow;
    ID3D11Device *m_pd3dDevice;
    ID3D11DeviceContext *m_pd3dDeviceContext;
    IDXGISwapChain *m_pSwapChain;
    ID3D11RenderTargetView *m_mainRenderTargetView;
#elif defined(__linux__)
    GLFWwindow *m_window;
#endif

    std::shared_ptr<Logger> m_logger;
};

#endif // EZPACKER_GUI_H
