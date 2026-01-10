#include "pch.h"

#include "Main Window.h"

#include "GUI/Fonts/Fonts.h"
#include "GUI/Main Menu/Main Menu.h"
#include "GUI/Radar/Radar.h"
#include "GUI/Fuser/Fuser.h"
#include "GUI/Aimbot/Aimbot.h"
#include "GUI/Player Table/Player Table.h"
#include "GUI/Item Table/Item Table.h"
#include "GUI/Color Picker/Color Picker.h"
#include "GUI/Keybinds/Keybinds.h"
#include "GUI/Config/Config.h"
#include "GUI/Flea Bot/Flea Bot.h"

// Custom dark theme with red accents (matching reference design)
void ApplyCustomTheme(ImGuiStyle& style)
{
	// Colors
	ImVec4* colors = style.Colors;
	
	// Dark red accent colors
	const ImVec4 redPrimary     = ImVec4(0.78f, 0.14f, 0.20f, 1.00f);  // #C82333
	const ImVec4 redDark        = ImVec4(0.55f, 0.10f, 0.14f, 1.00f);  // Darker red
	const ImVec4 redHover       = ImVec4(0.90f, 0.20f, 0.25f, 1.00f);  // Brighter red
	const ImVec4 greenActive    = ImVec4(0.20f, 0.80f, 0.20f, 1.00f);  // Green for enabled
	
	// Backgrounds - very dark
	colors[ImGuiCol_WindowBg]             = ImVec4(0.05f, 0.05f, 0.05f, 0.98f);  // #0D0D0D
	colors[ImGuiCol_ChildBg]              = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);  // #141414
	colors[ImGuiCol_PopupBg]              = ImVec4(0.08f, 0.08f, 0.08f, 0.98f);
	
	// Borders
	colors[ImGuiCol_Border]               = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
	colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	
	// Frame backgrounds (inputs, checkboxes, sliders)
	colors[ImGuiCol_FrameBg]              = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_FrameBgActive]        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	
	// Title bar
	colors[ImGuiCol_TitleBg]              = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_TitleBgActive]        = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.06f, 0.06f, 0.06f, 0.60f);
	
	// Menu bar
	colors[ImGuiCol_MenuBarBg]            = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
	
	// Scrollbar
	colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.05f, 0.05f, 0.05f, 0.60f);
	colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabActive]  = redPrimary;
	
	// Checkmark - red when enabled
	colors[ImGuiCol_CheckMark]            = greenActive;
	
	// Slider grab - red accent
	colors[ImGuiCol_SliderGrab]           = redPrimary;
	colors[ImGuiCol_SliderGrabActive]     = redHover;
	
	// Buttons
	colors[ImGuiCol_Button]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_ButtonHovered]        = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ButtonActive]         = redPrimary;
	
	// Headers (collapsing headers, selectable, menu items)
	colors[ImGuiCol_Header]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_HeaderHovered]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_HeaderActive]         = redDark;
	
	// Separator - subtle red tint
	colors[ImGuiCol_Separator]            = ImVec4(0.30f, 0.12f, 0.14f, 1.00f);
	colors[ImGuiCol_SeparatorHovered]     = redPrimary;
	colors[ImGuiCol_SeparatorActive]      = redHover;
	
	// Resize grip
	colors[ImGuiCol_ResizeGrip]           = ImVec4(0.30f, 0.30f, 0.30f, 0.40f);
	colors[ImGuiCol_ResizeGripHovered]    = redPrimary;
	colors[ImGuiCol_ResizeGripActive]     = redHover;
	
	// Tabs - red accent for active
	colors[ImGuiCol_Tab]                  = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_TabHovered]           = redPrimary;
	colors[ImGuiCol_TabActive]            = redDark;
	colors[ImGuiCol_TabUnfocused]         = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.18f, 0.10f, 0.12f, 1.00f);
	
	// Tables
	colors[ImGuiCol_TableHeaderBg]        = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]    = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
	colors[ImGuiCol_TableBorderLight]     = ImVec4(0.20f, 0.20f, 0.20f, 0.30f);
	colors[ImGuiCol_TableRowBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt]        = ImVec4(0.10f, 0.10f, 0.10f, 0.40f);
	
	// Text
	colors[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	
	// Misc
	colors[ImGuiCol_PlotLines]            = redPrimary;
	colors[ImGuiCol_PlotLinesHovered]     = redHover;
	colors[ImGuiCol_PlotHistogram]        = redPrimary;
	colors[ImGuiCol_PlotHistogramHovered] = redHover;
	
	// Style tweaks - subtle rounding
	style.WindowRounding    = 0.0f;   // Sharp window corners
	style.ChildRounding     = 0.0f;
	style.FrameRounding     = 2.0f;   // Slight rounding on frames
	style.PopupRounding     = 0.0f;
	style.ScrollbarRounding = 2.0f;
	style.GrabRounding      = 2.0f;
	style.TabRounding       = 0.0f;   // Sharp tabs
	
	style.WindowBorderSize  = 1.0f;
	style.ChildBorderSize   = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.PopupBorderSize   = 1.0f;
	style.TabBorderSize     = 0.0f;
	
	style.WindowPadding     = ImVec2(8.0f, 8.0f);
	style.FramePadding      = ImVec2(6.0f, 3.0f);
	style.ItemSpacing       = ImVec2(6.0f, 4.0f);
	style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
	style.IndentSpacing     = 16.0f;
	style.ScrollbarSize     = 12.0f;
	style.GrabMinSize       = 10.0f;
}

