/**
 * @file EzFrontendDebugGUICommon.h
 * @brief Precompiled common includes for the EzFrontendDebugGUI application.
 *
 * Pulls in the Win32 API headers, ImGui / DirectX 11 backend headers, and
 * the EzSemantics umbrella so every GUI source file can access both the
 * frontend compiler API and the ImGui rendering layer.
 */
#ifndef EZPACKER_EZFRONTENDDEBUGGUICOMMON_H
#define EZPACKER_EZFRONTENDDEBUGGUICOMMON_H

#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <optional>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commdlg.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <d3d11.h>

#elif defined(__linux__)

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#endif

#include <EzLexer.h>
#include <EzSemantics.h>
#include <EzFrontendCompiler.h>
#include <EzMir.h>

#endif // EZPACKER_EZFRONTENDDEBUGGUICOMMON_H
