# Kill Tracker Toast Notification — Technical Design

> **Purpose:** Technical blueprint for injecting a toast popup that displays the hostile kill counter every N kills (N = 5, 10, or 25), combining the Tracker (kill counter) system with the Toast notification system.

## System Overview

Two independent systems must be bridged:

```
Tracker System (data source)          Toast System (display)
─────────────────────────             ──────────────────────
PlayerKillCounterService              ToastObserver (abstract MonoBehaviour)
  └─ POST /tracker                    Toast (data object)
  └─ Tracker protobuf                 ToastViewController (queue + render)
  └─ EntityGroup type 9301            ToastManager (singleton, expiry)
                                      ToastLocaleSettings (ScriptableObject)
```

The kill counter data arrives via the game server, and the toast system displays transient popups in the HUD. There is no existing connection between them — this document describes how to build that bridge.

---

## Part 1: Tracker System (Data Source)

### 1.1 Protobuf Message

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Digit.Client.PrimeLib.Runtime\Digit.PrimeServer.Models\Tracker.cs`

```protobuf
message Tracker {
  int32 counter = 1;   // Current hostile kill count
  int32 limit  = 2;    // Kill limit / threshold (event-specific)
}
```

- `Counter` — the running total of hostile kills
- `Limit` — the event kill cap (0 if no limit)
- EntityGroup type: **9301**
- Endpoint: `POST /tracker`

### 1.2 Service Layer

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Digit.Client.PrimeLib.Runtime\Digit.PrimeServer.Services\PlayerKillCounterService.cs`

```
PlayerKillCounterService : GSService
├── _dataContainer: PlayerKillCounterDataContainer
├── FetchKillCounterStat(CallbackContainer<Tracker> callbacks)
│   ├── Creates PostRequest to /tracker
│   ├── On success: callbacks.TriggerSuccess(Tracker)
│   └── On error: callbacks.TriggerError(GSError)
└── RegisterEvents(ServerContext) — registers for EntityGroup binary sync
```

The `CallbackContainer<Tracker>` type (`@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Digit.Engine.Utilities.Runtime\Digit.Networking.Core\CallbackContainer.cs`) wraps:
- `OnSuccess<T>` — `public delegate void OnSuccess<T>(T item)` (`@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Digit.Engine.Utilities.Runtime\Digit.Networking.Core\OnSuccess.cs`)
- `OnError` — error callback
- `OnRetry` — retry callback
- `TriggerSuccess(T)` / `TriggerError(GSError)` / `TriggerRetry(int, int)`

### 1.3 Data Container (Binary Sync Cache)

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Digit.Client.PrimeLib.Runtime\Digit.PrimeServer.Models\PlayerKillCounterDataContainer.cs`

```
PlayerKillCounterDataContainer : IBinaryDataContainer<EntityGroup>
├── _volatileTracker: Tracker          — cached Tracker from binary sync
├── ParseBinaryObject(EntityGroup)     — deserializes EntityGroup → Tracker
├── HandleResponseData(Type)           — processes response type
└── TryPopTracker(out Tracker): bool   — returns cached Tracker (one-shot pop)
```

The `TryPopTracker` method returns the cached `Tracker` and clears the volatile reference. This is the binary data sync path — the server pushes EntityGroup updates containing Tracker data without an explicit request.

### 1.4 Profile Manager (Orchestration)

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.PlayerProfile\PlayerProfileManager.cs`

```
PlayerProfileManager : MonoSingleton<PlayerProfileManager>
├── _playerKillCounterService: PlayerKillCounterService
├── _hostileKillCounterDisabled: bool  — feature flag check
├── FetchPlayerKillCounter(CallbackContainer<Tracker> callbacks)
│   ├── Checks _hostileKillCounterDisabled
│   ├── Lazy-initialises _playerKillCounterService via InitialiseKillCounterService()
│   └── Calls _playerKillCounterService.FetchKillCounterStat(callbacks)
├── InitialiseKillCounterService(): bool
└── ShowPlayerProfile(string userId) / ShowPlayerProfile(UserProfile profile)
```

The `_hostileKillCounterDisabled` property checks the `HostileKillCounterFeatureDisabled` flag. When true, `FetchPlayerKillCounter` returns early without making the server request.

