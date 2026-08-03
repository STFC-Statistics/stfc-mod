#include "config.h"
#include "errormsg.h"
#include "patches/notification_service.h"

#include <il2cpp/il2cpp_helper.h>
#include <prime/Toast.h>

#include <spud/detour.h>

#if _MODDBG
#include "overlay/activity_feed.h"
#include <cstdio>
#endif

struct ToastObserver {
};

void ToastObserver_EnqueueToast_Hook(auto original, ToastObserver *_this, Toast *toast)
{
  if (toast == nullptr) {
    return original(_this, toast);
  }

#if _MODDBG
  {
    auto state = toast->get_State();
    char summary[256];
    std::snprintf(summary, sizeof(summary), "Toast: %s (%d)", ActivityFeed::ToastStateName(state), state);
    char detail[512];
    std::snprintf(detail, sizeof(detail), "State: %d (%s)\nSource: EnqueueToast", state, ActivityFeed::ToastStateName(state));
    ActivityFeed::Add(ActivityFeed::Category::Toast, summary, detail);
  }
#endif

  notification_handle_toast(toast);

  if (std::ranges::find(Config::Get().disabled_banner_types, toast->get_State())
      != Config::Get().disabled_banner_types.end()) {
    return;
  }

  original(_this, toast);
}

void ToastObserver_EnqueueOrCombineToast_Hook(auto original, ToastObserver *_this, Toast *toast, uintptr_t cmpAction)
{
  if (toast == nullptr) {
    return original(_this, toast, cmpAction);
  }

#if _MODDBG
  {
    auto state = toast->get_State();
    char summary[256];
    std::snprintf(summary, sizeof(summary), "Toast: %s (%d) [combined]", ActivityFeed::ToastStateName(state), state);
    char detail[512];
    std::snprintf(detail, sizeof(detail), "State: %d (%s)\nSource: EnqueueOrCombineToast", state, ActivityFeed::ToastStateName(state));
    ActivityFeed::Add(ActivityFeed::Category::Toast, summary, detail);
  }
#endif

  notification_handle_toast(toast);

  if (std::ranges::find(Config::Get().disabled_banner_types, toast->get_State())
      != Config::Get().disabled_banner_types.end()) {
    return;
  }

  original(_this, toast, cmpAction);
}

void InstallToastBannerHooks()
{
  notification_init();

  if (auto helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "ToastObserver");
      !helper.isValidHelper()) {
    ErrorMsg::MissingHelper("HUD", "ToastObserver");
  } else {
    if (const auto ptr = helper.GetMethod("EnqueueToast"); ptr == nullptr) {
      ErrorMsg::MissingMethod("ToastObserver", "EnqueueToast");
    } else {
      SPUD_STATIC_DETOUR(ptr, ToastObserver_EnqueueToast_Hook);
    }

    if (const auto ptr = helper.GetMethod("EnqueueOrCombineToast"); ptr == nullptr) {
      ErrorMsg::MissingMethod("ToastObserver", "EnqueueOrCombineToast");
    } else {
      SPUD_STATIC_DETOUR(ptr, ToastObserver_EnqueueOrCombineToast_Hook);
    }
  }
}
