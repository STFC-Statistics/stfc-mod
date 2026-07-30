#include "config.h"
#include "errormsg.h"
#include "toast_observer_cache.h"

#include <il2cpp/il2cpp_helper.h>
#include <il2cpp-object-internals.h>
#include <prime/CallbackContainer.h>
#include <prime/Hub.h>
#include <prime/MonoSingleton.h>
#include <prime/PlayerProfileManager.h>
#include <prime/Toast.h>
#include <prime/Tracker.h>

#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <atomic>

#if _WIN32
#include <Windows.h>
#endif

// Global cached ToastObserver — set by disable_banners.cc hook
// Must be outside anonymous namespace for external linkage.
void* g_cached_toast_observer = nullptr;

namespace {

// ---------------------------------------------------------------------------
// SEH wrapper — catches access violations from bad IL2CPP pointers
// ---------------------------------------------------------------------------
template <typename Fn>
static bool seh_call(Fn fn)
{
#if _WIN32
  __try {
    fn();
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
#else
  fn();
  return true;
#endif
}

// ---------------------------------------------------------------------------
// Section guard — only run the auto-fetch while docked in the main starbase
// ---------------------------------------------------------------------------
static bool is_in_main_starbase_section()
{
  auto* sm = Hub::get_SectionManager();
  if (!sm) return false;
  auto section = sm->CurrentSection;
  return section == SectionID::Starbase_Exterior;
}

static const MethodInfo* find_fetch_kill_counter_completion(Il2CppClass* service_class)
{
  if (!service_class) return nullptr;

  void* iter = nullptr;
  while (auto* nested = il2cpp_class_get_nested_types(service_class, &iter)) {
    auto* method = il2cpp_class_get_method_from_name(nested, "<FetchKillCounterStat>b__0", 1);
    if (method && method->methodPointer) {
      spdlog::info("[KillTracker] FetchKillCounterStat completion resolved on nested class {}", nested->name);
      return method;
    }
  }

  return nullptr;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
struct KillTrackerState {
  int                            lastNotifiedKillCount{-1};
  std::chrono::steady_clock::time_point lastPollTime;
  std::chrono::steady_clock::time_point lastFetchTime;
  bool                           initialised{false};
};

KillTrackerState s_state;

// Latest tracker values written by the network response hooks and consumed on
// the main thread by kill_tracker_poll. This prevents Unity API calls from a
// background networking thread, which is a common crash source.
struct LatestTracker {
  std::atomic<int>  counter{0};
  std::atomic<int>  limit{0};
  std::atomic<bool> has_new{false};
};
LatestTracker s_latest_tracker;

static void KillTrackerOnSuccessNoOp(void*, Tracker* tracker, const MethodInfo*)
{
  if (!tracker) return;

  int counter = 0;
  int limit = 0;
  if (!seh_call([&] {
        counter = tracker->get_Counter();
        limit = tracker->get_Limit();
      }))
  {
    return;
  }

  if (counter >= 0 && limit >= 0 && (limit == 0 || counter <= limit)) {
    s_latest_tracker.counter.store(counter);
    s_latest_tracker.limit.store(limit);
    s_latest_tracker.has_new.store(true);
  }
}

static void KillTrackerOnErrorNoOp(void*, void*, const MethodInfo*) {}

// Cached IL2CPP helpers
Il2CppClass*      s_toast_class{nullptr};
Il2CppClass*      s_ltc_class{nullptr};
const MethodInfo* s_ltc_ctor3{nullptr};         // .ctor(string, string, bool) — sets OverrideBaseIdentifier
const MethodInfo* s_ltc_ctor2{nullptr};         // .ctor(string, string) — fallback
const MethodInfo* s_ltc_apply_id_params{nullptr}; // ApplyIdentifierParameters(object[])
const MethodInfo* s_toast_ctor7{nullptr};       // .ctor(ToastState, LTC, string, Action, GameEvents, object, object[])
const MethodInfo* s_toast_ctor0{nullptr};       // .ctor() — fallback
Il2CppClass*      s_object_array_class{nullptr};
Il2CppClass*      s_int32_class{nullptr};
Il2CppClass*      s_callback_container_class{nullptr}; // CallbackContainer`1[Tracker]
Il2CppClass*      s_on_success_class{nullptr};        // OnSuccess<Tracker> delegate class
Il2CppClass*      s_on_error_class{nullptr};          // OnError delegate class
Il2CppClass*      s_on_retry_class{nullptr};          // OnRetry delegate class
FieldInfo*        s_callback_on_success_field{nullptr};
FieldInfo*        s_callback_on_error_field{nullptr};
Il2CppObject*     s_kill_tracker_callback_container{nullptr}; // our auto-fetch callbacks container
Il2CppGCHandle    s_kill_tracker_callback_container_handle{0}; // strong pinned handle to keep the container alive
static const MethodInfo* s_keep_alive_method{nullptr}; // System.GC::KeepAlive(object) no-op target
static bool       s_auto_fetch_crashed{false};           // disable fetch after a crash in FetchPlayerKillCounter
static bool       s_auto_fetch_response_hooked{false};
IL2CppClassHelper s_ltc_helper_cached{nullptr};
IL2CppClassHelper s_toast_helper_cached{nullptr};
ptrdiff_t         s_view_controller_offset{-1};        // ToastObserver._viewController field offset
Il2CppClass*      s_toast_observer_class{nullptr};      // Exact base ToastObserver class (not derived subclasses)

void ensure_cache()
{
  if (s_toast_class) return;

  spdlog::info("[KillTracker] ensure_cache() — resolving IL2CPP classes...");

  // Toast class
  auto toast_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "Toast");
  if (toast_helper.isValidHelper()) {
    s_toast_class = toast_helper.get_cls();
    spdlog::info("[KillTracker]   Toast class resolved: {}", (void*)s_toast_class);
  } else {
    spdlog::warn("[KillTracker]   Failed to resolve Toast class");
  }

  // LocaleTextContext class
  auto ltc_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "LocaleTextContext");
  if (ltc_helper.isValidHelper()) {
    s_ltc_class = ltc_helper.get_cls();
    s_ltc_helper_cached = ltc_helper;
    s_ltc_ctor3 = ltc_helper.GetMethodInfo(".ctor", 3);
    s_ltc_ctor2 = ltc_helper.GetMethodInfo(".ctor", 2);
    s_ltc_apply_id_params = ltc_helper.GetMethodInfo("ApplyIdentifierParameters", 1);
    spdlog::info("[KillTracker]   LocaleTextContext class: {}, ctor3: {}, ctor2: {}, applyIdParams: {}",
                 (void*)s_ltc_class, (void*)s_ltc_ctor3, (void*)s_ltc_ctor2, (void*)s_ltc_apply_id_params);
  } else {
    spdlog::warn("[KillTracker]   Failed to resolve LocaleTextContext class");
  }

  // Toast class — cache helper and constructors
  if (toast_helper.isValidHelper()) {
    s_toast_helper_cached = toast_helper;
    s_toast_ctor7 = toast_helper.GetMethodInfo(".ctor", 7);
    s_toast_ctor0 = toast_helper.GetMethodInfo(".ctor", 0);
    spdlog::info("[KillTracker]   Toast ctor7: {}, ctor0: {}", (void*)s_toast_ctor7, (void*)s_toast_ctor0);
  }

  // ToastObserver class and _viewController field offset (used to validate a live observer)
  auto observer_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "ToastObserver");
  if (observer_helper.isValidHelper()) {
    s_toast_observer_class = observer_helper.get_cls();
    auto field = observer_helper.GetField("_viewController");
    if (field.isValidHelper()) {
      s_view_controller_offset = field.offset();
      spdlog::info("[KillTracker]   ToastObserver class: {}, _viewController offset: {}",
                   (void*)s_toast_observer_class, s_view_controller_offset);
    } else {
      spdlog::warn("[KillTracker]   Failed to resolve ToastObserver._viewController field");
    }
  } else {
    spdlog::warn("[KillTracker]   Failed to resolve ToastObserver class");
  }

