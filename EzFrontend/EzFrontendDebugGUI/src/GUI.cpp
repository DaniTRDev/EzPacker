#include "GUI.h"

Gui::Gui() :
    m_initialized(false), m_guiWindow(nullptr), m_pd3dDevice(nullptr), m_pd3dDeviceContext(nullptr),
    m_mainRenderTargetView(nullptr), m_windowPos(0, 0),
    m_windowSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)), m_pSwapChain(nullptr)
{
}

Gui &Gui::get()
{
    static Gui obj = {};
    return obj;
}

bool Gui::initialize(std::shared_ptr<Logger> logger)
{
    m_logger = std::move(logger);

    if (!createGuiWindow())
    {
        return false;
    }

    if (!createDeviceD3D())
    {
        return false;
    }

    if (!createRenderTarget())
    {
        return false;
    }

    if (!createImGuiContext())
    {
        return false;
    }

    m_initialized = true;
    return true;
}

bool Gui::isInitialized() const { return m_initialized; }

bool Gui::uninitialize()
{
    m_initialized = false;

    if (!destroyImGuiContext())
    {
        return false;
    }

    if (!destroyDeviceD3D())
    {
        return false;
    }

    if (!destroyGuiWindow())
    {
        return false;
    }

    return true;
}

bool Gui::createDeviceD3D()
{
    if (!m_guiWindow)
        return false;

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_guiWindow;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    if (D3D11CreateDeviceAndSwapChain(nullptr,
                                      D3D_DRIVER_TYPE_HARDWARE,
                                      nullptr,
                                      createDeviceFlags,
                                      featureLevelArray,
                                      2,
                                      D3D11_SDK_VERSION,
                                      &sd,
                                      &m_pSwapChain,
                                      &m_pd3dDevice,
                                      &featureLevel,
                                      &m_pd3dDeviceContext) != S_OK)
    {
        return false;
    }

    ID3D11Texture2D *pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
    pBackBuffer->Release();
    return true;
}

bool Gui::createGuiWindow()
{
    const wchar_t *className = L"EzFrontendDebugGUI";

    WNDCLASSEX wc;

    wc.cbClsExtra = 0;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = "EzFrontendDebugGUI";
    wc.lpszMenuName = nullptr;
    wc.style = CS_VREDRAW | CS_HREDRAW;

    ::RegisterClassEx(&wc);
    m_guiWindow = ::CreateWindowExW(WS_EX_TRANSPARENT,
                                    className,
                                    L"EzFrontendDebugGUI",
                                    WS_POPUP,
                                    int(m_windowPos.x),
                                    int(m_windowPos.y),
                                    int(m_windowSize.x),
                                    int(m_windowSize.y),
                                    nullptr,
                                    nullptr,
                                    wc.hInstance,
                                    nullptr);

    if (!m_guiWindow)
    {
        return false;
    }

    ShowWindow(m_guiWindow, SW_SHOWDEFAULT);
    UpdateWindow(m_guiWindow);

    SetLayeredWindowAttributes(m_guiWindow, 0, 255, LWA_ALPHA);
    return true;
}

bool Gui::createImGuiContext()
{
    IMGUI_CHECKVERSION();
    if (!ImGui::CreateContext())
    {
        return false;
    }

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.FontGlobalScale = 1.5f;

    if (!ImGui_ImplWin32_Init(m_guiWindow))
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dDeviceContext))
    {
        return false;
    }

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

    // Primary background
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);  // #131318
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f); // #131318

    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

    // Headers
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    // Highlights
    colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

    // Style tweaks
    style.WindowRounding = 5.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.PopupBorderSize = 0.f;

    return true;
}

bool Gui::createRenderTarget()
{
    ID3D11Texture2D *pBackBuffer = nullptr;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
    pBackBuffer->Release();
    return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT Gui::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    auto &gui = Gui::get();

    switch (msg)
    {
        case WM_SIZE:
        {
            if (gui.isInitialized() && wparam != SIZE_MINIMIZED)
            {
                if (gui.m_pd3dDevice != nullptr)
                {
                    gui.destroyRenderTarget();
                    gui.m_pSwapChain->ResizeBuffers(0,
                                                    (UINT)LOWORD(lparam),
                                                    (UINT)HIWORD(lparam),
                                                    DXGI_FORMAT_UNKNOWN,
                                                    0);
                    gui.createRenderTarget();
                }
            }
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_SYSCOMMAND:
        {
            if ((wparam & 0xfff0) == SC_KEYMENU)
                return 0;
            break;
        }
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool Gui::destroyDeviceD3D()
{
    if (m_mainRenderTargetView)
    {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
    if (m_pSwapChain)
    {
        m_pSwapChain->Release();
        m_pSwapChain = nullptr;
    }
    if (m_pd3dDeviceContext)
    {
        m_pd3dDeviceContext->Release();
        m_pd3dDeviceContext = nullptr;
    }
    if (m_pd3dDevice)
    {
        m_pd3dDevice->Release();
        m_pd3dDevice = nullptr;
    }

    return true;
}

bool Gui::destroyGuiWindow() { return DestroyWindow(m_guiWindow); }

bool Gui::destroyImGuiContext()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    return true;
}

bool Gui::destroyRenderTarget()
{
    if (m_mainRenderTargetView)
    {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
    return true;
}

void Gui::loop()
{
    bool exit = false;
    bool opened = true;

    std::shared_ptr<EzFrontendWrapper> frontend =
            std::make_shared<EzFrontendWrapper>(std::make_shared<SourceLoggingSink>(m_logger.get()));

    while (true)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                exit = true;
                break;
            }
        }

        if (exit)
            break;

        ImGui_ImplWin32_NewFrame();
        ImGui_ImplDX11_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(m_windowPos);
        ImGui::SetNextWindowSize(m_windowSize);

        if (ImGui::Begin("EzFrontendDebugGUI",
                         &opened,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))
        {
            static std::shared_ptr<Editor> editor = std::make_shared<Editor>(frontend);
            static std::shared_ptr<Output> output = std::make_shared<Output>(frontend->getErrorCollector());
            static std::shared_ptr<MainMenuBar> menuBar = std::make_shared<MainMenuBar>();

            menuBar->render();
            if (ImGui::BeginChild(editor->getName()))
            {
                editor->render();

                if (ImGui::BeginChild(output->getName()))
                {
                    output->render();
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::Render();

        if (!opened)
        {
            break;
        }

        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
        m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear_color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_pSwapChain->Present(1, 0);
    }
}