### 1.5 View Controller (Polling + Display)

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.PlayerProfile\PlayerProfileViewController.cs`

```
PlayerProfileViewController : ViewController<PlayerProfileDataContext>
├── KillCounterTextCooldownSeconds = 30 (const)
├── _killCounterText: TextLocalizer              — UI label for kill count
├── _userProfile: UserProfile                    — currently displayed profile
├── _isLocalPlayerBool: AnimatorParameterInfo    — animator param for local player
├── _nextKillCounterTextUpdateTime: DateTime     — next allowed poll time
├── AboutToShow()
│   ├── Checks if profile is local player: *(plVar1 + 0x60) == 0
│   │   └── If local player: calls UpdateKillCounterText()
│   └── Binds widgets, sets up profile data
├── UpdateKillCounterText()
│   ├── Checks DateTime.Now >= _nextKillCounterTextUpdateTime
│   ├── Updates _nextKillCounterTextUpdateTime = Now + 30s
│   ├── Creates CallbackContainer<Tracker> with:
│   │   ├── OnSuccess: OnFetchPlayerKillCounterSuccess
│   │   └── OnError: OnFetchPlayerKillCounterError
│   └── Calls PlayerProfileManager.FetchPlayerKillCounter(callbacks)
├── OnFetchPlayerKillCounterSuccess(Tracker tracker)
│   ├── Reads tracker.Counter (offset +0x18 in Ghidra)
│   ├── Reads tracker.Limit (offset +0x1c in Ghidra)
│   ├── Formats counter as locale text parameter
│   ├── Sets _killCounterText text parameters
│   └── Updates TextLocalizer via virtual call at +0x378
└── OnFetchPlayerKillCounterError(GSError error)
    └── Logs/handles error
```

**Key Ghidra findings** (`@/d:\Dev\STFC\client\decompiled\m93-beta2\ghidra\decompiled\Digit\Prime\PlayerProfile\PlayerProfileViewController\Digit.Prime.PlayerProfile.PlayerProfileViewController__AboutToShow_180a46540.c:70`):

```c
// Line 70: Local player check — offset +0x60 on the profile data context
if ((*plVar1 != 0) && (*(int *)(*plVar1 + 0x60) == 0)) {
    FUN_180a47660(param_1, 0);  // UpdateKillCounterText()
}
```

The `+0x60` field corresponds to `_isLocalPlayerBool` / the `IsLocalPlayer` flag on `PlayerProfileDataContext`. **Kill counter polling only fires for the local player's profile.**

**Ghidra: `OnFetchPlayerKillCounterSuccess`** (`@/d:\Dev\STFC\client\decompiled\m93-beta2\ghidra\decompiled\Digit\Prime\PlayerProfile\PlayerProfileViewController\Digit.Prime.PlayerProfile.PlayerProfileViewController__OnFetchPlayerKillCounterSuccess_180a478c0.c`):

```c
// Line 44: reads tracker.Counter (param_2 + 0x18)
local_res10[0] = *(undefined4 *)(param_2 + 0x18);
// Line 54: reads tracker.Limit (param_2 + 0x1c)
local_res10[0] = *(undefined4 *)(param_2 + 0x1c);
// Line 68-69: updates TextLocalizer via virtual call
(**(code **)(*plVar1 + 0x378))(plVar1, cVar6 != '\0', plVar2, ...);
```

### 1.6 Data Flow Summary

```
[Every 30s, if local player profile is visible]
  PlayerProfileViewController.UpdateKillCounterText()
    │
    ├── Creates CallbackContainer<Tracker>(
    │     onSuccess: OnFetchPlayerKillCounterSuccess,
    │     onError: OnFetchPlayerKillCounterError)
    │
    ▼
  PlayerProfileManager.FetchPlayerKillCounter(callbacks)
    ├── Checks _hostileKillCounterDisabled
    └── PlayerKillCounterService.FetchKillCounterStat(callbacks)
        │
        ├── POST /tracker → server returns EntityGroup(type=9301)
        ├── PlayerKillCounterDataContainer.ParseBinaryObject()
        │   └── Deserializes → Tracker { counter, limit }
        └── callbacks.TriggerSuccess(tracker)
            │
            ▼
        OnFetchPlayerKillCounterSuccess(Tracker tracker)
            ├── tracker.Counter → formatted as locale text
            ├── tracker.Limit → formatted as locale text
            └── _killCounterText.TextLocalizer updated
```

### 1.7 Limitation: Local Player Only

The kill counter fetch is gated by the `IsLocalPlayer` check in `AboutToShow` (offset `+0x60` on the profile data context). The `FetchPlayerKillCounter` method itself does not take a `userId` — it implicitly fetches for the authenticated session. There is no client-side mechanism to query another player's kill counter via the `/tracker` endpoint.

---

## Part 2: Toast Notification System (Display Layer)

### 2.1 Toast Class

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\Toast.cs`