  // System.Object[] array class
  auto obj_helper = il2cpp_get_class_helper("mscorlib", "System", "Object");
  if (obj_helper.isValidHelper()) {
    s_object_array_class = il2cpp_array_class_get(obj_helper.get_cls(), 1);
    spdlog::info("[KillTracker]   Object[] array class: {}", (void*)s_object_array_class);
  } else {
    spdlog::warn("[KillTracker]   Failed to resolve System.Object class");
  }

  // System.Int32 class (for boxing)
  auto i32_helper = il2cpp_get_class_helper("mscorlib", "System", "Int32");
  if (i32_helper.isValidHelper()) {
    s_int32_class = i32_helper.get_cls();
    spdlog::info("[KillTracker]   Int32 class: {}", (void*)s_int32_class);
  } else {
    spdlog::warn("[KillTracker]   Failed to resolve System.Int32 class");
  }

  // Find CallbackContainer`1[Tracker] by inspecting FetchPlayerKillCounter's parameter type
  auto ppm_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.PlayerProfile", "PlayerProfileManager");
  if (ppm_helper.isValidHelper()) {
    auto* method = ppm_helper.GetMethodInfo("FetchPlayerKillCounter", 1);
    if (method && method->parameters_count >= 1 && method->parameters) {
      auto* param_type = method->parameters[0];
      if (param_type) {
        s_callback_container_class = il2cpp_class_from_type(param_type);
        spdlog::info("[KillTracker]   CallbackContainer<Tracker> class: {}", (void*)s_callback_container_class);

  // Derive OnSuccess<Tracker> from the _onSuccess field so we can build a no-op delegate later.
  if (s_callback_container_class) {
    auto fill_delegate_class = [&](const char* fieldName, Il2CppClass*& outClass) {
      auto* field = il2cpp_class_get_field_from_name(s_callback_container_class, fieldName);
      if (field) {
        auto* t = il2cpp_field_get_type(field);
        if (t) outClass = il2cpp_class_from_type(t);
      }
    };
    fill_delegate_class("_onSuccess", s_on_success_class);
    fill_delegate_class("_onError",   s_on_error_class);
    fill_delegate_class("_onRetry",   s_on_retry_class);
    spdlog::info("[KillTracker]   OnSuccess<Tracker> class: {}", (void*)s_on_success_class);
    spdlog::info("[KillTracker]   OnError class: {}", (void*)s_on_error_class);
    spdlog::info("[KillTracker]   OnRetry class: {}", (void*)s_on_retry_class);

    // Locate a real managed no-op we can use as the delegate target.
    // System.GC.KeepAlive(object) is static, does nothing, and has the same ABI as any single-reference-arg callback.
    auto gc_helper = il2cpp_get_class_helper("mscorlib", "System", "GC");
    if (gc_helper.isValidHelper()) {
      s_keep_alive_method = gc_helper.GetMethodInfo("KeepAlive", 1);
      spdlog::info("[KillTracker]   GC.KeepAlive(object) MethodInfo: {}", (void*)s_keep_alive_method);
    }
  }
      } else {
        spdlog::warn("[KillTracker]   FetchPlayerKillCounter param[0] type is null");
      }
    } else {
      spdlog::warn("[KillTracker]   FetchPlayerKillCounter method not found or no params (method={}, count={})",
                   (void*)method, method ? (int)method->parameters_count : -1);
    }
  } else {
    spdlog::warn("[KillTracker]   Failed to resolve PlayerProfileManager class");
  }

  if (!s_toast_class || !s_ltc_class || !s_ltc_ctor2 || !s_object_array_class) {
    spdlog::warn("[KillTracker] Failed to resolve some IL2CPP classes — toast creation may fail");
  }
  if (!s_callback_container_class || !s_on_success_class) {
    spdlog::warn("[KillTracker] Could not resolve CallbackContainer<Tracker> / OnSuccess<Tracker> — active polling may fail");
  }
  spdlog::info("[KillTracker] ensure_cache() complete");
}

