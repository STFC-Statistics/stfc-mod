#include "overlay.h"
#include "config.h"
#include "file.h"
#include "hook_health.h"
#include "main_thread_queue.h"
#include "overlay_log.h"
#include "sync_stats.h"
#include "version.h"

#include <il2cpp/il2cpp_helper.h>
#include <prime/CanvasController.h>
#include <prime/DebugInfo.h>
#include <prime/ScreenManager.h>
#include <prime/SectionDirectorBase.h>
#include <str_utils.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

#if _WIN32
#include <Windows.h>
#include <Psapi.h>
#include <d3d11.h>
#include <dxgi.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "psapi.lib")
#endif

#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Overlay
{
static std::atomic<bool> s_visible{false};

// Panel visibility flags — control which tabs are shown
static bool s_showConfig      = true;
static bool s_showOverview    = true;
static bool s_showSync        = true;
static bool s_showSystem      = true;
static bool s_showLogViewer   = true;
static bool s_showInspector   = true;

// FPS history for graph
static constexpr int kFpsHistorySize = 120;
static float s_fpsHistory[kFpsHistorySize] = {};
static int   s_fpsHistoryOffset = 0;

// Forward declarations
void RenderPanels();
static void RenderConfigPanel();
static void RenderOverviewPanel();
static void RenderSyncPanel();
static void RenderSystemPanel();
static void RenderLogViewerPanel();
static void RenderInspectorPanel();
static void ExportDiagnostics();
static void SaveConfigToFile();

#if _WIN32

// --- D3D11 Present hook ---
//
// We hook IDXGISwapChain::Present via vtable patching.
// The vtable index for Present is 8.

static HRESULT(STDMETHODCALLTYPE* s_origPresent)(IDXGISwapChain*, UINT, UINT) = nullptr;
static ID3D11Device*           s_d3dDevice     = nullptr;
static ID3D11DeviceContext*    s_d3dContext    = nullptr;
static IDXGISwapChain*         s_swapChain     = nullptr;
static ID3D11RenderTargetView* s_rtv           = nullptr;
static bool                    s_imguiInit     = false;
static WNDPROC                 s_origWndProc   = nullptr;
static HWND                    s_hwnd          = nullptr;

static void CreateRenderTarget()
{
  if (!s_swapChain || !s_d3dDevice) return;
  if (s_rtv) { s_rtv->Release(); s_rtv = nullptr; }

  ID3D11Texture2D* backBuffer = nullptr;
  if (SUCCEEDED(s_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)))) {
    s_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &s_rtv);
    backBuffer->Release();
  }
}

static void CleanupRenderTarget()
{
  if (s_rtv) { s_rtv->Release(); s_rtv = nullptr; }
}

static LRESULT CALLBACK WndProc_Hook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // F8 toggles overlay visibility (even when overlay is not yet visible)
  if (msg == WM_KEYDOWN && wParam == VK_F8) {
    Toggle();
    if (s_visible.load()) {
      spdlog::info("Overlay: toggled visible");
    }
    return 0;
  }

  // Recreate render target on resize
  if (msg == WM_SIZE && s_imguiInit) {
    CleanupRenderTarget();
    // RTV will be recreated on next Present_Hook
  }

  if (s_visible.load()) {
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    // Consume input when overlay is visible
    switch (msg) {
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_CHAR:
      case WM_MOUSEMOVE:
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
      case WM_MOUSEWHEEL:
        return 0;
    }
  }
  return CallWindowProcW(s_origWndProc, hWnd, msg, wParam, lParam);
}

static void InitImGui(HWND hwnd)
{
  if (s_imguiInit) return;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = nullptr; // Don't save imgui.ini

  ImGui::StyleColorsDark();

  // Obtain D3D11 device and context from the swap chain
  if (SUCCEEDED(s_swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&s_d3dDevice)))) {
    s_d3dDevice->GetImmediateContext(&s_d3dContext);
  }

  if (!s_d3dDevice || !s_d3dContext) {
    spdlog::error("Overlay: failed to get D3D11 device/context from swap chain");
    return;
  }

  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(s_d3dDevice, s_d3dContext);

  CreateRenderTarget();

  s_hwnd = hwnd;

  // Hook WndProc for input
  s_origWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                                              reinterpret_cast<LONG_PTR>(WndProc_Hook)));

  s_imguiInit = true;
  spdlog::info("Overlay: ImGui initialized (D3D11), RTV={}", s_rtv ? "ok" : "FAILED");
}

static void ShutdownImGui()
{
  if (!s_imguiInit) return;

  // Restore WndProc
  if (s_hwnd && s_origWndProc) {
    SetWindowLongPtrW(s_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(s_origWndProc));
    s_origWndProc = nullptr;
    s_hwnd = nullptr;
  }

  CleanupRenderTarget();

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  if (s_d3dContext) { s_d3dContext->Release(); s_d3dContext = nullptr; }
  if (s_d3dDevice)  { s_d3dDevice->Release();  s_d3dDevice = nullptr; }

  s_imguiInit = false;
}