```csharp
public class Toast
{
    public ToastState State { get; protected set; }
    public object[] TextParameters { get; set; }
    public LocaleTextContext TextLocaleTextContext { get; set; }
    public LocaleTextContext SecondaryTextLocaleTextContext { get; set; }
    public string IconIdentifier { get; set; }
    public object Data { get; set; }
    public ToastTimeout Timeout;
    public long UserDataLong;
    public GeneratedGameEvents.GameEvents ShowEvent;
    public SafeAction<Toast> ClickCallback;
    public bool ToastWasClicked;

    // Full constructor:
    public Toast(
        ToastState state,
        LocaleTextContext textLocaleTextContext,
        string iconIdentifier,
        Action<Toast> callback,           // click callback
        GeneratedGameEvents.GameEvents showEvent,
        object data,
        object[] textParameters)
    { ... }

    // Parameterless constructor:
    public Toast() { ... }
}
```

### 2.2 ToastState Enum

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\ToastState.cs`

60 states defined. For a kill counter toast, use **`ToastState.Standard`** (value 0), which maps to `StandardToastWidget` — a simple widget with a `TextLocalizer` and `ImageSelector`.

### 2.3 ToastTimeout Enum

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\ToastTimeout.cs`

```csharp
public enum ToastTimeout
{
    Default,   // Standard duration (~3-5s based on locale settings)
    Long,      // Extended duration
    None       // No auto-timeout (requires user dismiss)
}
```

### 2.4 LocaleTextContext

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Client.UI\LocaleTextContext.cs`

```csharp
[Serializable]
public class LocaleTextContext : IHashable
{
    public string Identifier { get; set; }      // Localization key
    public string Category { get; set; }        // Localization category
    public object[] IdentifierParameters { get; set; }
    public object[] TextParameters { get; set; }  // Format parameters
    public bool OverrideBaseIdentifier { get; set; }
    public bool ForceLocalizeParams { get; set; }
    // ... number formatting options

    public LocaleTextContext(string identifier, string category) { ... }
    public LocaleTextContext(string identifier, string category, object[] textParameters) { ... }
}
```

The `Identifier` is a localization key (e.g. `"toast_kill_counter_milestone"`), `Category` is the localization category (e.g. `"kill_counter"`), and `TextParameters` are format arguments injected into the localized string (e.g. `[5]` for the kill count).

### 2.5 ToastObserver (Abstract Base)

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\ToastObserver.cs`

```csharp
public abstract class ToastObserver : MonoBehaviour
{
    protected ToastViewController _viewController;

    protected void EnqueueToast(Toast toast) { ... }
    protected void EnqueueOrCombineToast(Toast toast, UnityAction<Toast> compareCallback) { ... }
    protected void AddFirstOrCombineToast(Toast toast, UnityAction<Toast> compareCallback = null) { ... }

    private bool AreToastsAllowed(Toast toast) { ... }
    private bool ExistsInQueue(Toast toast, UnityAction<Toast> compareCallback = null) { ... }
}
```

`EnqueueToast` is the primary method. It:
1. Checks `AreToastsAllowed` (scene status, ignored sections, block flags)
2. Checks `ExistsInQueue` (deduplication)
3. Accesses `_viewController` (the `ToastViewController` in the scene)
4. Adds the toast to the `ToastUIContext.ToastQueue`

### 2.6 ToastViewController

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\ToastViewController.cs`

```
ToastViewController : ViewController<ToastUIContext>
├── Widget resolution by ToastState:
│   ├── Standard → _standardWidget (StandardToastWidget)
│   ├── FleetBattle → _standardWidget (with battle params)
│   ├── Tournament → _tournamentToastWidget
│   ├── ArmadaCreated → _armadaWidget
│   └── ... (20+ specialized widgets)
├── CheckToastQueue() coroutine — polls queue, shows next toast
├── ShowNextToast() — dequeues and displays
├── UpdateFromCurrentToast() — binds toast to widget
├── TryResolveToastWidget(state, out Widget) — maps state → widget
├── TimeOutToast() coroutine — auto-dismiss after timeout
└── DismissCurrentToast() — manual dismiss
```

The `ToastUIContext` holds a `Queue<Toast>` (`@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\ToastUIContext.cs:7`).

### 2.7 StandardToastWidget

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\StandardToastWidget.cs`

```csharp
public class StandardToastWidget : Widget<Toast>
{
    private TextLocalizer _label;      // Displays toast text
    private ImageSelector _image;      // Displays toast icon

    protected override void OnDidBindContext() { ... }
}
```

Simple widget: text + icon. When a `Toast` with `ToastState.Standard` is bound, the `TextLocalizer` reads `Toast.TextLocaleTextContext` and the `ImageSelector` reads `Toast.IconIdentifier`.