// ---------------------------------------------------------------------------
// Observer validation / lookup
// ---------------------------------------------------------------------------
static void* read_observer_view_controller(void* observer)
{
  if (!observer || s_view_controller_offset < 0) return nullptr;
  return *reinterpret_cast<void**>(reinterpret_cast<char*>(observer) + s_view_controller_offset);
}

static bool is_base_toast_observer(void* obs)
{
  if (!obs || !s_toast_observer_class) return false;
  auto* obj = reinterpret_cast<Il2CppObject*>(obs);
  return obj && obj->klass && obj->klass == s_toast_observer_class;
}

static void* find_valid_toast_observer()
{
  // Prefer the observer cached by disable_banners.cc, but only if it is the base
  // ToastObserver (not a derived subclass like ToastTournamentObserver) and still
  // has a live _viewController.
  if (g_cached_toast_observer && is_base_toast_observer(g_cached_toast_observer)
      && read_observer_view_controller(g_cached_toast_observer)) {
    return g_cached_toast_observer;
  }

  // Fall back to UnityEngine.Object.FindObjectsOfType(typeof(ToastObserver))
  // This returns all ToastObserver subclasses too, so we must filter for the
  // exact base class — derived observers have their own view controllers that
  // don't handle ToastState.Standard.
  auto toast_observer_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "ToastObserver");
  auto* observer_type = toast_observer_helper.GetType();
  if (!observer_type) {
    spdlog::warn("[KillTracker] find_valid_toast_observer: failed to get ToastObserver System.Type");
    return nullptr;
  }

  auto unity_obj_helper = il2cpp_get_class_helper("UnityEngine.CoreModule", "UnityEngine", "Object");
  auto* find_method = unity_obj_helper.GetMethodInfo("FindObjectsOfType", 1);
  if (!find_method) {
    spdlog::warn("[KillTracker] find_valid_toast_observer: failed to resolve FindObjectsOfType");
    return nullptr;
  }

  void* find_params[1] = {observer_type};
  Il2CppException* findExc = nullptr;
  auto* result_array = il2cpp_runtime_invoke(find_method, nullptr, find_params, &findExc);
  if (findExc || !result_array) {
    spdlog::warn("[KillTracker] find_valid_toast_observer: FindObjectsOfType failed");
    return nullptr;
  }

  auto* arr = reinterpret_cast<Il2CppArraySize*>(result_array);
  auto arr_len = arr->max_length;
  spdlog::info("[KillTracker] find_valid_toast_observer: FindObjectsOfType returned {} observer(s)", arr_len);

  auto* observers = reinterpret_cast<void**>(arr->vector);
  for (il2cpp_array_size_t i = 0; i < arr_len; ++i) {
    void* obs = observers[i];
    if (!obs) continue;

    const char* obsName = "unknown";
    try {
      auto* obj = reinterpret_cast<Il2CppObject*>(obs);
      if (obj && obj->klass && obj->klass->name) obsName = obj->klass->name;
    } catch (...) {
    }

    if (!is_base_toast_observer(obs)) {
      spdlog::info("[KillTracker] find_valid_toast_observer: observer[{}] is {} (derived), skipping", i, obsName);
      continue;
    }

    if (read_observer_view_controller(obs)) {
      spdlog::info("[KillTracker] find_valid_toast_observer: observer[{}] is base ToastObserver with valid view controller, caching it", i);
      g_cached_toast_observer = obs;
      return obs;
    } else {
      spdlog::info("[KillTracker] find_valid_toast_observer: observer[{}] is base ToastObserver but no valid view controller", i);
    }
  }

  spdlog::warn("[KillTracker] find_valid_toast_observer: no base ToastObserver with valid view controller found");
  return nullptr;
}

