#include "config.h"
#include "errormsg.h"
#include "patches/notification_service.h"
#include "str_utils.h"
#include "toast_observer_cache.h"

#include <il2cpp/il2cpp_helper.h>
#include <prime/Toast.h>

#include <spdlog/spdlog.h>
#include <spud/detour.h>

struct ToastObserver {
};

// Helper: safely read an Il2CppObject* field at a given offset and convert to string
static std::string safe_read_string_field(void* obj, ptrdiff_t offset)
{
  if (!obj) return {};
  try {
    auto* str = *reinterpret_cast<Il2CppString**>(reinterpret_cast<char*>(obj) + offset);
    if (!str) return {};
    return to_string(str);
  } catch (...) {
    return {};
  }
}

static void log_toast_details(const char* hookName, ToastObserver* _this, Toast* toast)
{
  if (!_this || !toast) return;

  // Observer concrete subclass name
  const char* observerName = "unknown";
  try {
    auto* obj = reinterpret_cast<Il2CppObject*>(_this);
    if (obj && obj->klass && obj->klass->name) {
      observerName = obj->klass->name;
    }
  } catch (...) {
  }

  int state = -1;
  try {
    state = toast->get_State();
  } catch (...) {
  }

  void* ltc = nullptr;
  try {
    ltc = toast->get_TextLocaleTextContext();
  } catch (...) {
  }

  std::string identifier;
  std::string category;
  if (ltc) {
    identifier = safe_read_string_field(ltc, 0x10); // _identifier
    category     = safe_read_string_field(ltc, 0x20); // _category
  }

  // TextParameters count
  int paramCount = 0;
  try {
    auto* params = toast->get_TextParameters();
    if (params) {
      paramCount = static_cast<int>(reinterpret_cast<Il2CppArraySize*>(params)->max_length);
    }
  } catch (...) {
  }

  spdlog::info("[ToastLogger] {}: observer={}, state={}, id=\"{}\", cat=\"{}\", params={}",
               hookName, observerName, state, identifier, category, paramCount);
}

static bool is_internal_test_toast(Toast* toast)
{
  if (!toast) return false;
  try {
    auto* ltc = toast->get_TextLocaleTextContext();
    if (!ltc) return false;
    auto id  = safe_read_string_field(ltc, 0x10); // _identifier
    auto cat = safe_read_string_field(ltc, 0x20); // _category
    return (cat == "kill_tracker") || (id == "hello world" && cat == "kill_tracker");
  } catch (...) {
    return false;
  }
}

void ToastObserver_EnqueueToast_Hook(auto original, ToastObserver *_this, Toast *toast)
{
  // Cache the observer for use by other patches (e.g. kill_tracker)
  if (_this) {
    g_cached_toast_observer = _this;
  }

  log_toast_details("EnqueueToast", _this, toast);

  if (!is_internal_test_toast(toast)) {
    notification_handle_toast(toast);
  }

  if (std::ranges::find(Config::Get().disabled_banner_types, toast->get_State())
      != Config::Get().disabled_banner_types.end()) {
    return;
  }

  original(_this, toast);
}

void ToastObserver_EnqueueOrCombineToast_Hook(auto original, ToastObserver *_this, Toast *toast, uintptr_t cmpAction)
{
  // Cache the observer for use by other patches (e.g. kill_tracker)
  if (_this) {
    g_cached_toast_observer = _this;
  }

  log_toast_details("EnqueueOrCombineToast", _this, toast);

  if (!is_internal_test_toast(toast)) {
    notification_handle_toast(toast);
  }

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