### 2.8 ToastLocaleSettings

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\ToastLocaleSettings.cs`

A `ScriptableObject` with pre-configured `LocaleTextContext` instances for every toast type. Contains ~100+ locale context fields (victory, defeat, alliance, armada, etc.). Also defines timeout durations:

```csharp
public float ToastTimeout(ToastTimeout type) { ... }
// _toastTimeout (Default) and _toastTimeoutLong (Long)
```

### 2.9 ToastManager

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.HUD\ToastManager.cs`

```csharp
public class ToastManager : MonoSingleton<ToastManager>
{
    private const float _allowableTimeSinceCreation = 5f;
    private DateTime _gameSessionStartTime;

    public bool IsToastExpired(DateTime creationTime, string toastType) { ... }
}
```

Singleton that tracks session time and toast expiry. Toasts created more than 5 seconds after session start are considered valid; earlier ones may be suppressed (prevents toast spam during loading).

### 2.10 Toast Display Flow

```
ToastObserver.EnqueueToast(Toast toast)
    │
    ├── AreToastsAllowed(toast) — scene/section check
    ├── ExistsInQueue(toast) — deduplication
    │
    ▼
ToastUIContext.ToastQueue.Enqueue(toast)
    │
    ▼
ToastViewController.CheckToastQueue() coroutine
    │
    ├── ShouldShowNextToast() — checks block flags, battle state
    ├── ShowNextToast()
    │   ├── Dequeue from ToastQueue
    │   ├── UpdateFromCurrentToast()
    │   │   ├── TryResolveToastWidget(toast.State, out widget)
    │   │   │   └── Standard → StandardToastWidget
    │   │   └── BindToastWidgetDataContext()
    │   │       └── widget.SetDataContext(toast)
    │   └── SetAnimations() — trigger show animation
    │
    ▼
StandardToastWidget.OnDidBindContext()
    ├── _label.TextLocalizer → reads Toast.TextLocaleTextContext
    │   └── Localized string with TextParameters injected
    └── _image.ImageSelector → reads Toast.IconIdentifier
    │
    ▼
TimeOutToast() coroutine — waits ToastTimeout.Default seconds
    │
    ▼
DismissCurrentToast() — trigger hide animation
    │
    ▼
CheckToastQueue() — loop back for next toast
```

---

## Part 3: Alternative Display Path — Notification System

### 3.1 NotificationsService.InjectNotification

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Digit.Client.PrimeLib.Runtime\Digit.PrimeServer.Services\NotificationsService.cs:65`

```csharp
public void InjectNotification(Notification notification) { ... }
```

This allows injecting a synthetic `Notification` protobuf directly into the notification pipeline. The notification would then flow through:

```
NotificationsService.InjectNotification(Notification)
    → UpdatePlayerNotifications()
    → NotificationsChangedEventHandler(producerType)
    → NotificationManager.EnqueueNotifications()
    → NotificationsDataContainer.EnqueueNotifications()
    → UINotification wrapper
    → ToastObservers / PipManager