// ---------------------------------------------------------------------------
// Toast creation and enqueue
// ---------------------------------------------------------------------------
static void show_toast_internal(const char* identifier, const char* category, int state)
{
  ensure_cache();
  if (!s_toast_class || !s_ltc_class || !s_ltc_ctor2) {
    spdlog::warn("[KillTracker] Cannot create toast — missing IL2CPP classes (toast={}, ltc={}, ctor2={})",
                 (void*)s_toast_class, (void*)s_ltc_class, (void*)s_ltc_ctor2);
    return;
  }

  if (!seh_call([&] {
        // Create IL2CPP strings
        auto* idStr  = il2cpp_string_new(identifier);
        auto* catStr = il2cpp_string_new(category);
        if (!idStr || !catStr) {
          spdlog::warn("[KillTracker]   Failed to create IL2CPP strings");
          return;
        }

        // Create LocaleTextContext(identifier, category) then set OverrideBaseIdentifier=true
        auto* ltc = il2cpp_object_new(s_ltc_class);
        if (!ltc) {
          spdlog::warn("[KillTracker]   Failed to allocate LocaleTextContext");
          return;
        }
        spdlog::info("[KillTracker]   ltc allocated at {}", (void*)ltc);

        void* ltcCtorParams[2] = {idStr, catStr};
        Il2CppException* ltcExc = nullptr;
        il2cpp_runtime_invoke(s_ltc_ctor2, ltc, ltcCtorParams, &ltcExc);
        if (ltcExc) {
          spdlog::warn("[KillTracker]   LocaleTextContext .ctor(2) threw exception");
          return;
        }
        spdlog::info("[KillTracker]   ltc .ctor(2) invoked OK");

        // Set OverrideBaseIdentifier = true so TextLocalizer displays the identifier as-is
        static auto overrideProp = s_ltc_helper_cached.GetProperty("OverrideBaseIdentifier");
        if (overrideProp.isValidHelper()) {
          bool overrideVal = true;
          overrideProp.SetRaw(ltc, overrideVal);
          spdlog::info("[KillTracker]   OverrideBaseIdentifier set to true");
        } else {
          spdlog::warn("[KillTracker]   Could not resolve OverrideBaseIdentifier property");
        }

        // Create Toast using parameterized constructor
        auto* toast = il2cpp_object_new(s_toast_class);
        if (!toast) {
          spdlog::warn("[KillTracker]   Failed to allocate Toast");
          return;
        }
        spdlog::info("[KillTracker]   toast allocated at {}", (void*)toast);

        int32_t stateVal = static_cast<int32_t>(state);
        // TOASTS_SHOW_NEUTRAL = 1243600485 — a valid GameEvents enum value for neutral toasts.
        // Using 0 causes a NullReferenceException in HashedGameEvent..ctor inside
        // UpdateFromCurrentToast → GameEventMessenger.PostEvent, which kills the
        // CheckToastQueue coroutine and stops all future toasts.
        int32_t gameEventVal = 1243600485;
        auto* textParameters = il2cpp_array_new_specific(s_object_array_class, 0);
        if (!textParameters) {
          spdlog::warn("[KillTracker]   Failed to allocate Toast.TextParameters array");
          return;
        }

        if (s_toast_ctor7) {
          void* toastCtorParams[7] = {
            &stateVal,        // ToastState (int32, by ref)
            ltc,              // LocaleTextContext (object)
            nullptr,          // string iconIdentifier
            nullptr,          // Action<Toast> callback
            &gameEventVal,    // GameEvents (int32, by ref)
            nullptr,          // object data
            textParameters    // object[] textParameters
          };
          Il2CppException* toastExc = nullptr;
          il2cpp_runtime_invoke(s_toast_ctor7, toast, toastCtorParams, &toastExc);
          if (toastExc) {
            spdlog::warn("[KillTracker]   Toast .ctor(7) threw exception");
            return;
          }
          spdlog::info("[KillTracker]   Toast .ctor(7) invoked OK");
        } else if (s_toast_ctor0) {
          Il2CppException* toastExc = nullptr;
          il2cpp_runtime_invoke(s_toast_ctor0, toast, nullptr, &toastExc);
          if (toastExc) {
            spdlog::warn("[KillTracker]   Toast .ctor(0) threw exception");
            return;
          }
          spdlog::info("[KillTracker]   Toast .ctor(0) invoked OK (fallback)");

          static auto state_prop = s_toast_helper_cached.GetProperty("State");
          if (state_prop.isValidHelper()) {
            state_prop.SetRaw(toast, stateVal);
            spdlog::info("[KillTracker]   Toast.State set via property setter");
          }

          static auto ltc_prop = s_toast_helper_cached.GetProperty("TextLocaleTextContext");
          if (ltc_prop.isValidHelper()) {
            ltc_prop.SetRaw(toast, ltc);
            spdlog::info("[KillTracker]   Toast.TextLocaleTextContext set via property setter");
          }

          // ShowEvent is a public field at offset 0x50, not a property — set it directly.
          // Must not be 0 (causes NullReferenceException in HashedGameEvent..ctor).
          *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(toast) + 0x50) = 1243600485;
          spdlog::info("[KillTracker]   Toast.ShowEvent set to TOASTS_SHOW_NEUTRAL");
        } else {
          spdlog::warn("[KillTracker]   No Toast constructor available");
          return;
        }

        // Find a live ToastObserver with a valid _viewController
        auto toast_observer_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "ToastObserver");
        static auto* enqueue_method_info = toast_observer_helper.GetMethodInfo("EnqueueToast", 1);
        if (!enqueue_method_info) {
          spdlog::warn("[KillTracker]   Could not resolve EnqueueToast method");
          return;
        }

        void* observer = find_valid_toast_observer();
        if (!observer) {
          spdlog::warn("[KillTracker]   No valid ToastObserver with live view controller — skipping toast");
          return;
        }
        spdlog::info("[KillTracker]   using observer at {}", (void*)observer);

        // Call EnqueueToast via direct method pointer (IL2CPP calling convention)
        using EnqueueToastFn = void(*)(void*, void*, const MethodInfo*);
        auto* enqueue_fn = reinterpret_cast<EnqueueToastFn>(enqueue_method_info->methodPointer);
        if (!enqueue_fn) {
          spdlog::warn("[KillTracker]   EnqueueToast methodPointer is null");
          return;
        }
        spdlog::info("[KillTracker]   EnqueueToast methodPointer: {}", (void*)enqueue_fn);
        enqueue_fn(observer, toast, enqueue_method_info);
        spdlog::info("[KillTracker]   EnqueueToast called — toast enqueued");
      }))
  {
    spdlog::warn("[KillTracker] SEH: toast creation/enqueue crashed");
  }
}

void show_hello_world_toast()
{
  spdlog::info("[KillTracker] show_hello_world_toast()");
  show_toast_internal("hello world", "kill_tracker", Standard);
}

void show_kill_toast(int currentKills, int limit)
{
  std::string msg = "Hostile kills: " + std::to_string(currentKills) + "/" + std::to_string(limit);
  spdlog::info("[KillTracker] show_kill_toast({}/{})", currentKills, limit);
  show_toast_internal(msg.c_str(), "kill_tracker", Standard);
}

// ---------------------------------------------------------------------------
// Threshold logic
// ---------------------------------------------------------------------------
void check_threshold(int currentKills, int limit)
{
  if (!s_state.initialised) {
    s_state.lastNotifiedKillCount = 0; // Notify on current progress the first time
    s_state.initialised = true;
    spdlog::info("[KillTracker] First tracker data — baseline reset to 0, current counter={}, limit={}",
                 currentKills, limit);
  }

  int delta = currentKills - s_state.lastNotifiedKillCount;
  int threshold = Config::Get().kill_tracker_threshold;
  spdlog::info("[KillTracker] check_threshold: kills={}, lastNotified={}, delta={}, threshold={}, limit={}",
               currentKills, s_state.lastNotifiedKillCount, delta, threshold, limit);

  if (delta <= 0) {
    if (delta < 0) {
      spdlog::info("[KillTracker] Kill count decreased (reset?): {} -> {}", s_state.lastNotifiedKillCount, currentKills);
    } else {
      spdlog::info("[KillTracker] No change in kill count ({}), skipping", currentKills);
    }
    s_state.lastNotifiedKillCount = currentKills;
    return;
  }

  if (delta >= threshold) {
    spdlog::info("[KillTracker] Threshold MET: delta {} >= threshold {} — showing toast", delta, threshold);
    show_kill_toast(currentKills, limit);
    s_state.lastNotifiedKillCount = currentKills;
  } else {
    spdlog::info("[KillTracker] Threshold not met: delta {} < threshold {} — skipping", delta, threshold);
  }
}