static HRESULT STDMETHODCALLTYPE Present_Hook(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
  s_swapChain = This;

  if (!s_imguiInit) {
    // Find the HWND from the swap chain
    DXGI_SWAP_CHAIN_DESC desc;
    if (SUCCEEDED(This->GetDesc(&desc))) {
      InitImGui(desc.OutputWindow);
    }
  }

  if (s_imguiInit && s_visible.load()) {
    // Ensure we have a valid render target (recreate on resize)
    if (!s_rtv) {
      CreateRenderTarget();
    }

    if (s_rtv) {
      s_d3dContext->OMSetRenderTargets(1, &s_rtv, nullptr);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    RenderPanels();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  }

  return s_origPresent(This, SyncInterval, Flags);
}

// Hook the Present vtable by creating a temporary D3D11 device and swap chain,
// then patching the vtable entry.
static void HookPresentVTable()
{
  // Create a temporary device and swap chain to get the vtable
  DXGI_SWAP_CHAIN_DESC desc{};
  desc.BufferCount = 1;
  desc.BufferDesc.Width = 1;
  desc.BufferDesc.Height = 1;
  desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.BufferDesc.RefreshRate.Numerator = 60;
  desc.BufferDesc.RefreshRate.Denominator = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.OutputWindow = GetDesktopWindow();
  desc.SampleDesc.Count = 1;
  desc.Windowed = TRUE;
  desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  ID3D11Device*          tempDevice = nullptr;
  ID3D11DeviceContext*   tempContext = nullptr;
  IDXGISwapChain*        tempSwapChain = nullptr;

  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
  if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                           &featureLevel, 1, D3D11_SDK_VERSION,
                                           &desc, &tempSwapChain, &tempDevice, nullptr, &tempContext))) {
    spdlog::error("Overlay: failed to create temporary D3D11 device for vtable hook");
    return;
  }

  // Get the vtable pointer from the swap chain
  void** vtable = *reinterpret_cast<void***>(tempSwapChain);

  // Present is at vtable index 8
  const int PresentIndex = 8;

  // Save original
  s_origPresent = reinterpret_cast<HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT)>(vtable[PresentIndex]);

  // Patch vtable
  DWORD oldProtect = 0;
  VirtualProtect(&vtable[PresentIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
  vtable[PresentIndex] = reinterpret_cast<void*>(Present_Hook);
  VirtualProtect(&vtable[PresentIndex], sizeof(void*), oldProtect, &oldProtect);

  // Release temporary objects
  tempSwapChain->Release();
  tempContext->Release();
  tempDevice->Release();

  spdlog::info("Overlay: IDXGISwapChain::Present vtable hooked");
}

void Install()
{
  HookPresentVTable();
  MainThreadQueue::Install();
  spdlog::info("Overlay: installed (Windows D3D11)");
}

#else
// macOS: deferred until Metal integration is proven
void Install()
{
  spdlog::info("Overlay: macOS Metal integration deferred (not yet implemented)");
}
#endif

void Toggle()
{
  s_visible.store(!s_visible.load());
}

bool IsVisible()
{
  return s_visible.load();
}

// --- Panels ---

void RenderPanels()
{
  // Update FPS history
  s_fpsHistory[s_fpsHistoryOffset] = ImGui::GetIO().Framerate;
  s_fpsHistoryOffset = (s_fpsHistoryOffset + 1) % kFpsHistorySize;

  ImGui::SetNextWindowSize(ImVec2(800, 580), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("STFC Community Mod Debug Overlay", nullptr,
                    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse))
  {
    ImGui::End();
    return;
  }

  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("Tabs"))
    {
      ImGui::MenuItem("Overview", nullptr, &s_showOverview);
      ImGui::MenuItem("Config", nullptr, &s_showConfig);
      ImGui::MenuItem("Sync", nullptr, &s_showSync);
      ImGui::MenuItem("System", nullptr, &s_showSystem);
      ImGui::MenuItem("Inspector", nullptr, &s_showInspector);
      ImGui::MenuItem("Log Viewer", nullptr, &s_showLogViewer);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Actions"))
    {
      if (ImGui::MenuItem("Export Diagnostics"))
        ExportDiagnostics();
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  if (ImGui::BeginTabBar("OverlayTabs", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip))
  {
    if (s_showOverview && ImGui::BeginTabItem("Overview"))
    {
      RenderOverviewPanel();
      ImGui::EndTabItem();
    }
    if (s_showConfig && ImGui::BeginTabItem("Config"))
    {
      RenderConfigPanel();
      ImGui::EndTabItem();
    }
    if (s_showSync && ImGui::BeginTabItem("Sync"))
    {
      RenderSyncPanel();
      ImGui::EndTabItem();
    }
    if (s_showSystem && ImGui::BeginTabItem("System"))
    {
      RenderSystemPanel();
      ImGui::EndTabItem();
    }
    if (s_showInspector && ImGui::BeginTabItem("Inspector"))
    {
      RenderInspectorPanel();
      ImGui::EndTabItem();
    }
    if (s_showLogViewer && ImGui::BeginTabItem("Logs"))
    {
      RenderLogViewerPanel();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::End();
}

static const char* StatusToString(HookHealth::Status s)
{
  switch (s) {
    case HookHealth::Status::Installed:    return "Installed";
    case HookHealth::Status::Skipped:      return "Skipped";
    case HookHealth::Status::Failed:       return "FAILED";
    default:                               return "NotInstalled";
  }
}

static ImVec4 StatusColor(HookHealth::Status s)
{
  switch (s) {
    case HookHealth::Status::Installed:    return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    case HookHealth::Status::Skipped:      return ImVec4(0.9f, 0.8f, 0.2f, 1.0f);
    case HookHealth::Status::Failed:       return ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
    default:                               return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
  }
}

static void RenderOverviewPanel()
{
  // --- Quick stats row ---
  auto entries = HookHealth::GetEntries();
  auto installed = 0, skipped = 0, failed = 0, pending = 0;
  for (const auto& e : entries) {
    switch (e.status) {
      case HookHealth::Status::Installed: installed++; break;
      case HookHealth::Status::Skipped:   skipped++;   break;
      case HookHealth::Status::Failed:    failed++;    break;
      default:                            pending++;   break;
    }
  }

  ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%d installed", installed);
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "%d skipped", skipped);
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%d failed", failed);
  ImGui::SameLine();
  if (pending > 0) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%d pending", pending);
  }

  ImGui::Separator();

  // --- Patch status grid ---
  if (!entries.empty()) {
    ImGui::Text("Patch Status:");
    float badgeW = 140.0f;
    int   cols = (int)(ImGui::GetWindowWidth() / badgeW);
    if (cols < 1) cols = 1;

    for (int i = 0; i < (int)entries.size(); i++) {
      if (i > 0 && (i % cols) != 0) ImGui::SameLine();

      const auto& e = entries[i];
      ImVec4 col = StatusColor(e.status);
      ImGui::PushStyleColor(ImGuiCol_Button, col);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
      ImGui::Button(e.name.c_str(), ImVec2(badgeW - 8, 0));
      if (ImGui::IsItemHovered() && !e.error_msg.empty()) {
        ImGui::SetTooltip("%s", e.error_msg.c_str());
      }
      ImGui::PopStyleColor(2);
    }
  }

  ImGui::Separator();

  // --- Failed hooks detail (only if any) ---
  if (failed > 0) {
    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Failed Hooks:");
    if (ImGui::BeginTable("FailedHooksTable", 2,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupScrollFreeze(0, 1);
      ImGui::TableSetupColumn("Patch");
      ImGui::TableSetupColumn("Error");
      ImGui::TableHeadersRow();

      for (const auto& e : entries) {
        if (e.status != HookHealth::Status::Failed) continue;
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(e.name.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::TextWrapped("%s", e.error_msg.empty() ? "unknown error" : e.error_msg.c_str());
      }
      ImGui::EndTable();
    }
  }

  // --- Sync quick stats ---
  ImGui::Separator();
  ImGui::Text("Sync: %zu sent, %zu errors, %zu queued",
              SyncStats::GetTotalSent(), SyncStats::GetTotalErrors(), SyncStats::GetTotalQueueDepth());
}

static void RenderConfigPanel()
{
  auto& cfg = Config::Get();
  bool changed = false;

  ImGui::Text("Config file: %s", File::Config());

  ImGui::Separator();

  if (ImGui::CollapsingHeader("UI Scale"))
  {
    changed |= ImGui::SliderFloat("UI Scale##slider", &cfg.ui_scale, 0.0f, 5.0f, "%.2f");
    changed |= ImGui::SliderFloat("UI Scale (Viewer)", &cfg.ui_scale_viewer, 0.0f, 5.0f, "%.2f");
    changed |= ImGui::Checkbox("Adjust Scale for Resolution", &cfg.adjust_scale_res);
    changed |= ImGui::Checkbox("Allow Cursor", &cfg.allow_cursor);
  }

  if (ImGui::CollapsingHeader("Zoom"))
  {
    changed |= ImGui::SliderFloat("Zoom##slider", &cfg.zoom, 0.0f, 500.0f, "%.1f");
    changed |= ImGui::SliderFloat("FR Scale", &cfg.fr_scale, 0.0f, 10.0f, "%.2f");
    changed |= ImGui::SliderFloat("Default System Zoom", &cfg.default_system_zoom, 0.0f, 500.0f, "%.1f");
    changed |= ImGui::SliderFloat("Keyboard Zoom Speed", &cfg.keyboard_zoom_speed, 0.0f, 10.0f, "%.2f");
    ImGui::Separator();
    ImGui::Text("Zoom Presets:");
    changed |= ImGui::SliderFloat("Preset 1", &cfg.system_zoom_preset_1, 0.0f, 500.0f, "%.1f");
    changed |= ImGui::SliderFloat("Preset 2", &cfg.system_zoom_preset_2, 0.0f, 500.0f, "%.1f");
    changed |= ImGui::SliderFloat("Preset 3", &cfg.system_zoom_preset_3, 0.0f, 500.0f, "%.1f");
    changed |= ImGui::SliderFloat("Preset 4", &cfg.system_zoom_preset_4, 0.0f, 500.0f, "%.1f");
    changed |= ImGui::SliderFloat("Preset 5", &cfg.system_zoom_preset_5, 0.0f, 500.0f, "%.1f");
  }

  if (ImGui::CollapsingHeader("Pan & Transition"))
  {
    changed |= ImGui::SliderFloat("Pan Momentum", &cfg.system_pan_momentum, 0.0f, 10.0f, "%.2f");
    changed |= ImGui::SliderFloat("Momentum Falloff", &cfg.system_pan_momentum_falloff, 0.0f, 10.0f, "%.2f");
    changed |= ImGui::SliderFloat("Transition Time", &cfg.transition_time, 0.0f, 10.0f, "%.2f");
  }

  if (ImGui::CollapsingHeader("Hotkeys"))
  {
    changed |= ImGui::Checkbox("Hotkeys Enabled", &cfg.hotkeys_enabled);
    changed |= ImGui::Checkbox("Hotkeys Extended", &cfg.hotkeys_extended);
    changed |= ImGui::Checkbox("Use Scopely Hotkeys", &cfg.use_scopely_hotkeys);
    changed |= ImGui::Checkbox("Use Presets as Default", &cfg.use_presets_as_default);
    ImGui::Separator();
    changed |= ImGui::Checkbox("Disable Move Keys", &cfg.disable_move_keys);
    changed |= ImGui::Checkbox("Disable Preview Locate", &cfg.disable_preview_locate);
    changed |= ImGui::Checkbox("Disable Preview Recall", &cfg.disable_preview_recall);
    changed |= ImGui::Checkbox("Disable Escape Exit", &cfg.disable_escape_exit);
    changed |= ImGui::SliderInt("Select Timer", &cfg.select_timer, 0, 5000);
  }

  if (ImGui::CollapsingHeader("Chat"))
  {
    changed |= ImGui::Checkbox("Disable Galaxy Chat", &cfg.disable_galaxy_chat);
    changed |= ImGui::Checkbox("Disable Veil Chat", &cfg.disable_veil_chat);
    changed |= ImGui::Checkbox("Disable First Popup", &cfg.disable_first_popup);
  }

  if (ImGui::CollapsingHeader("Cargo Viewers"))
  {
    changed |= ImGui::Checkbox("Show Cargo Default", &cfg.show_cargo_default);
    changed |= ImGui::Checkbox("Show Player Cargo", &cfg.show_player_cargo);
    changed |= ImGui::Checkbox("Show Station Cargo", &cfg.show_station_cargo);
    changed |= ImGui::Checkbox("Show Hostile Cargo", &cfg.show_hostile_cargo);
    changed |= ImGui::Checkbox("Show Armada Cargo", &cfg.show_armada_cargo);
    ImGui::Separator();
    changed |= ImGui::SliderInt("Cargo Significant Decimals", &cfg.cargo_significant_decimals, 0, 6);
  }

  if (ImGui::CollapsingHeader("Loading Screen"))
  {
    changed |= ImGui::Checkbox("Loader Enabled", &cfg.loader_enabled);
    changed |= ImGui::Checkbox("Loader Transition", &cfg.loader_transition);
    changed |= ImGui::Checkbox("Loader Transition Black", &cfg.loader_transition_black);
    changed |= ImGui::SliderFloat("Loader Logo Scale", &cfg.loader_logo_scale, 0.0f, 5.0f, "%.2f");
    changed |= ImGui::Checkbox("Loader Tip Enabled", &cfg.loader_tip_enabled);
    changed |= ImGui::Checkbox("Skip Reveal Sequence", &cfg.always_skip_reveal_sequence);
  }

  if (ImGui::CollapsingHeader("Misc"))
  {
    changed |= ImGui::Checkbox("Auto Open Bulk Claim Flyout", &cfg.auto_open_bulk_claim_flyout);
    changed |= ImGui::Checkbox("Extend Donation Slider", &cfg.extend_donation_slider);
    changed |= ImGui::SliderInt("Extend Donation Max", &cfg.extend_donation_max, 0, 10000);
    ImGui::Separator();
    changed |= ImGui::Checkbox("Borderless Fullscreen", &cfg.borderless_fullscreen);
    changed |= ImGui::Checkbox("Show All Resolutions", &cfg.show_all_resolutions);
    changed |= ImGui::Checkbox("Disable Toast Banners", &cfg.disable_toast_banners);
    ImGui::Separator();
    changed |= ImGui::Checkbox("Sync Logging", &cfg.sync_logging);
    changed |= ImGui::Checkbox("Sync Debug", &cfg.sync_debug);
  }

  if (ImGui::CollapsingHeader("Patch Toggles (require restart)"))
  {
    changed |= ImGui::Checkbox("UI Scale Hooks",       &cfg.installUiScaleHooks);
    changed |= ImGui::Checkbox("Zoom Hooks",           &cfg.installZoomHooks);
    changed |= ImGui::Checkbox("Buff Fix Hooks",       &cfg.installBuffFixHooks);
    changed |= ImGui::Checkbox("Toast Banner Hooks",   &cfg.installToastBannerHooks);
    changed |= ImGui::Checkbox("Pan Hooks",            &cfg.installPanHooks);
    changed |= ImGui::Checkbox("Hotkey Hooks",         &cfg.installHotkeyHooks);
    changed |= ImGui::Checkbox("Sync Patches",         &cfg.installSyncPatches);
    changed |= ImGui::Checkbox("Object Tracker",       &cfg.installObjectTracker);
    changed |= ImGui::Checkbox("Game Version Hook",    &cfg.installGameVersionHook);
    changed |= ImGui::Checkbox("Chat Patches",         &cfg.installChatPatches);
    changed |= ImGui::Checkbox("Loading Screen",       &cfg.installLoadingScreenHooks);
    changed |= ImGui::Checkbox("Transition Screen",    &cfg.installTransitionScreenHooks);
    changed |= ImGui::Checkbox("Officer Sort",         &cfg.installOfficerSortHooks);
    changed |= ImGui::Checkbox("Cargo Format",         &cfg.installCargoFormatHooks);
    changed |= ImGui::Checkbox("Focus Search",         &cfg.installFocusSearchHooks);
  }

  if (changed)
    SaveConfigToFile();

  ImGui::Separator();
  if (ImGui::Button("Save Config"))
    SaveConfigToFile();
  ImGui::SameLine();
  if (ImGui::Button("Export Diagnostics"))
    ExportDiagnostics();
  ImGui::SameLine();
  ImGui::TextWrapped("Changes auto-save. Patch toggles require restart.");
}

static void RenderSyncPanel()
{
  auto stats = SyncStats::GetStats();

  ImGui::Text("Total: %zu sent, %zu errors, %zu queued",
              SyncStats::GetTotalSent(), SyncStats::GetTotalErrors(), SyncStats::GetTotalQueueDepth());

  ImGui::Separator();

  if (stats.empty()) {
    ImGui::TextDisabled("No sync targets configured or active.");
    return;
  }

  if (ImGui::BeginTable("SyncStatsTable", 7,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                        ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Target");
    ImGui::TableSetupColumn("Queue");
    ImGui::TableSetupColumn("Sent");
    ImGui::TableSetupColumn("Errors");
    ImGui::TableSetupColumn("Bytes");
    ImGui::TableSetupColumn("Status");
    ImGui::TableSetupColumn("Last");
    ImGui::TableHeadersRow();

    auto now = std::chrono::steady_clock::now();

    for (const auto& s : stats) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(s.name.c_str());
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%zu", s.queue_depth);
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%zu", s.total_sent);
      ImGui::TableSetColumnIndex(3);
      if (s.total_errors > 0)
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%zu", s.total_errors);
      else
        ImGui::Text("%zu", s.total_errors);
      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%zu", s.total_bytes);
      ImGui::TableSetColumnIndex(5);
      int code = s.last_status_code;
      if (code == 0)
        ImGui::TextDisabled("n/a");
      else if (code >= 200 && code < 300)
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%d", code);
      else if (code >= 400)
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%d", code);
      else
        ImGui::Text("%d", code);
      ImGui::TableSetColumnIndex(6);
      if (s.last_send.time_since_epoch().count() == 0) {
        ImGui::TextDisabled("never");
      } else {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(now - s.last_send).count();
        ImGui::Text("%llds ago", (long long)secs);
      }
    }
    ImGui::EndTable();
  }
}

static void RenderSystemPanel()
{
  auto& io = ImGui::GetIO();

  // FPS graph
  float values[kFpsHistorySize];
  for (int i = 0; i < kFpsHistorySize; i++) {
    values[i] = s_fpsHistory[(s_fpsHistoryOffset + i) % kFpsHistorySize];
  }
  ImGui::PlotLines("FPS", values, kFpsHistorySize, 0, nullptr, 0.0f, 120.0f, ImVec2(0, 60));

  ImGui::Text("FPS: %.1f", io.Framerate);
  ImGui::Text("Frame time: %.3f ms", 1000.0f / (io.Framerate > 0 ? io.Framerate : 1.0f));

  ImGui::Separator();

#if VERSION_PATCH
  ImGui::Text("Mod version: %d.%d.%d (Patch %d)", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_PATCH);
#else
  ImGui::Text("Mod version: %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
#endif

  // Tracked objects count
  {
    std::scoped_lock lk{tracked_objects_mutex};
    size_t total = 0;
    for (const auto& [cls, vec] : tracked_objects) {
      total += vec.size();
    }
    ImGui::Text("Tracked objects: %zu (across %zu classes)", total, tracked_objects.size());
  }

  // Process memory (Windows)
#if _WIN32
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    ImGui::Text("Working set: %.1f MB", pmc.WorkingSetSize / (1024.0f * 1024.0f));
    ImGui::Text("Peak working set: %.1f MB", pmc.PeakWorkingSetSize / (1024.0f * 1024.0f));
    ImGui::Text("Page file usage: %.1f MB", pmc.PagefileUsage / (1024.0f * 1024.0f));
  }
#endif

  ImGui::Separator();
  ImGui::Text("Overlay: %s", IsVisible() ? "Visible" : "Hidden");
  ImGui::Text("Log entries buffered: %zu", OverlayLog::Count());
  ImGui::Text("Press F8 to toggle overlay");

  // --- Game Info (DebugInfo) ---
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Game Info"))
  {
    auto appVer   = DebugInfo::AppVersion();
    auto srvVer   = DebugInfo::ServerVersion();
    auto unityVer = DebugInfo::UnityVersion();
    auto stack    = DebugInfo::StackName();
    auto srvInst  = DebugInfo::CurrentServerInstanceName();
    auto homeSrv  = DebugInfo::HomeServerInstanceName();
    auto userId   = DebugInfo::UserId();
    auto quality  = DebugInfo::QualitySettings();
    auto platform = DebugInfo::Platform();
    auto memMode  = DebugInfo::MemoryMode();

    ImGui::Text("App Version: %s", appVer.empty() ? "(n/a)" : appVer.c_str());
    ImGui::Text("Server Version: %s", srvVer.empty() ? "(n/a)" : srvVer.c_str());
    ImGui::Text("Unity Version: %s", unityVer.empty() ? "(n/a)" : unityVer.c_str());
    ImGui::Text("Stack: %s", stack.empty() ? "(n/a)" : stack.c_str());
    ImGui::Text("Server Instance: %s", srvInst.empty() ? "(n/a)" : srvInst.c_str());
    ImGui::Text("Home Server: %s", homeSrv.empty() ? "(n/a)" : homeSrv.c_str());
    ImGui::Text("User ID: %s", userId.empty() ? "(n/a)" : userId.c_str());
    ImGui::Text("Quality: %s", quality.empty() ? "(n/a)" : quality.c_str());
    ImGui::Text("Platform: %s", platform.empty() ? "(n/a)" : platform.c_str());
    ImGui::Text("Memory Mode: %s", memMode.empty() ? "(n/a)" : memMode.c_str());
  }
}

static void RenderLogViewerPanel()
{
  // Controls
  static bool autoScroll = true;
  static int  maxLines = 200;
  ImGui::Checkbox("Auto-scroll", &autoScroll);
  ImGui::SameLine();
  ImGui::SliderInt("Lines", &maxLines, 50, 500);
  ImGui::SameLine();
  if (ImGui::Button("Clear"))
    OverlayLog::Clear();

  ImGui::Separator();

  auto entries = OverlayLog::GetRecent(maxLines);

  ImGui::BeginChild("LogScroll", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);

  for (const auto& entry : entries) {
    ImVec4 col;
    switch (entry.level) {
      case spdlog::level::trace:    col = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
      case spdlog::level::debug:    col = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); break;
      case spdlog::level::info:     col = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;
      case spdlog::level::warn:     col = ImVec4(0.9f, 0.8f, 0.2f, 1.0f); break;
      case spdlog::level::err:      col = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); break;
      case spdlog::level::critical: col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
      default:                      col = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(entry.text.c_str());
    ImGui::PopStyleColor();
  }

  if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
}

static void RenderInspectorPanel()
{
  // --- Current View ---
  if (ImGui::CollapsingHeader("Current View", ImGuiTreeNodeFlags_DefaultOpen))
  {
    auto* top = ScreenManager::GetTopCanvas(true);
    if (top) {
      auto* nameStr = top->name;
      std::string name = nameStr ? to_string(nameStr) : "(unnamed)";
      bool visible = top->Visible();
      bool mVisible = top->m_Visible();

      ImGui::Text("Top Canvas: %s", name.c_str());
      ImGui::Text("Visible: %s  m_Visible: %s", visible ? "true" : "false", mVisible ? "true" : "false");

      // Toggle visibility
      bool vis = mVisible;
      if (ImGui::Checkbox("##top_visible", &vis)) {
        static auto helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "CanvasController");
        static auto offset = helper.GetField("m_visible").offset();
        *(bool*)((ptrdiff_t)top + offset) = vis;
      }
      ImGui::SameLine();
      ImGui::Text("Toggle m_visible");
    } else {
      ImGui::TextDisabled("No visible canvas found.");
    }
  }

  ImGui::Separator();

  // --- Tracked Objects ---
  if (ImGui::CollapsingHeader("Tracked Objects", ImGuiTreeNodeFlags_DefaultOpen))
  {
    struct TrackedEntry {
      std::string  className;
      std::string  ns;
      size_t       count;
      Il2CppClass* cls;
    };

    std::vector<TrackedEntry> entries;
    size_t totalObjects = 0;

    {
      std::scoped_lock lk{tracked_objects_mutex};
      entries.reserve(tracked_objects.size());
      for (const auto& [cls, vec] : tracked_objects) {
        if (!cls) continue;
        const char* name = il2cpp_class_get_name(cls);
        const char* ns   = il2cpp_class_get_namespace(cls);
        entries.push_back({
          name ? name : "?",
          ns ? ns : "",
          vec.size(),
          cls
        });
        totalObjects += vec.size();
      }
    }

    std::sort(entries.begin(), entries.end(),
              [](const TrackedEntry& a, const TrackedEntry& b) { return a.count > b.count; });

    ImGui::Text("Total: %zu objects across %zu classes", totalObjects, entries.size());

    ImGui::Separator();

    if (ImGui::BeginTable("TrackedObjectsTable", 3,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupScrollFreeze(0, 1);
      ImGui::TableSetupColumn("Class");
      ImGui::TableSetupColumn("Namespace");
      ImGui::TableSetupColumn("Count");
      ImGui::TableHeadersRow();

      static auto ccHelper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "CanvasController");
      auto ccCls = ccHelper.get_cls();

      for (const auto& e : entries) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::PushID(e.cls);
        bool expanded = ImGui::TreeNode(e.className.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(e.ns.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%zu", e.count);

        if (expanded) {
          ImGui::TreePop();

          if (e.cls) {
            std::scoped_lock lk{tracked_objects_mutex};
            auto it = tracked_objects.find(e.cls);
            if (it != tracked_objects.end()) {
              // Check if this class is CanvasController or subclass
              bool isCC = false;
              for (auto* c = e.cls; c; c = il2cpp_class_get_parent(c)) {
                if (c == ccCls) { isCC = true; break; }
              }

              for (size_t i = 0; i < it->second.size(); i++) {
                auto* ptr = reinterpret_cast<void*>(it->second[i]);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("  [%zu] 0x%p", i, (void*)ptr);

                if (isCC && ptr) {
                  auto* cc = reinterpret_cast<CanvasController*>(ptr);
                  auto* nameStr = cc->name;
                  std::string name = nameStr ? to_string(nameStr) : "(unnamed)";
                  bool vis = cc->m_Visible();

                  ImGui::TableSetColumnIndex(1);
                  ImGui::Text("name=%s vis=%s", name.c_str(), vis ? "Y" : "N");

                  ImGui::TableSetColumnIndex(2);
                  bool toggleVis = vis;
                  ImGui::PushID((int)i);
                  if (ImGui::Checkbox("##vis", &toggleVis)) {
                    static auto offset = ccHelper.GetField("m_visible").offset();
                    *(bool*)((ptrdiff_t)cc + offset) = toggleVis;
                  }
                  ImGui::PopID();
                } else {
                  ImGui::TableSetColumnIndex(1);
                  ImGui::TextDisabled("-");
                  ImGui::TableSetColumnIndex(2);
                  ImGui::TextDisabled("-");
                }
              }
            }
          }
        }
        ImGui::PopID();
      }
      ImGui::EndTable();
    }
  }

  // --- Game Sections ---
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Game Sections"))
  {
    auto* sm = SectionManager::Instance();
    if (sm) {
      ImGui::Text("Current Section ID: %d", sm->CurrentSection());
      ImGui::Text("Previous Section ID: %d", sm->PreviousSection());
      ImGui::Text("Section Change In Progress: %s", sm->IsSectionChangeInProgress() ? "yes" : "no");
    } else {
      ImGui::TextDisabled("SectionManager not available");
    }

    ImGui::Separator();

    // Find tracked SectionDirectorBase instances
    static auto sdbHelper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.Sections", "SectionDirectorBase");
    auto sdbCls = sdbHelper.get_cls();

    std::vector<SectionDirectorBase*> directors;

    {
      std::scoped_lock lk{tracked_objects_mutex};
      for (const auto& [cls, vec] : tracked_objects) {
        if (!cls) continue;
        // Check if this class is SectionDirectorBase or a subclass
        bool isSDB = false;
        for (auto* c = cls; c; c = il2cpp_class_get_parent(c)) {
          if (c == sdbCls) { isSDB = true; break; }
        }
        if (!isSDB) continue;

        for (auto ptr : vec) {
          directors.push_back(reinterpret_cast<SectionDirectorBase*>(ptr));
        }
      }
    }

    ImGui::Text("Section Directors: %zu", directors.size());

    if (ImGui::BeginTable("SectionDirectorsTable", 3,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupScrollFreeze(0, 1);
      ImGui::TableSetupColumn("Section ID");
      ImGui::TableSetupColumn("State");
      ImGui::TableSetupColumn("Address");
      ImGui::TableHeadersRow();

      for (size_t i = 0; i < directors.size(); i++) {
        auto* d = directors[i];
        if (!d) continue;

        auto state = d->MyState();
        int sectionId = d->MySectionId();

        ImVec4 stateColor(0.5f, 0.5f, 0.5f, 1.0f);
        switch (state) {
          case SectionDirectorBase::State::Active:    stateColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); break;
          case SectionDirectorBase::State::PreLoaded: stateColor = ImVec4(0.9f, 0.8f, 0.2f, 1.0f); break;
          case SectionDirectorBase::State::Sleeping:  stateColor = ImVec4(0.3f, 0.5f, 0.9f, 1.0f); break;
          case SectionDirectorBase::State::NotLoaded: stateColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
          case SectionDirectorBase::State::Idle:      stateColor = ImVec4(0.9f, 0.6f, 0.2f, 1.0f); break;
          default: break;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", sectionId);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushStyleColor(ImGuiCol_Text, stateColor);
        ImGui::TextUnformatted(SectionDirectorBase::StateToString(state));
        ImGui::PopStyleColor();
        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("0x%p", (void*)d);
      }
      ImGui::EndTable();
    }
  }
}

static void SaveConfigToFile()
{
  auto& cfg = Config::Get();
  auto configPath = std::string(File::MakePath(File::Config(), true));

  toml::table tbl;
  try {
    tbl = toml::parse_file(configPath);
  } catch (...) {
    spdlog::warn("Overlay: could not parse existing config, creating fresh");
    tbl = toml::table{};
  }

  auto set = [&tbl]<typename T>(const char* sec, const char* key, T val) {
    if (!tbl.contains(sec)) tbl.emplace<toml::table>(sec, toml::table{});
    tbl[sec].as_table()->insert_or_assign(key, val);
  };

  // patches
  set("patches", "uiscalehooks", cfg.installUiScaleHooks);
  set("patches", "zoomhooks", cfg.installZoomHooks);
  set("patches", "bufffixhooks", cfg.installBuffFixHooks);
  set("patches", "toastbannerhooks", cfg.installToastBannerHooks);
  set("patches", "panhooks", cfg.installPanHooks);
  set("patches", "hotkeyhooks", cfg.installHotkeyHooks);
  set("patches", "syncpatches", cfg.installSyncPatches);
  set("patches", "objecttracker", cfg.installObjectTracker);
  set("patches", "game_version", cfg.installGameVersionHook);
  set("patches", "chatpatches", cfg.installChatPatches);
  set("patches", "loadingscreenhooks", cfg.installLoadingScreenHooks);
  set("patches", "transitionscreenhooks", cfg.installTransitionScreenHooks);
  set("patches", "officersorthooks", cfg.installOfficerSortHooks);
  set("patches", "cargoformathooks", cfg.installCargoFormatHooks);
  set("patches", "focussearch", cfg.installFocusSearchHooks);

  // graphics
  set("graphics", "ui_scale", cfg.ui_scale);
  set("graphics", "ui_scale_viewer", cfg.ui_scale_viewer);
  set("graphics", "adjust_scale_res", cfg.adjust_scale_res);
  set("graphics", "allow_cursor", cfg.allow_cursor);
  set("graphics", "zoom", cfg.zoom);
  set("graphics", "fr_scale", cfg.fr_scale);
  set("graphics", "default_system_zoom", cfg.default_system_zoom);
  set("graphics", "keyboard_zoom_speed", cfg.keyboard_zoom_speed);
  set("graphics", "system_zoom_preset_1", cfg.system_zoom_preset_1);
  set("graphics", "system_zoom_preset_2", cfg.system_zoom_preset_2);
  set("graphics", "system_zoom_preset_3", cfg.system_zoom_preset_3);
  set("graphics", "system_zoom_preset_4", cfg.system_zoom_preset_4);
  set("graphics", "system_zoom_preset_5", cfg.system_zoom_preset_5);
  set("graphics", "system_pan_momentum", cfg.system_pan_momentum);
  set("graphics", "system_pan_momentum_falloff", cfg.system_pan_momentum_falloff);
  set("graphics", "transition_time", cfg.transition_time);
  set("graphics", "borderless_fullscreen", cfg.borderless_fullscreen);
  set("graphics", "show_all_resolutions", cfg.show_all_resolutions);
  set("graphics", "loader_enabled", cfg.loader_enabled);
  set("graphics", "loader_transition", cfg.loader_transition);
  set("graphics", "loader_transition_black", cfg.loader_transition_black);
  set("graphics", "loader_logo_scale", cfg.loader_logo_scale);
  set("graphics", "loader_tip_enabled", cfg.loader_tip_enabled);

  // control
  set("control", "hotkeys_enabled", cfg.hotkeys_enabled);
  set("control", "hotkeys_extended", cfg.hotkeys_extended);
  set("control", "use_scopely_hotkeys", cfg.use_scopely_hotkeys);
  set("control", "use_presets_as_default", cfg.use_presets_as_default);
  set("control", "select_timer", cfg.select_timer);

  // ui
  set("ui", "disable_move_keys", cfg.disable_move_keys);
  set("ui", "disable_preview_locate", cfg.disable_preview_locate);
  set("ui", "disable_preview_recall", cfg.disable_preview_recall);
  set("ui", "disable_escape_exit", cfg.disable_escape_exit);
  set("ui", "disable_galaxy_chat", cfg.disable_galaxy_chat);
  set("ui", "disable_veil_chat", cfg.disable_veil_chat);
  set("ui", "disable_first_popup", cfg.disable_first_popup);
  set("ui", "disable_toast_banners", cfg.disable_toast_banners);
  set("ui", "auto_open_bulk_claim_flyout", cfg.auto_open_bulk_claim_flyout);
  set("ui", "show_cargo_default", cfg.show_cargo_default);
  set("ui", "show_player_cargo", cfg.show_player_cargo);
  set("ui", "show_station_cargo", cfg.show_station_cargo);
  set("ui", "show_hostile_cargo", cfg.show_hostile_cargo);
  set("ui", "show_armada_cargo", cfg.show_armada_cargo);
  set("ui", "cargo_significant_decimals", cfg.cargo_significant_decimals);
  set("ui", "always_skip_reveal_sequence", cfg.always_skip_reveal_sequence);
  set("ui", "extend_donation_slider", cfg.extend_donation_slider);
  set("ui", "extend_donation_max", cfg.extend_donation_max);

  // sync
  set("sync", "logging", cfg.sync_logging);
  set("sync", "debug", cfg.sync_debug);

  // buffs
  set("buffs", "use_out_of_dock_power", cfg.use_out_of_dock_power);

  Config::Save(tbl, File::Config(), false);
  spdlog::info("Overlay: config saved to {}", configPath);
}

static void ExportDiagnostics()
{
  auto path = std::string(File::MakePath("diagnostics.txt", true));
  std::ofstream out(path);
  if (!out.is_open()) {
    spdlog::error("Overlay: failed to write diagnostics to {}", path);
    return;
  }

  out << "STFC Community Mod Diagnostics Export" << std::endl;
  out << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
  out << std::endl;

  // Version
#if VERSION_PATCH
  out << "Mod version: " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION
      << " (Patch " << VERSION_PATCH << ")" << std::endl;
#else
  out << "Mod version: " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION << std::endl;
#endif
  out << std::endl;

  // Hook health
  out << "=== Hook Health ===" << std::endl;
  auto entries = HookHealth::GetEntries();
  for (const auto& e : entries) {
    out << "  " << e.name << ": " << StatusToString(e.status);
    if (!e.error_msg.empty()) out << " - " << e.error_msg;
    out << std::endl;
  }
  out << std::endl;

  // Sync stats
  out << "=== Sync Stats ===" << std::endl;
  auto stats = SyncStats::GetStats();
  for (const auto& s : stats) {
    out << "  " << s.name << ": sent=" << s.total_sent
        << " errors=" << s.total_errors
        << " queued=" << s.queue_depth
        << " bytes=" << s.total_bytes
        << " status=" << s.last_status_code << std::endl;
  }
  out << std::endl;

  // Config
  out << "=== Config ===" << std::endl;
  auto& cfg = Config::Get();
  out << "  installUiScaleHooks: " << cfg.installUiScaleHooks << std::endl;
  out << "  installZoomHooks: " << cfg.installZoomHooks << std::endl;
  out << "  installBuffFixHooks: " << cfg.installBuffFixHooks << std::endl;
  out << "  installToastBannerHooks: " << cfg.installToastBannerHooks << std::endl;
  out << "  installPanHooks: " << cfg.installPanHooks << std::endl;
  out << "  installHotkeyHooks: " << cfg.installHotkeyHooks << std::endl;
  out << "  installSyncPatches: " << cfg.installSyncPatches << std::endl;
  out << "  installObjectTracker: " << cfg.installObjectTracker << std::endl;
  out << "  installGameVersionHook: " << cfg.installGameVersionHook << std::endl;
  out << "  installChatPatches: " << cfg.installChatPatches << std::endl;
  out << "  installLoadingScreenHooks: " << cfg.installLoadingScreenHooks << std::endl;
  out << "  installTransitionScreenHooks: " << cfg.installTransitionScreenHooks << std::endl;
  out << "  installOfficerSortHooks: " << cfg.installOfficerSortHooks << std::endl;
  out << "  installCargoFormatHooks: " << cfg.installCargoFormatHooks << std::endl;
  out << "  installFocusSearchHooks: " << cfg.installFocusSearchHooks << std::endl;
  out << "  cargo_significant_decimals: " << cfg.cargo_significant_decimals << std::endl;
  out << "  sync_logging: " << cfg.sync_logging << std::endl;
  out << "  sync_debug: " << cfg.sync_debug << std::endl;
  out << std::endl;

  // Recent logs
  out << "=== Recent Logs (last 100) ===" << std::endl;
  auto logs = OverlayLog::GetRecent(100);
  for (const auto& entry : logs) {
    out << entry.text;
  }

  out.close();
  spdlog::info("Overlay: diagnostics exported to {}", path);
}

} // namespace Overlay