```

### 3.2 NotificationManager.TriggerShowUINotification

**File:** `@/d:\Dev\STFC\client\decompiled\m93-beta2\source\Assembly-CSharp\Digit.Prime.Notifications\NotificationManager.cs:143`

```csharp
public static bool TriggerShowUINotification(UINotification notification) { ... }
```

Directly fires the `OnShowUINotification` event, bypassing the queue. Observers listening to this event would display the notification.

### 3.3 Trade-off: Toast vs Notification

| Aspect | Toast (recommended) | Notification |
|--------|-------------------|--------------|
| Weight | Lightweight — single `Toast` object | Heavy — full `Notification` protobuf + `UINotification` wrapper |
| Queue | Simple FIFO in `ToastUIContext` | Category-based queues in `NotificationsDataContainer` |
| Widget | `StandardToastWidget` (text + icon) | Requires matching toast observer or custom widget |
| Localization | `LocaleTextContext` directly on `Toast` | `Notification` params → observer → `LocaleTextContext` |
| Click handling | `Toast.ClickCallback` | Observer-specific click handlers |
| Post-processors | None | Full post-processor pipeline |
| Scene awareness | `AreToastsAllowed` check | `NotificationSceneStatus` (Off/GeneralOnly/All) |
| Best for | Simple transient popups | Game-state notifications with categories/pips |

**Recommendation:** Use the **Toast** path. It's simpler, lighter, and the kill counter milestone is a transient informational popup — not a persistent game notification that needs pip badges or category queuing.

---

## Part 4: Hook Points

### 4.1 Primary Hook: `OnFetchPlayerKillCounterSuccess`

**Target:** `PlayerProfileViewController.OnFetchPlayerKillCounterSuccess(Tracker tracker)`

This method fires every 30 seconds with a fresh `Tracker` object containing the current kill count. It's the ideal hook point because:
- It receives the actual `Tracker` data
- It only fires for the local player (already gated by `AboutToShow`)
- It fires periodically (every 30s) — natural polling interval
- The `Tracker` object is a complete protobuf with `Counter` and `Limit`

**Patch strategy:** Postfix patch — after the original method updates the UI text, check if the kill count has crossed a threshold and fire a toast.

### 4.2 Alternative Hook: `PlayerKillCounterDataContainer.TryPopTracker`

**Target:** `PlayerKillCounterDataContainer.TryPopTracker(out Tracker tracker)`

This fires whenever binary sync delivers new Tracker data, regardless of whether the profile view is open. This could catch kill counter updates even when the player isn't viewing their own profile.

**Trade-off:** More responsive (doesn't require profile view open), but may fire during loading or transitions when toasts are suppressed by `ToastManager`.

### 4.3 Alternative Hook: `PlayerKillCounterService.FetchKillCounterStat`

**Target:** `PlayerKillCounterService.FetchKillCounterStat(CallbackContainer<Tracker> callbacks)`

Patch the service method to intercept the `CallbackContainer` and add an additional `OnSuccess` callback. This fires at the service level before the view controller processes the data.

**Trade-off:** Requires manipulating the `CallbackContainer` to inject a secondary callback. More complex but gives earliest access to the data.

---

## Part 5: Threshold Logic

### 5.1 State Tracking

The patch must maintain state across callbacks:

```csharp
// Persistent state
static int s_lastNotifiedKillCount = -1;   // -1 = not yet initialized
static int s_threshold = 5;                 // configurable: 5, 10, or 25
```

### 5.2 Threshold Check Algorithm

```
On each Tracker callback:
  ┌─────────────────────────────────────────────────────┐
  │  currentKills = tracker.Counter                     │
  │                                                     │
  │  if s_lastNotifiedKillCount == -1:                  │
  │      // First load — initialize baseline             │
  │      s_lastNotifiedKillCount = currentKills         │
  │      return                                         │
  │                                                     │
  │  delta = currentKills - s_lastNotifiedKillCount     │
  │                                                     │
  │  if delta <= 0:                                     │
  │      // Counter reset or unchanged — update baseline │
  │      s_lastNotifiedKillCount = currentKills         │
  │      return                                         │
  │                                                     │
  │  if delta >= s_threshold:                           │
  │      // Threshold crossed — fire toast               │
  │      ShowKillCounterToast(currentKills, delta)      │
  │      s_lastNotifiedKillCount = currentKills         │
  │                                                     │
  │  // else: not enough kills yet, wait                │
  └─────────────────────────────────────────────────────┘
```

### 5.3 Edge Cases

| Scenario | Handling |
|----------|----------|
| **First load** | Initialize `s_lastNotifiedKillCount` to current value, no toast |
| **Counter reset** (new event) | `delta <= 0` → update baseline, no toast |
| **Feature disabled** | `_hostileKillCounterDisabled` → original method returns early, hook never fires |
| **Profile not visible** | `UpdateKillCounterText` not called → hook never fires |
| **Multiple thresholds** | Could check `currentKills % threshold == 0` instead of delta, but delta is more reliable with 30s polling |
| **Threshold change** | Reset `s_lastNotifiedKillCount` to current value when threshold changes |

### 5.4 Threshold Selection

| Threshold | Use case |
|-----------|----------|
| **5** | High feedback frequency — every 5 kills |
| **10** | Balanced — moderate notification frequency |
| **25** | Low noise — only major milestones |

With 30s polling, the maximum detectable delta per check depends on kill rate. At 1 kill/second, delta could be up to 30 per check. The threshold should be ≤ 30 to avoid missing milestones.

---

## Part 6: Toast Creation Pattern

### 6.1 Creating a Toast

```csharp
// 1. Create LocaleTextContext for the toast message
//    Identifier: localization key (must exist in game's locale DB or use a fallback)
//    Category: localization category
//    TextParameters: format arguments for the localized string
var localeContext = new LocaleTextContext(
    identifier: "toast_kill_counter_milestone",  // Custom key
    category: "kill_counter",
    textParameters: new object[] { currentKills, delta }
);

// 2. Create the Toast
var toast = new Toast(
    state: ToastState.Standard,           // Maps to StandardToastWidget
    textLocaleTextContext: localeContext,  // Text to display
    iconIdentifier: "kill_counter_icon",   // Icon to show (or null for no icon)
    callback: null,                        // No click handler needed
    showEvent: GeneratedGameEvents.GameEvents.None,  // No analytics event
    data: tracker,                         // Original Tracker object for reference
    textParameters: new object[] { currentKills, delta }
);