// ---------------------------------------------------------------------------
// TryPopTracker hook (backup — binary sync path)
// ---------------------------------------------------------------------------
struct PlayerKillCounterDataContainer {};

bool TryPopTracker_Hook(auto original, PlayerKillCounterDataContainer* _this, Tracker** outTracker)
{
  bool result = original(_this, outTracker);

  if (!result || !outTracker || !*outTracker) {
    spdlog::info("[KillTracker] TryPopTracker called but no data popped (result={}, outTracker={})",
                 (int)result, (void*)outTracker);
    return result;
  }

  int currentKills = 0;
  int limit = 0;

  if (!seh_call([&] {
        currentKills = (*outTracker)->get_Counter();
        limit = (*outTracker)->get_Limit();
      }))
  {
    spdlog::warn("[KillTracker] SEH: reading Tracker fields crashed");
    return result;
  }

  spdlog::info("[KillTracker] TryPopTracker popped: counter={}, limit={}, tracker ptr={}",
               currentKills, limit, (void*)*outTracker);

  // Validate hostile-kill tracker data per Phase 2A criteria.
  if (limit != 2500 || currentKills < 0 || currentKills > limit) {
    spdlog::info("[KillTracker] TryPopTracker data rejected: expected limit=2500, got limit={}, counter={} — ignoring",
                 limit, currentKills);
    return result;
  }

  // Defer to the main-thread poll; do not drive toasts from TryPopTracker.
  s_latest_tracker.counter.store(currentKills);
  s_latest_tracker.limit.store(limit);
  s_latest_tracker.has_new.store(true);
  return result;
}

// ---------------------------------------------------------------------------
// CallbackContainer<Tracker>.TriggerSuccess hook
// We install this so we can drive notifications from our own auto-fetch
// requests. For any other container we call through to the original.
// For our container we bypass original entirely because original would try
// to invoke a null OnSuccess<Tracker> delegate and crash.
// ---------------------------------------------------------------------------
struct CallbackContainer_Tracker {};

void TriggerSuccess_Hook(auto original, CallbackContainer_Tracker* _this, Tracker* tracker,
                         const MethodInfo* method)
{
  spdlog::info("[KillTracker] TriggerSuccess_Hook entered: _this={}, tracker={}, method={}",
                (void*)_this, (void*)tracker, (void*)method);

  // If this is not our auto-fetch container, leave the game's callback alone.
  if (reinterpret_cast<void*>(_this) != s_kill_tracker_callback_container) {
    original(_this, tracker, method);
    return;
  }

  // Our auto-fetch response — process the tracker directly.
  if (!tracker) {
    spdlog::warn("[KillTracker] TriggerSuccess on auto-fetch container: null tracker");
    return;
  }

  int currentKills = 0;
  int limit = 0;

  if (!seh_call([&] {
        currentKills = tracker->get_Counter();
        limit = tracker->get_Limit();
      }))
  {
    spdlog::warn("[KillTracker] SEH: reading Tracker fields in TriggerSuccess crashed");
    return;
  }

  spdlog::info("[KillTracker] auto-fetch TriggerSuccess received: counter={}, limit={}", currentKills, limit);
  if (currentKills >= 0 && limit >= 0 && (limit == 0 || currentKills <= limit)) {
    // Defer the actual toast creation to the main-thread poll so we don't call
    // Unity APIs from a networking callback thread.
    s_latest_tracker.counter.store(currentKills);
    s_latest_tracker.limit.store(limit);
    s_latest_tracker.has_new.store(true);
  } else {
    spdlog::info("[KillTracker] auto-fetch TriggerSuccess data rejected: limit={}, counter={}", limit, currentKills);
  }
}

// ---------------------------------------------------------------------------
// CallbackContainer<Tracker>.TriggerError hook
// Same idea: swallow errors for our auto-fetch container so the null OnError
// delegate is never invoked.
// ---------------------------------------------------------------------------
struct GSError {};

void TriggerError_Hook(auto original, CallbackContainer_Tracker* _this, GSError* error,
                       const MethodInfo* method)
{
  if (reinterpret_cast<void*>(_this) != s_kill_tracker_callback_container) {
    original(_this, error, method);
    return;
  }

  spdlog::debug("[KillTracker] auto-fetch TriggerError — ignoring");
}

// ---------------------------------------------------------------------------
// PlayerProfileViewController hook — clean kill counter data path
// The game calls this whenever the kill counter server response is processed,
// passing the actual Tracker payload. No generic CallbackContainer noise here.
// ---------------------------------------------------------------------------
void OnFetchPlayerKillCounterSuccess_Hook(auto original, void* _this, Tracker* tracker,
                                        const MethodInfo* method)
{
  spdlog::info("[KillTracker] OnFetchPlayerKillCounterSuccess called (controller={}, tracker={}, method={})",
               (void*)_this, (void*)tracker, (void*)method);

  if (tracker) {
    int currentKills = 0;
    int limit = 0;

    if (!seh_call([&] {
          currentKills = tracker->get_Counter();
          limit = tracker->get_Limit();
        }))
    {
      spdlog::warn("[KillTracker] SEH: reading Tracker fields in OnFetchPlayerKillCounterSuccess crashed");
    } else {
      spdlog::info("[KillTracker] OnFetchPlayerKillCounterSuccess received: counter={}, limit={}",
                   currentKills, limit);

      // Validate hostile-kill tracker data.
      if (currentKills >= 0 && limit >= 0 && (limit == 0 || currentKills <= limit)) {
        // Defer the actual toast creation to the main-thread poll.
        s_latest_tracker.counter.store(currentKills);
        s_latest_tracker.limit.store(limit);
        s_latest_tracker.has_new.store(true);
      } else {
        spdlog::info("[KillTracker] OnFetchPlayerKillCounterSuccess data rejected: limit={}, counter={} — ignoring",
                     limit, currentKills);
      }
    }
  } else {
    spdlog::warn("[KillTracker] OnFetchPlayerKillCounterSuccess called with null tracker");
  }

  original(_this, tracker, method);
}