void Render(ImGuiContext* ctx)
{
	ImGui::SetCurrentContext(ctx);

	if (Fonts::m_IBMPlexMonoSemiBold == nullptr)
		Fonts::Initialize(ImGui::GetIO());

	ImGui::PushFont(Fonts::m_IBMPlexMonoSemiBold, 16.0f);

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

	// Overlays (keep as separate windows)
	Fuser::Render();
	Radar::Render();
	
	// Data tables (keep as separate toggleable windows)
	PlayerTable::Render();
	ItemTable::Render();
	
	// Main unified control panel (all settings integrated here)
	MainMenu::Render();


	ImGui::PopFont();
}

bool MainWindow::OnFrame()
{
	PreFrame();

	Render(ImGui::GetCurrentContext());

	PostFrame();

	return true;
}

bool MainWindow::CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	// Disable DXGI's default Alt+Enter fullscreen behavior.
	// - You are free to leave this enabled, but it will not work properly with multiple viewports.
	// - This must be done for all windows associated to the device. Our DX11 backend does this automatically for secondary viewports that it creates.
	IDXGIFactory* pSwapChainFactory;
	if (SUCCEEDED(g_pSwapChain->GetParent(IID_PPV_ARGS(&pSwapChainFactory))))
		pSwapChainFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

	CreateRenderTarget();
	return true;
}

void MainWindow::CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void MainWindow::CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void MainWindow::CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool MainWindow::Initialize()
{
	ImGui_ImplWin32_EnableDpiAwareness();

	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"MainWindow", nullptr };
	::RegisterClassExW(&wc);
	g_hWnd = ::CreateWindowEx(NULL, wc.lpszClassName, L"EFT DMA", WS_OVERLAPPEDWINDOW, 100, 100, 800, 800, nullptr, nullptr, wc.hInstance, nullptr);
	// Initialize Direct3D
	if (!CreateDeviceD3D(g_hWnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(g_hWnd, SW_SHOWDEFAULT);
	::UpdateWindow(g_hWnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	
	// Apply our custom dark theme with cyan accents
	ApplyCustomTheme(style);
	
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
	io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

	// When viewports are enabled we ensure full opacity on platform windows.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	return false;
}

bool MainWindow::Cleanup()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(g_hWnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return false;
}

bool MainWindow::PreFrame()
{
	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		if (msg.message == WM_QUIT)
			return false;
	}

	// Handle window resize (we don't resize directly in the WM_SIZE handler)
	if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
	{
		CleanupRenderTarget();
		g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
		g_ResizeWidth = g_ResizeHeight = 0;
		CreateRenderTarget();
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	return true;
}

bool MainWindow::PostFrame()
{
	ImGui::Render();
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	// Present
	HRESULT hr = g_pSwapChain->Present(0, 0);   // Present with vsync

	return true;
}