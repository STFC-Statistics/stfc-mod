#include "main_thread_queue.h"

#include <il2cpp/il2cpp_helper.h>

#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <mutex>
#include <vector>

namespace MainThreadQueue
{
static std::mutex                         s_mutex;
static std::vector<std::function<void()>> s_queue;

void Enqueue(std::function<void()> action)
{
  std::scoped_lock lk{s_mutex};
  s_queue.emplace_back(std::move(action));
}

static void Drain()
{
  std::vector<std::function<void()>> pending;
  {
    std::scoped_lock lk{s_mutex};
    if (s_queue.empty()) {
      return;
    }
    pending.swap(s_queue);
  }

  for (auto& action : pending) {
    try {
      action();
    } catch (const std::exception& e) {
      spdlog::error("MainThreadQueue: action threw exception: {}", e.what());
    } catch (...) {
      spdlog::error("MainThreadQueue: action threw unknown exception");
    }
  }
}

// ScreenManager::LateUpdate is not hooked anywhere else in the codebase.
// ScreenManager::Update is already hooked by hotkeys.cc; we deliberately use
// a different method on the same always-present singleton to avoid stacking
// two detours on the same underlying function.
static void ScreenManager_LateUpdate_Hook(auto original, void* _this)
{
  original(_this);
  Drain();
}

void Install()
{
  auto helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "ScreenManager");
  if (!helper.isValidHelper()) {
    spdlog::warn("MainThreadQueue: ScreenManager class not found; overlay actions will not run");
    return;
  }

  auto ptr = helper.GetMethod("LateUpdate");
  if (!ptr) {
    spdlog::warn("MainThreadQueue: ScreenManager::LateUpdate not found; overlay actions will not run");
    return;
  }

  SPUD_STATIC_DETOUR(ptr, ScreenManager_LateUpdate_Hook);
  spdlog::info("MainThreadQueue: installed on ScreenManager::LateUpdate");
}
}