// ---------------------------------------------------------------------------
// PlayerKillCounterService fetch-response hook — data from the tracker endpoint
// This compiler-generated callback is invoked when the server responds to a
// GET_PLAYER_KILL_COUNTER_TRACKER request. We read the freshly-parsed Tracker
// straight from the service's data container so notifications work without
// requiring the player profile view to be open.
// ---------------------------------------------------------------------------
void FetchKillCounterStat_Response_Hook(auto original, void* _this, bool requestSuccessful)
{
  auto* displayClass = reinterpret_cast<char*>(_this);
  if (!displayClass) {
    original(_this, requestSuccessful);
    return;
  }

  void* callbacks = nullptr;
  if (!seh_call([&] { callbacks = *reinterpret_cast<void**>(displayClass + 0x18); })) {
    original(_this, requestSuccessful);
    return;
  }

  if (callbacks != s_kill_tracker_callback_container) {
    original(_this, requestSuccessful);
    return;
  }

  int counter = 0;
  int limit = 0;
  if (requestSuccessful && seh_call([&] {
        auto* service = *reinterpret_cast<char**>(displayClass + 0x10);
        if (!service) return;
        auto* dataContainer = *reinterpret_cast<char**>(service + 0x70);
        if (!dataContainer) return;
        auto* tracker = *reinterpret_cast<char**>(dataContainer + 0x10);
        if (!tracker) return;
        counter = *reinterpret_cast<int*>(tracker + 0x18);
        limit = *reinterpret_cast<int*>(tracker + 0x1c);
      }))
  {
    if (counter >= 0 && limit >= 0 && (limit == 0 || counter <= limit)) {
      spdlog::info("[KillTracker] auto-fetch response: counter={}, limit={}", counter, limit);
      s_latest_tracker.counter.store(counter);
      s_latest_tracker.limit.store(limit);
      s_latest_tracker.has_new.store(true);
    } else {
      spdlog::warn("[KillTracker] auto-fetch response invalid: counter={}, limit={}", counter, limit);
    }
  } else if (!requestSuccessful) {
    spdlog::warn("[KillTracker] auto-fetch request failed");
  } else {
    spdlog::warn("[KillTracker] auto-fetch response access fault");
  }

  spdlog::info("[KillTracker] auto-fetch response captured; bypassing mod callback dispatch");
  spdlog::default_logger()->flush();
  return;
}

// ---------------------------------------------------------------------------
// Diagnostic hook on the managed no-op target.
// This lets us confirm the auto-fetch callback actually invokes the delegate.
// ---------------------------------------------------------------------------
void KeepAlive_Hook(auto original, Il2CppObject* obj, const MethodInfo* method)
{
  (void)obj;
  (void)method;
  spdlog::info("[KillTracker] no-op delegate invoked (obj={}, method={})", (void*)obj, (void*)method);
  spdlog::default_logger()->flush();
  original(obj, method);
}

} // namespace

static bool read_kill_counter_from_profile_manager(int& outCounter, int& outLimit)
{
  auto* ppm = PlayerProfileManager::Instance();
  if (!ppm) {
    spdlog::debug("[KillTracker] PlayerProfileManager instance is null");
    return false;
  }

  // Resolve field offsets once.
  static ptrdiff_t serviceOffset = -1;
  static ptrdiff_t dataContainerOffset = -1;
  static ptrdiff_t volatileTrackerOffset = -1;
  static bool      offsetsResolved = false;
  if (!offsetsResolved) {
    auto ppmHelper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.PlayerProfile", "PlayerProfileManager");
    auto serviceField = ppmHelper.GetField("_playerKillCounterService");
    if (serviceField.isValidHelper()) serviceOffset = serviceField.offset();

    auto serviceHelper = il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Services", "PlayerKillCounterService");
    auto dcField = serviceHelper.GetField("_dataContainer");
    if (dcField.isValidHelper()) dataContainerOffset = dcField.offset();

    auto dcHelper = il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Models", "PlayerKillCounterDataContainer");
    auto vtField = dcHelper.GetField("_volatileTracker");
    if (vtField.isValidHelper()) volatileTrackerOffset = vtField.offset();

    offsetsResolved = true;
    spdlog::info("[KillTracker] kill counter field offsets: service={}, dataContainer={}, volatileTracker={}",
                 serviceOffset, dataContainerOffset, volatileTrackerOffset);
  }

  if (serviceOffset < 0 || dataContainerOffset < 0 || volatileTrackerOffset < 0) {
    return false;
  }

  return seh_call([&] {
    auto* ppmObj = reinterpret_cast<char*>(ppm);
    auto* service = *reinterpret_cast<char**>(ppmObj + serviceOffset);
    if (!service) {
      spdlog::debug("[KillTracker] _playerKillCounterService is null");
      outCounter = outLimit = 0;
      return false;
    }

    auto* dataContainer = *reinterpret_cast<char**>(service + dataContainerOffset);
    if (!dataContainer) {
      spdlog::debug("[KillTracker] _dataContainer is null");
      outCounter = outLimit = 0;
      return false;
    }

    auto* tracker = *reinterpret_cast<char**>(dataContainer + volatileTrackerOffset);
    if (!tracker) {
      spdlog::debug("[KillTracker] _volatileTracker is null");
      outCounter = outLimit = 0;
      return false;
    }

    outCounter = *reinterpret_cast<int*>(tracker + 0x18);
    outLimit   = *reinterpret_cast<int*>(tracker + 0x1c);
    return true;
  });
}