// 3. Set timeout
toast.Timeout = ToastTimeout.Default;

// 4. Enqueue via ToastObserver
//    Need a reference to an active ToastObserver in the scene
//    Options:
//    a) Find existing ToastObserver in scene (e.g. ToastChallengeObserver)
//    b) Use ToastViewController directly
//    c) Create a temporary MonoBehaviour to call EnqueueToast
```

### 6.2 Localization Considerations

The `LocaleTextContext.Identifier` must reference a key in the game's localization database. Since we're injecting a custom toast, the key won't exist. Options:

1. **Use an existing toast locale key** — repurpose an existing key like `"toast_victory"` or `"toast_challenge_complete"` and override the text parameters. The displayed text will be the existing localized string with our parameters injected.

2. **Use `OverrideBaseIdentifier = true`** — set `Identifier` to a raw string and enable `OverrideBaseIdentifier`. This may cause the `TextLocalizer` to display the identifier directly if no localization match is found.

3. **Direct string construction** — set `Identifier` to a formatted string like `"Hostile Kills: {0} (+{1})"` and enable `OverrideBaseIdentifier`. The `TextLocalizer` will display this as-is.

**Recommended:** Option 3 — direct string with `OverrideBaseIdentifier = true`. This avoids dependency on the localization system and ensures the toast always displays meaningful text.

### 6.3 Accessing the Toast Queue

The `ToastObserver.EnqueueToast` method is `protected`, so it can only be called from within a `ToastObserver` subclass. Options:

**Option A: Find an existing ToastObserver in the scene**

```csharp
// Find any active ToastObserver in the scene
var observer = UnityEngine.Object.FindObjectOfType<ToastObserver>();
// Use reflection to call EnqueueToast (protected method)
var method = typeof(ToastObserver).GetMethod("EnqueueToast",
    System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
method.Invoke(observer, new object[] { toast });
```

**Option B: Access ToastViewController directly**

```csharp
// ToastViewController is a ViewController<ToastUIContext>
// ToastUIContext has a public ToastQueue field
var vc = UnityEngine.Object.FindObjectOfType<ToastViewController>();
var context = vc.GetDataContext();  // ToastUIContext
context.ToastQueue.Enqueue(toast);
```

**Option C: Create a custom ToastObserver MonoBehaviour**

```csharp
// In a BepInEx plugin, add a new MonoBehaviour to the scene
// that extends ToastObserver and exposes a public enqueue method
public class KillCounterToastObserver : ToastObserver
{
    public void ShowToast(Toast toast)
    {
        EnqueueToast(toast);
    }
}
```

**Recommended:** Option A (reflection) for simplicity, or Option C for a cleaner long-term solution.

---

## Part 7: Code Sketch — BepInEx + Harmony Plugin

This sketch shows the overall structure of a plugin that would implement the kill tracker toast. It is **not** production code — it's a blueprint for implementation.

### 7.1 Plugin Skeleton

```csharp
using BepInEx;
using BepInEx.IL2CPP;
using HarmonyLib;
using Digit.PrimeServer.Models;
using Digit.Prime.HUD;
using Digit.Client.UI;
using Digit.Prime.PlayerProfile;
using UnityEngine;

[BepInPlugin("stfc.killtrackertoast", "Kill Tracker Toast", "1.0.0")]
public class KillTrackerToastPlugin : BasePlugin
{
    static int s_threshold = 5;              // Configurable: 5, 10, 25
    static int s_lastNotifiedKillCount = -1;

    public override void Load()
    {
        // Read threshold from config file
        var config = Config.Bind("General", "Threshold", 5,
            "Show toast every N kills (5, 10, or 25)");
        s_threshold = config.Value;

        // Apply Harmony patches
        Harmony.PatchAll();
    }

    // ─── Hook: OnFetchPlayerKillCounterSuccess ───
    [HarmonyPatch(typeof(PlayerProfileViewController),
        nameof(PlayerProfileViewController.OnFetchPlayerKillCounterSuccess))]
    static class OnFetchSuccessPatch
    {
        static void Postfix(PlayerProfileViewController __instance, Tracker tracker)
        {
            if (tracker == null) return;

            int currentKills = tracker.Counter;

            // First load — initialize baseline
            if (s_lastNotifiedKillCount == -1)
            {
                s_lastNotifiedKillCount = currentKills;
                return;
            }

            int delta = currentKills - s_lastNotifiedKillCount;

            // Counter reset or no change
            if (delta <= 0)
            {
                s_lastNotifiedKillCount = currentKills;
                return;
            }

            // Threshold crossed
            if (delta >= s_threshold)
            {
                ShowKillToast(currentKills, delta);
                s_lastNotifiedKillCount = currentKills;
            }
        }
    }

    // ─── Toast Creation ───
    static void ShowKillToast(int currentKills, int delta)
    {
        // Create locale context with direct string override
        var localeContext = new LocaleTextContext(
            $"Hostile Kills: {currentKills} (+{delta})",
            "kill_counter"
        );
        localeContext.OverrideBaseIdentifier = true;

        // Create toast
        var toast = new Toast(
            state: ToastState.Standard,
            textLocaleTextContext: localeContext,
            iconIdentifier: null,                    // No custom icon
            callback: null,                          // No click handler
            showEvent: GeneratedGameEvents.GameEvents.None,
            data: null,
            textParameters: new object[] { currentKills, delta }
        );
        toast.Timeout = ToastTimeout.Default;

        // Enqueue via reflection on existing ToastObserver
        var observer = Object.FindObjectOfType<ToastObserver>();
        if (observer != null)
        {
            var method = typeof(ToastObserver).GetMethod("EnqueueToast",
                System.Reflection.BindingFlags.NonPublic |
                System.Reflection.BindingFlags.Instance);
            method?.Invoke(observer, new object[] { toast });
        }
    }
}
```

### 7.2 Alternative Hook: DataContainer

```csharp
// Hook the binary sync path instead of the view controller
[HarmonyPatch(typeof(PlayerKillCounterDataContainer),
    nameof(PlayerKillCounterDataContainer.TryPopTracker))]
static class TryPopTrackerPatch
{
    static void Postfix(ref Tracker tracker, ref bool __result)
    {
        if (!__result || tracker == null) return;

        int currentKills = tracker.Counter;
        // Same threshold logic as above
        // This fires regardless of whether profile view is open
    }
}
```

### 7.3 Alternative Hook: Service Level

```csharp
// Hook the service to intercept the callback container
[HarmonyPatch(typeof(PlayerKillCounterService),
    nameof(PlayerKillCounterService.FetchKillCounterStat))]
static class FetchKillCounterStatPatch
{
    static void Prefix(ref CallbackContainer<Tracker> callbacks)
    {
        // Wrap the existing OnSuccess callback to add our logic
                var originalOnSuccess = callbacks.OnSuccess;
                // Note: CallbackContainer.OnSuccess has only a getter,
                // so this approach requires reflection or a different strategy.
                // Consider patching the lambda inside FetchKillCounterStat instead.
    }
}
```

---

## Part 8: Key Types Reference

| Type | Namespace | File | Purpose |
|------|-----------|------|---------|
| `Tracker` | `Digit.PrimeServer.Models` | `Tracker.cs` | Protobuf: `Counter`, `Limit` |
| `PlayerKillCounterService` | `Digit.PrimeServer.Services` | `PlayerKillCounterService.cs` | Game server POST `/tracker` |
| `PlayerKillCounterDataContainer` | `Digit.PrimeServer.Models` | `PlayerKillCounterDataContainer.cs` | Binary sync cache, `TryPopTracker` |
| `PlayerProfileManager` | `Digit.Prime.PlayerProfile` | `PlayerProfileManager.cs` | Singleton, `FetchPlayerKillCounter` |
| `PlayerProfileViewController` | `Digit.Prime.PlayerProfile` | `PlayerProfileViewController.cs` | 30s polling, `OnFetchPlayerKillCounterSuccess` |
| `CallbackContainer<T>` | `Digit.Networking.Core` | `CallbackContainer.cs` | Callback wrapper with `OnSuccess<T>`, `OnError` |
| `OnSuccess<T>` | `Digit.Networking.Core` | `OnSuccess.cs` | `delegate void OnSuccess<T>(T item)` |
| `Toast` | `Digit.Prime.HUD` | `Toast.cs` | Toast data object |
| `ToastState` | `Digit.Prime.HUD` | `ToastState.cs` | 60 toast visual states |
| `ToastTimeout` | `Digit.Prime.HUD` | `ToastTimeout.cs` | Default, Long, None |
| `ToastObserver` | `Digit.Prime.HUD` | `ToastObserver.cs` | Abstract base, `EnqueueToast` |
| `ToastViewController` | `Digit.Prime.HUD` | `ToastViewController.cs` | Queue management, widget resolution |
| `StandardToastWidget` | `Digit.Prime.HUD` | `StandardToastWidget.cs` | Text + icon widget for `Standard` state |
| `ToastManager` | `Digit.Prime.HUD` | `ToastManager.cs` | Singleton, expiry checking |
| `ToastLocaleSettings` | `Digit.Prime.HUD` | `ToastLocaleSettings.cs` | ScriptableObject with locale contexts |
| `ToastUIContext` | `Digit.Prime.HUD` | `ToastUIContext.cs` | `Queue<Toast> ToastQueue` |
| `LocaleTextContext` | `Digit.Client.UI` | `LocaleTextContext.cs` | Localization key + parameters |
| `GeneratedGameEvents` | `Digit.Prime.GeneratedEvents` | `GeneratedGameEvents.cs` | Analytics event enum |
| `NotificationManager` | `Digit.Prime.Notifications` | `NotificationManager.cs` | Notification queue, `TriggerShowUINotification` |
| `NotificationsService` | `Digit.PrimeServer.Services` | `NotificationsService.cs` | `InjectNotification(Notification)` |
| `UINotification` | `Digit.Prime.Notifications` | `UINotification.cs` | Notification UI wrapper |

---

## Part 9: Implementation Checklist

- [ ] **Choose threshold value** — 5, 10, or 25 (make configurable via BepInEx config)
- [ ] **Choose hook point** — `OnFetchPlayerKillCounterSuccess` (recommended), `TryPopTracker`, or `FetchKillCounterStat`
- [ ] **Implement threshold logic** — `s_lastNotifiedKillCount` tracking, delta computation
- [ ] **Create toast** — `ToastState.Standard`, `LocaleTextContext` with `OverrideBaseIdentifier`
- [ ] **Access toast queue** — reflection on `ToastObserver.EnqueueToast` or direct `ToastUIContext.ToastQueue`
- [ ] **Handle edge cases** — first load, counter reset, feature disabled, profile not visible
- [ ] **Test with BepInEx IL2CPP** — install in `BepInEx/plugins/`, launch game, verify toast appears
- [ ] **Optional: locale DB entry** — add a proper localization key for the toast text instead of `OverrideBaseIdentifier`

---

## Part 10: Architecture Diagram

```
                    ┌─────────────────────────────────────────────────┐
                    │              GAME SERVER                         │
                    │  POST /tracker → EntityGroup(type=9301)          │
                    │  → Tracker { counter: N, limit: M }              │
                    └────────────────────┬────────────────────────────┘
                                         │
                    ┌────────────────────▼────────────────────────────┐
                    │         PlayerKillCounterService                 │
                    │  FetchKillCounterStat(CallbackContainer<Tracker>)│
                    │  └─ PlayerKillCounterDataContainer               │
                    │     └─ TryPopTracker(out Tracker)                │
                    └────────────────────┬────────────────────────────┘
                                         │
                    ┌────────────────────▼────────────────────────────┐
                    │         PlayerProfileManager                     │
                    │  FetchPlayerKillCounter(callbacks)               │
                    │  └─ Checks _hostileKillCounterDisabled           │
                    └────────────────────┬────────────────────────────┘
                                         │
                    ┌────────────────────▼────────────────────────────┐
                    │    PlayerProfileViewController                   │
                    │  UpdateKillCounterText() [every 30s]             │
                    │  └─ OnFetchPlayerKillCounterSuccess(Tracker)     │
                    │     └─ Updates _killCounterText UI               │
                    └────────────────────┬────────────────────────────┘
                                         │
              ═══════════════════════════╪═══════════════════════════
                       HOOK POINT (Harmony postfix patch)
              ═══════════════════════════╪═══════════════════════════
                                         │
                    ┌────────────────────▼────────────────────────────┐
                    │         KillTrackerToastPlugin                   │
                    │  s_lastNotifiedKillCount: int                    │
                    │  s_threshold: int (5/10/25)                      │
                    │                                                  │
                    │  if delta >= threshold:                          │
                    │    ┌──────────────────────────────────────────┐  │
                    │    │  ShowKillToast(currentKills, delta)      │  │
                    │    │  └─ new Toast(Standard, LocaleTextCtx)   │  │
                    │    │     └─ ToastObserver.EnqueueToast(toast) │  │
                    │    └──────────────────────────────────────────┘  │
                    └────────────────────┬────────────────────────────┘
                                         │
                    ┌────────────────────▼────────────────────────────┐
                    │         ToastViewController                     │
                    │  ToastUIContext.ToastQueue                       │
                    │  └─ CheckToastQueue() coroutine                  │
                    │     └─ ShowNextToast()                           │
                    │        └─ TryResolveToastWidget(Standard)        │
                    │           └─ StandardToastWidget                 │
                    │              ├─ TextLocalizer (kill count text)  │
                    │              └─ ImageSelector (icon)             │
                    │  └─ TimeOutToast() → auto-dismiss                │
                    └─────────────────────────────────────────────────┘
```