// ---------------------------------------------------------------------------
// Active polling — called from ui_scale.cc Update hook
// (must be outside anonymous namespace for external linkage)
// ---------------------------------------------------------------------------
void kill_tracker_poll()
{
  if (!Config::Get().installKillTrackerHooks) return;

  auto now = std::chrono::steady_clock::now();
  int interval = Config::Get().kill_tracker_interval;

  if (now - s_state.lastPollTime < std::chrono::seconds(interval)) return;
  s_state.lastPollTime = now;

  // Phase 2: trigger a fresh tracker fetch so we don't depend on the profile view.
  // Only do this while safely docked in the main starbase, and disable the path
  // permanently if the call itself ever crashes.
  constexpr auto fetchInterval = std::chrono::seconds(60);
  if (s_auto_fetch_response_hooked &&
      !s_auto_fetch_crashed &&
      s_kill_tracker_callback_container &&
      s_kill_tracker_callback_container_handle != 0 &&
      Config::Get().kill_tracker_auto_fetch &&
      now - s_state.lastFetchTime >= fetchInterval) {
    s_state.lastFetchTime = now;
    auto* ppm = PlayerProfileManager::Instance();
    if (ppm) {
      spdlog::info("[KillTracker] triggering tracker fetch");
      bool fetch_ok = seh_call([&] { ppm->FetchPlayerKillCounter(s_kill_tracker_callback_container); });
      if (fetch_ok) {
        spdlog::info("[KillTracker] tracker fetch call returned");
        spdlog::default_logger()->flush();
      } else {
        s_auto_fetch_crashed = true;
        spdlog::error("[KillTracker] FetchPlayerKillCounter crashed; disabling auto-fetch");
        spdlog::default_logger()->flush();
      }
    } else {
      spdlog::debug("[KillTracker] PlayerProfileManager instance is null, cannot fetch");
    }
  }

  // Fallback: read the live _volatileTracker directly from PlayerProfileManager.
  // This works when the profile view is open and data has been fetched at least once.
  int counter = 0;
  int limit   = 0;
  if (read_kill_counter_from_profile_manager(counter, limit)) {
    spdlog::info("[KillTracker] poll read kill counter: counter={}, limit={}", counter, limit);
    if (counter >= 0 && limit >= 0 && (limit == 0 || counter <= limit)) {
      check_threshold(counter, limit);
    } else {
      spdlog::info("[KillTracker] poll rejected: limit={}, counter={}", limit, counter);
    }
  } else {
    spdlog::debug("[KillTracker] poll could not read kill counter");
  }

  // Process any tracker data delivered by network response hooks. We intentionally
  // do this on the main-thread poll rather than inside the hooks to avoid Unity
  // API calls from a networking background thread.
  if (s_latest_tracker.has_new.exchange(false)) {
    int new_counter = s_latest_tracker.counter.load();
    int new_limit   = s_latest_tracker.limit.load();
    spdlog::info("[KillTracker] poll consuming response tracker: counter={}, limit={}", new_counter, new_limit);
    if (new_counter >= 0 && new_limit >= 0 && (new_limit == 0 || new_counter <= new_limit)) {
      check_threshold(new_counter, new_limit);
    } else {
      spdlog::info("[KillTracker] poll rejected response tracker: limit={}, counter={}", new_limit, new_counter);
    }
  }
}

// ---------------------------------------------------------------------------
// Auto-fetch callback container setup
// FetchPlayerKillCounter refuses to dispatch with a bare zero-initialized
// CallbackContainer. We give it valid no-op delegates and set
// CallbackErrorHandling to Ignore so the request actually fires.
// IMPORTANT: IL2CPP delegates can only invoke managed methods, not raw native
// function pointers, so we build closed delegates to real managed no-ops.
// ---------------------------------------------------------------------------

static Il2CppObject* make_no_op_delegate(Il2CppClass* delegate_class, const MethodInfo* target_method,
                                         Il2CppMethodPointer native_invoke)
{
  if (!delegate_class || !target_method || !target_method->methodPointer || !native_invoke) return nullptr;

  auto* del = il2cpp_object_new(delegate_class);
  if (!del) return nullptr;

  // Let IL2CPP's own delegate constructor initialize all the internal fields.
  // Signature: .ctor(object target, IntPtr method)
  // The IntPtr argument is a pointer to the MethodInfo* (IL2CPP convention).
  auto* ctor = il2cpp_class_get_method_from_name(delegate_class, ".ctor", 2);
  if (!ctor) {
    spdlog::warn("[KillTracker] make_no_op_delegate: no .ctor(2) on {}", delegate_class->name);
    return nullptr;
  }

  MethodInfo* method_nonconst = const_cast<MethodInfo*>(target_method);
  void* args[2] = { nullptr, &method_nonconst };
  Il2CppException* exc = nullptr;
  il2cpp_runtime_invoke(ctor, del, args, &exc);
  if (exc) {
    spdlog::warn("[KillTracker] make_no_op_delegate: .ctor threw");
    return nullptr;
  }

  auto* delegate = reinterpret_cast<Il2CppDelegate*>(del);
  delegate->method_ptr = native_invoke;
  delegate->invoke_impl = native_invoke;
  delegate->invoke_impl_this = nullptr;

  return del;
}

static bool configure_auto_fetch_callback_container()
{
  if (!s_kill_tracker_callback_container || !s_callback_container_class || !s_keep_alive_method) return false;

  auto set_delegate_field = [&](const char* fieldName, Il2CppClass* delegate_class,
                                Il2CppMethodPointer native_invoke) -> bool {
    auto* field = il2cpp_class_get_field_from_name(s_callback_container_class, fieldName);
    if (!field) {
      spdlog::warn("[KillTracker] Could not find CallbackContainer<Tracker>.{} field", fieldName);
      return false;
    }
    if (strcmp(fieldName, "_onSuccess") == 0) s_callback_on_success_field = field;
    if (strcmp(fieldName, "_onError") == 0) s_callback_on_error_field = field;

    auto* del = make_no_op_delegate(delegate_class, s_keep_alive_method, native_invoke);
    if (!del) {
      spdlog::warn("[KillTracker] Failed to allocate no-op {} delegate", fieldName);
      return false;
    }
    il2cpp_field_set_value(s_kill_tracker_callback_container, field, &del);
    auto* runtime_delegate = reinterpret_cast<Il2CppDelegate*>(del);
    spdlog::info("[KillTracker] native {} delegate: object={}, invoke_impl={}, target={}, invoke_impl_this={}, method={}",
                 fieldName, (void*)del, (void*)runtime_delegate->invoke_impl, (void*)runtime_delegate->target,
                 (void*)runtime_delegate->invoke_impl_this, (void*)runtime_delegate->method);
    return true;
  };

  if (!set_delegate_field("_onSuccess", s_on_success_class,
                          reinterpret_cast<Il2CppMethodPointer>(KillTrackerOnSuccessNoOp))) return false;
  if (!set_delegate_field("_onError", s_on_error_class,
                          reinterpret_cast<Il2CppMethodPointer>(KillTrackerOnErrorNoOp))) return false;

  // OnRetry is null with CallbackErrorHandling.Ignore; retry callbacks are not expected.
  {
    auto* field = il2cpp_class_get_field_from_name(s_callback_container_class, "_onRetry");
    if (field) {
      Il2CppObject* nullRetry = nullptr;
      il2cpp_field_set_value(s_kill_tracker_callback_container, field, &nullRetry);
    }
  }

  auto* errorHandlingField = il2cpp_class_get_field_from_name(s_callback_container_class, "_callbackErrorHandling");
  if (errorHandlingField) {
    int32_t ignore = 1; // CallbackErrorHandling.Ignore
    il2cpp_field_set_value(s_kill_tracker_callback_container, errorHandlingField, &ignore);
  }

  // Sanity-check that the _onSuccess field really got the delegate object.
  auto* successField = il2cpp_class_get_field_from_name(s_callback_container_class, "_onSuccess");
  if (successField) {
    Il2CppObject* verify = nullptr;
    il2cpp_field_get_value(s_kill_tracker_callback_container, successField, &verify);
    spdlog::info("[KillTracker] callback container _onSuccess verified at {}", (void*)verify);
  }

  spdlog::info("[KillTracker] auto-fetch callback container configured with managed no-op delegates");
  return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void InstallKillTrackerHooks()
{
  spdlog::info("[KillTracker] InstallKillTrackerHooks() — starting installation");
  ensure_cache();

  s_auto_fetch_response_hooked = false;
  s_auto_fetch_crashed = false;

  // Reset state on install
  s_state = KillTrackerState{};
  auto now = std::chrono::steady_clock::now();
  s_state.lastPollTime  = now;
  s_state.lastFetchTime = now;

  // Allocate our auto-fetch callback container early. It lives for the session.
  if (s_callback_container_class) {
    s_kill_tracker_callback_container = il2cpp_object_new(s_callback_container_class);
    if (s_kill_tracker_callback_container) {
      spdlog::info("[KillTracker] auto-fetch CallbackContainer<Tracker> allocated at {}",
                   (void*)s_kill_tracker_callback_container);
      if (configure_auto_fetch_callback_container()) {
        s_kill_tracker_callback_container_handle =
            il2cpp_gchandle_new(s_kill_tracker_callback_container, true); // pinned
        spdlog::info("[KillTracker] auto-fetch CallbackContainer<Tracker> pinned with handle={}",
                     (uint64_t)s_kill_tracker_callback_container_handle);
        auto service_helper = il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime",
                                                       "Digit.PrimeServer.Services", "PlayerKillCounterService");
        auto* response_method = find_fetch_kill_counter_completion(service_helper.get_cls());
        if (response_method) {
          SPUD_STATIC_DETOUR(response_method->methodPointer, FetchKillCounterStat_Response_Hook);
          s_auto_fetch_response_hooked = true;
          s_auto_fetch_crashed = false;
          spdlog::info("[KillTracker] auto-fetch response hook installed; mod callback dispatch will be bypassed");
        } else {
          s_auto_fetch_crashed = true;
          spdlog::warn("[KillTracker] auto-fetch disabled: response completion method was not resolved");
        }
      } else {
        spdlog::warn("[KillTracker] callback container configuration failed; auto-fetch will not be used");
        s_auto_fetch_crashed = true;
        s_kill_tracker_callback_container = nullptr;
      }
    } else {
      spdlog::warn("[KillTracker] il2cpp_object_new failed for CallbackContainer<Tracker>");
    }
  }

  // NOTE: We intentionally do NOT hook CallbackContainer<Tracker>.TriggerSuccess/TriggerError.
  // The response updates _volatileTracker directly and then invokes our no-op OnSuccess
  // delegate. We read the updated tracker from the main-thread poll, so there is no
  // need to detour the heavily-used generic callback method.

  // Resolve TryPopTracker for diagnostic purposes only (it is not the hostile-kill path).
  auto helper = il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Models",
                                        "PlayerKillCounterDataContainer");
  if (helper.isValidHelper()) {
    auto ptr = helper.GetMethod("TryPopTracker");
    if (ptr == nullptr) {
      spdlog::warn("[KillTracker] Failed to resolve TryPopTracker method");
    } else {
      spdlog::info("[KillTracker]   TryPopTracker method resolved: {} (not installed)", (void*)ptr);
    }
  }

  // Hook PlayerProfileViewController.OnFetchPlayerKillCounterSuccess — clean data path
  auto ppvc_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.PlayerProfile", "PlayerProfileViewController");
  if (ppvc_helper.isValidHelper()) {
    auto* ppvc_method = ppvc_helper.GetMethodInfo("OnFetchPlayerKillCounterSuccess", 1);
    if (ppvc_method && ppvc_method->methodPointer) {
      spdlog::info("[KillTracker]   OnFetchPlayerKillCounterSuccess method resolved: {}", (void*)ppvc_method->methodPointer);
      SPUD_STATIC_DETOUR(ppvc_method->methodPointer, OnFetchPlayerKillCounterSuccess_Hook);
      spdlog::info("[KillTracker]   OnFetchPlayerKillCounterSuccess hook installed");
    } else {
      spdlog::warn("[KillTracker]   Failed to resolve OnFetchPlayerKillCounterSuccess method");
    }
  } else {
    spdlog::warn("[KillTracker]   Failed to resolve PlayerProfileViewController class");
  }

  spdlog::info("[KillTracker] Hooks installed — threshold: {} kills, interval: {}s",
               Config::Get().kill_tracker_threshold, Config::Get().kill_tracker_interval);
}
