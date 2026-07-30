# Game Launch & Reload Lifecycle

Analysis of the STFC game launch process (from startup to user input) and the reload process (after logout or reconnect). All findings are confirmed against runtime lifecycle logger logs (2026-07-06, build 1.1.2.1).

---

## Table of Contents

1. [Game Launch Process](#game-launch-process)
2. [Reload Process](#reload-process-after-logout-or-reconnect)
3. [Key Classes](#key-classes)
4. [What's Re-done vs. What's Not](#whats-re-done-vs-whats-not)
5. [Reload-Related Bugs in the Mod](#reload-related-bugs-in-the-mod)

---

## Game Launch Process

### Phase 1: Engine & Runtime Initialization

1. **Unity/IL2CPP runtime starts** — loads the game assembly, initializes metadata
2. **Mod DLL loads** — `version.dll` proxy injects into the game process, mod hooks `il2cpp_init`, installs all patches
3. **`PrimeApp.InitializeLogger()`** — `[RuntimeInitializeOnLoadMethod(BeforeSceneLoad)]`, sets up Bugsnag/logger

### Phase 2: Scene & Core Object Creation

4. **Unity loads the initial scene** — instantiates all MonoBehaviours in the scene hierarchy
5. **`PrimeApp()` constructor** — creates the main app instance, sets up Bugsnag configuration, model registry config
6. **`HubGameObject()` constructor** — creates the root GameObject that will host all core systems
7. **`Hub.Init()`** — initializes all static `Hub` properties:
   - `Hub.App` → the `PrimeApp` instance
   - `Hub.UI` → new `UIElements` containing `TransitionManager` as `LoadingManager`
   - `Hub.UIRegistry`, `Hub.Locale`, `Hub.User`, `Hub.AssetBundles`, `Hub.Model`
   - `Hub.SceneTransition`, `Hub.Prefab`, `Hub.Events`
   - `Hub.SectionManager` → new `SectionManager` instance
   - `Hub.HudManager`, `Hub.JobsManager`, `Hub.TutorialManager`, etc.

### Phase 3: TransitionManager Initialization

8. **`TransitionManager.Awake()`** — TM created (state=Hidden, cc=0, ccNative=0, ccState=-1). BlurController created (blurTarget=1, tweenVal=0).

9. **`TransitionManager.OnEnable()`** — fires immediately after Awake.

### Phase 4: Platform & Login Initialization

10. **`LoginSequence.Awake()`**:
    - Sets `_instance = this`
    - Sets `CurrentLoginStage = LoadingEndpoints (0)`
    - Starts the login behavior tree (`_bt`)
11. **`PrimeApp.InitPlatformServer()`** — connects to Scopely platform servers, fetches config (IsReloading=false, SessionCount=0)
12. **Login stages progress** (`LoginStage` enum) — runtime order differs from enum order:

| Stage | Value | Description | User Input? | Runtime Order |
|-------|-------|-------------|-------------|---------------|
| `LoadingEndpoints` | 0 | Loads platform endpoint configuration | No | 1st |
| `CheckMaintenance` | 7 | Checks server maintenance status | No | 2nd |
| `Localization` | 1 | Loads localization/language data | No | 3rd |
| `LoadingAssets` | 3 | Downloads/loads required asset bundles | No | 4th |
| `Login2_0` | 6 | New login flow handling | No | 5th |
| `LoadingGame` | 5 | Connects to game server, performs full sync | No | 6th |
| `Environment` | 2 | Environment selection (auto in production, manual in dev) | No (prod) / Yes (dev) | Skipped (prod) |
| `LoginDetails` | 4 | Login UI appears, user taps to login, accepts TOS/GDPR | **Yes** | Skipped (prod) |
| `CloudStorageCheck` | 8 | Cloud save validation | No | Skipped (prod) |

### Phase 5: Section Navigation (after login)

13. **`SectionManager.TriggerSectionChange()`** — navigates to login section (section ID `73596745`, forced=true)
14. **`SetLoadingScreen(type=0 (SceneLoad))`** — TM stays Hidden, no visual transition. SectionStatus: prev=0, curr=0, next=73596745.
15. **`LoginSequence.Awake()`** fires, login stages progress (see Phase 4 above).
16. **`PrimeApp.InitPlatformServer()`** — connects to platform servers (IsReloading=false, SessionCount=0).
17. **`SectionManager.TriggerSectionChange()`** — navigates to main game section (section ID `385452890`)
18. **`ChangeSectionCoroutine`** runs:
    - `OnLeaveSection` phases 0, 3: return `Ignore`. Phases 1-2: `Ignore` then `Ok`.
    - **`SetLoadingScreen(type=1 (DirectorNotDownloaded))`** — TM goes Hidden→Showing, cc and ccNative created (ccState=0). `TVC.Awake` + `TVC.AboutToShow` fire.
    - **`SetLoadingScreen(type=0 (SceneLoad))`** — TM stays Showing, same cc/ccNative. SectionStatus: prev=0, curr=73596745, next=385452890.
    - `OnEnterSection` phase 1 polls `Ok` repeatedly (~1.3s) while resources load.
    - `OnEnterSection` phase 2: `Ok`
    - `TransitionManager.Hide()` — TM goes Shown→Hidden, `TVC.AboutToHide`
    - `OnEnterSection` phase 3: `Ok`
19. **User input required** — game UI is interactive (station, alliance, etc.)

---

## Reload Process (after logout or reconnect)

The reload is triggered by `PrimeApp.Reload()`, but **`Reload()` is asynchronous** — it returns immediately after triggering a section change, and `DoReload()` is called later (~2.3 seconds later in observed logs) after the intermediate section transition completes.

A `MonoSingleton.PrepareAllForReload()` call also occurs before `Reload()` — in observed logs, ~2.5 minutes earlier (21:49:28 vs 21:51:56). This is triggered by a websocket disconnect (observed: `WSSCLOSE` code 3503 "force disconnect" and code 3004 "internal server error"), which causes an error dialog to appear. The gap between `PrepareAllForReload` and `Reload()` is the time the user takes to click the relaunch button on the error dialog. `Reload()` is then called via `PrimeApp.ReloadWithReason(MessageBoxContext)`.

### Step 1: Reload Entry — Prepare & Trigger Section Change

1. **`PrimeApp.Reload(quickReloading, customReloadArgs, forceReload)`**:
   - Guards against double-reload
   - Calls `MonoSingleton.PrepareAllForReload()`

2. **`MonoSingleton.PrepareAllForReload()`**:
   - Iterates all registered `MonoSingleton` instances in `s_singletons` list
   - Calls `OnApplicationPrepareReload()` on each (virtual, default no-op)
   - Observed duration: ~21ms (inside Reload), ~10ms (pre-reload)

3. **`SectionManager.TriggerSectionChange()`** with intermediate reload section ID (forced=true):
   - Section ID: `-1545897473` (distinct from login section)
   - `OnLeaveSection` phase 0: returns `Ignore` (TM state=Hidden)
   - `SetLoadingScreen(type=2)` — TM goes Hidden→Showing, reuses existing TransitionViewController
   - `OnLeaveSection` phase 1: returns `NotOk` repeatedly while blur tween animates, then `Ok`
   - **`PrimeApp.Reload()` RETURNS** — the method exits here, before `DoReload` is called

4. **Section transition completes** (async, ~2.3 seconds observed):
   - `OnLeaveSection` phase 1 polls until blur completes (NotOk → Ok)
   - `OnLeaveSection` phases 2, 3: return `Ignore` (TM state=Shown)
   - `TransitionViewController.AboutToHide` on old TVC
   - `OnEnterSection` phases 0-2: return `Ok`
   - `TransitionManager.Hide()` — TM goes Shown→Hidden
   - `OnEnterSection` phase 3: `Ok`

### Step 2: DoReload — Core Reload Sequence

5. **`PrimeApp.DoReload(reloadArgs)`** — called AFTER section transition completes:
   - `IsReloading` is still `false` at DoReload entry. It is set to `true` before `GoToLoginSection` and back to `false` before DoReload returns.
   - `SessionCount` increments from 0 to 1 during DoReload.
   - **`PrimeApp.ClearStaticEvents()`** — removes ALL static event handlers (observed: ~133ms)
   - **`PrimeApp.ReloadHub(quickReloading)`** (observed ~561ms):
     - Hub static property pointers (SectionManager, UI, App) do NOT change after ReloadHub. The same pointers persist before and after.
     - The canvas controller (cc) and native canvas (ccNative) pointers do NOT change during ReloadHub. They change later, during `GoToLoginSection`.
     - **`TransitionManager.OnApplicationReload()`** is called during ReloadHub (see Step 3 below)
   - **`MonoSingleton.ReloadAll()`** (observed ~5ms):
     - Iterates all `MonoSingleton` instances
     - Calls `OnApplicationReload()` on each (virtual method)
     - `TransitionViewController.AboutToHide` fires multiple times for the old TVC during this step
   - **`PrimeApp.GoToLoginSection(quickReload)`** — triggers section change to login section. New cc/ccNative are created here.
   - After GoToLoginSection returns, DoReload sets `IsReloading = false` and returns

### Step 3: TransitionManager During Reload

6. **`TransitionManager.OnApplicationReload()`** — same TM instance persists throughout reload:
   - The same TransitionManager instance (same pointer) persists through the entire reload. There is NO new `TransitionManager.Awake()` or `OnEnable()` during reload.
   - `OnApplicationReload()` is called during `ReloadHub`
   - TM state, cc, ccNative, and ccState remain unchanged across `OnApplicationReload()` (state=Hidden, ccState=4)
   - TM `OnDisable()` and `OnDestroy()` do NOT fire during reload — they only fire at game shutdown

7. **New `TransitionViewController` created** — during GoToLoginSection:
   - A new TVC instance is created (different pointer) — observed: old TVC `22492d1cf00`, new TVC `225c9a09f00`
   - `TVC.Awake()` → `TVC.AboutToShow()` fire for the new instance
   - The old TVC gets `AboutToHide` called multiple times during `MonoSingleton.ReloadAll()` and subsequent steps
   - The `TransitionManager` itself is NOT recreated — only the `TransitionViewController` is new

### Step 4: Navigate to Login Section

8. **`PrimeApp.GoToLoginSection(quickReload)`**:
   - `IsReloading = true` at entry, `SessionCount = 0`
   - Calls `SectionManager.TriggerSectionChange()` with login section ID `73596745` (forced=true)
   - New cc and ccNative are created (different pointers from pre-reload values)
   - New TVC is created (Awake + AboutToShow fire)
   - Old TVC gets AboutToHide
   - After GoToLoginSection returns: `IsReloading = true`, `SessionCount = 0`
   - After DoReload returns: `IsReloading = false`, `SessionCount = 1`

### Step 5: Login Sequence Re-runs

9. **`LoginSequence` stages re-run** — same order for both launch and full reload:
   - **Quick reload**: starts at `LoadingGame (5)` — reconnects to game server, full sync
   - **Full reload**: starts at `LoadingEndpoints (0)` — full re-login including asset loading, TOS/GDPR, login UI
   - **Observed stage order** (production, full reload): `0` (LoadingEndpoints) → `7` (CheckMaintenance) → `1` (Localization) → `3` (LoadingAssets) → `6` (Login2_0) → `5` (LoadingGame)
   - Stages `2` (Environment), `4` (LoginDetails), and `8` (CloudStorageCheck) are skipped in production builds

---

## Key Classes

### `PrimeApp` (`Digit.Client.Core.PrimeApp`)

Inherits from `App`. The main application controller.

| Field | Offset | Purpose |
|-------|--------|---------|
| `IsReloading` | `0x50` | Flag indicating reload in progress |
| `_sessionCount` | `0x88` | Session counter, increments during DoReload |

| Method | Purpose |
|--------|---------|
| `Reload(quickReloading, customReloadArgs, forceReload)` | Entry point for reload. Guards against double-reload. Calls `PrepareAllForReload()`, triggers section change, then returns. `DoReload()` is called later via callback. |
| `ReloadWithReason(MessageBoxContext)` | Entry point for reload triggered by error dialog. Calls `Reload()`. |
| `StartReload(AppReloadArgs)` | Calls `DoReload()`. Not directly observed in lifecycle logger logs. |
| `DoReload(reloadArgs)` | Core reload orchestration: clears events, reloads Hub, reloads singletons, navigates to login. |
| `ReloadHub(quickReloading)` | Re-initializes Hub static properties. Pointers for SectionManager, UI, App do NOT change. |
| `GoToLoginSection(quickReload)` | Sets up scene args and triggers section change to login section. |
| `ReloadNetworkServices(bool)` | Re-initializes network/game server connections. Not directly observed in lifecycle logger logs. |
| `ClearStaticEvents()` | Removes all static event handlers across dozens of static classes. |
| `InitPlatformServer()` | Connects to Scopely platform servers. Called during launch and after reload. |

### `TransitionManager` (`Digit.Prime.LoadingScreen.TransitionManager`)

| Field | Offset | Type | Purpose |
|-------|--------|------|---------|
| `_settings` | `0x20` | `TransitionSettings` | Serialized settings |
| `_loadingScreenPrefab` | `0x28` | `GameObject` | Prefab for loading screen |
| `_material` | `0x30` | `Material` | Blur shader material |
| `_itemsToHideSetAsset` | `0x38` | `AssetBundleResource` | Items to hide during transition |
| `_blurTime` | `0x40` | `float` | Blur tween duration |
| `_waitFrames` | `0x44` | `int` | Frames to wait before blur removal |
| `_itemsToHideSet` | `0x48` | `CanvasGroupRuntimeSet` | Runtime set for dimming |
| `_canvasController` | `0x50` | `CanvasController` | Canvas controller for transition UI |
| `_currentState` | `0x58` | `State` (enum) | Showing=0, Shown=1, Hidden=2 |
| `BlurController` | `0x60` | `BlurController` | Blur tween controller |
| `_loadStart` | `0x68` | `Coroutine` | Load() coroutine reference |
| `_textureOverride` | `0x70` | `Texture2D` | Custom texture override |
| `_fpsChanged` | `0x78` | `bool` | FPS changed flag |
| `_sharedMaterial` | `0x80` | `Material` | Shared material instance |

| Method | Purpose |
|--------|---------|
| `Awake()` | TM created (state=Hidden, cc=0, ccState=-1). BlurController created (blurTarget=1, tweenVal=0). |
| `OnEnable()` | Fires immediately after `Awake()` during launch |
| `OnDisable()` | Fires at game shutdown (observed: ~140ms before `OnDestroy`) |
| `OnDestroy()` | Fires at game shutdown, after `OnDisable()` |
| `OnApplicationReload()` | Called during `ReloadHub`. TM state and cc remain unchanged. |
| `SetLoadingScreen(status, type, messagingType)` | Starts a transition: sets state based on type, may create canvas and TVC |
| `Hide(status)` | Hides transition: TM goes Shown→Hidden, `TVC.AboutToHide` fires |
| `CheckResourcesOnLeave(status, phase, storage)` | Phase 0 handler. Observed returning `Ignore` during section transitions. |
| `OnEnterSection(status, phase, storage)` | Phase 0: `Ok`. Phase 1: polls `Ok` while resources load. Phase 2: `Ok` then calls `Hide()`. Phase 3: `Ok`. |
| `OnLeaveSection(status, phase, storage)` | Phase 0: `Ignore`. Phase 1: polls `NotOk` while blur animates, then `Ok`. Phases 2-3: `Ignore`. |

### `SectionManager` (`Digit.Client.Sections.SectionManager`)

| Event/Field | Purpose |
|-------------|---------|
| `OnEnterSection` | Delegate called during section entry (phases 0-3: PrepareSectionActivation, CheckIfAllReadyForActivation, ActivateSection, SectionActivated) |
| `OnLeaveSection` | Delegate called during section exit (phases 0-4: CheckingIfOkToLeave, WaitUntilReadyToDeactivate, Deactivate, CheckIfDeactivationIsCompleted, SectionWasDeactivated) |
| `ChangeSectionCoroutine` | Coroutine that drives section transitions, calling delegates phase by phase |

**`CheckFeedback` return values:**

| Value | Name | Meaning |
|-------|------|---------|
| 0 | `Ignore` | Skip this listener for this phase |
| 1 | `NotOk` | Not ready — coroutine will retry this phase after `_recheckDelayInSec` |
| 2 | `Ok` | Ready — proceed to next phase |
| 3 | `SectionDropped` | Section was dropped during transition |

> If any listener returns `NotOk`, the coroutine retries the same phase forever (no max retry count). This is the most dangerous stall condition.

### `MonoSingleton` / `MonoSingleton<T>`

Base class for Unity singletons that survive reload.

| Method | Purpose |
|--------|---------|
| `PrepareAllForReload()` | Iterates `s_singletons`, calls `OnApplicationPrepareReload()` on each |
| `ReloadAll()` | Iterates `s_singletons`, calls `OnApplicationReload()` on each. Observed: ~5ms. |
| `OnApplicationPrepareReload()` | Virtual — default no-op. Override to prepare for reload. |
| `OnApplicationReload()` | Virtual — default no-op. Override to clean up and re-register. |

### `LoginSequence` (`Digit.Prime.Login.LoginSequence`)

| Field | Offset | Purpose |
|-------|--------|---------|
| `_instance` | `0x08` (static) | Static singleton instance |
| `_bt` | `0x28` | Behavior tree owner |
| `_nextSection` | `0x38` | Next section ID after login |
| `InitialStage` | (static property) | Starting stage for login (set by `GoToLoginSection`) |
| `_loginStage` | `0xB0` | Current login stage |

| Method | Purpose |
|--------|---------|
| `Awake()` | Sets `_instance`, sets `CurrentLoginStage` from `InitialStage`, starts behavior tree |
| `UpdateLoginStage()` | Processes current stage, transitions to next when complete |
| `set_CurrentLoginStage(value)` | Sets stage, fires `OnLoginStateChanged` event |

### `Hub` (`Digit.Client.Core.Hub`)

Static class holding references to all core game systems.

| Property | Type | Purpose |
|----------|------|---------|
| `App` | `PrimeApp` | Main app instance |
| `UI` | `UIElements` | Contains `LoadingManager` (TransitionManager) |
| `SectionManager` | `SectionManager` | Section navigation controller |
| `AssetBundles` | `AssetBundleManager` | Asset bundle loading/caching |
| `Model` | `Model` | Data model |
| `Locale` | `Locale` | Localization |
| `User` | `User` | User account data |
| `SceneTransition` | `SceneTransitionManager` | Scene loading |
| `Prefab` | `PrefabManager` | Prefab instantiation |
| `Events` | `EventManager` | Event system |
| `HudManager` | `HUDManager` | HUD management |
| `JobsManager` | `JobsManager` | Jobs system |

### `UIElements` (`Digit.Client.Core.UIElements`)

| Field | Offset | Purpose |
|-------|--------|---------|
| `LoadingManager` | `0x10` | Reference to `TransitionManager` |

### `TransitionViewController` (`Digit.Prime.LoadingScreen.TransitionViewController`)

| Method | Purpose |
|--------|---------|
| `Awake()` | Fires when new TVC is created. Observed during initial `SetLoadingScreen(type=1)` and during `GoToLoginSection` after reload. |
| `AboutToShow()` | Fires after `Awake()` when TVC becomes visible. |
| `AboutToHide()` | Fires when TVC is being hidden, during `TransitionManager.Hide()`. Old TVC gets `AboutToHide` multiple times during reload. |

---

## What's Re-done vs. What's Not

### Re-done on reload

- **All static events** — cleared via `ClearStaticEvents()` and re-registered
- **All MonoSingletons** — `OnApplicationReload()` called on each
- **Section navigation** — restarts from login section
- **LoginSequence** — re-runs from appropriate stage
- **TransitionViewController** — new TVC instance created during `GoToLoginSection`
- **Canvas controller** — new `CanvasController` and native canvas created during `GoToLoginSection` (cc and ccNative pointers change)
- **Hub internal components** — TVC and canvas are recreated during `GoToLoginSection`, but Hub static property pointers (SectionManager, UI, App) do NOT change

### NOT re-done on reload

- **Unity engine** — no re-initialization
- **IL2CPP runtime** — no re-initialization
- **Mod DLL** — not reloaded, all hooks remain active
- **Unity scene** — not reloaded (GameObjects persist)
- **PrimeApp instance** — reused (not destroyed/recreated, same pointer throughout)
- **TransitionManager** — same TM instance persists throughout reload. `OnApplicationReload` is called for cleanup, but the TM is NOT destroyed/recreated. `OnDisable`/`OnDestroy` only fire at game shutdown.
- **BlurController** — same BlurController instance persists (same pointer throughout reload)

---

## Mod Interaction with Lifecycle

The community mod hooks into the following lifecycle events:

| Lifecycle Event | Mod Hook | Action |
|----------------|----------|--------|
| `il2cpp_init` | Mod entry point | Installs all patches |
| `LoginSequence.Awake` (step 7/15) | `loading_screen.cc` | Replaces login BG with custom texture, adds logos |
| `TVC.Awake` (step 11/12) | `transition_screen.cc` | Resets state, applies transition customization as fallback |
| `TVC.AboutToShow` (step 11/12) | `transition_screen.cc` | Applies custom BG + logos (or logos-only in black mode) |
| `TVC.AboutToHide` (step 14) | `transition_screen.cc` | Re-enables canvas animator for hide animation |
| `MonoSingleton.PrepareAllForReload` (reload step 1/3) | `transition_screen.cc` | Nulls stale overlay pointers, resets textures and tip state |
| `LoadingTipViewController.SetRandomTipLocalisedText` | `loading_tip.cc` | Overrides tip text (welcome tip on loading screen, 50% custom tips on transitions) |
| `LoadingTipViewController.OnEnable` | `loading_tip.cc` | Resets tip counter for each new transition cycle |

### Mod State During Reload

The mod's `PrepareAllForReload` hook cleans up before Unity destroys objects:
- Nulls all overlay GameObject pointers (logos, BG overlay)
- Resets texture pointers (textures reloaded on next access)
- Resets loading screen state (`g_loginLogoGO`, `g_loginCCLogoGO`)
- Resets transition screen state (`g_logoGO`, `g_ccLogoGO`, `g_bgOverlayGO`, etc.)
- Resets loading tip state (`g_tipCount`, `g_isLoadingScreen`, `g_lastCustomTipIdx`)

After reload, `TVC.Awake` fires for the new `TransitionViewController` and re-applies customization. `LoginSequence.Awake` fires for the new login sequence and re-applies the login screen background.

---

## Runtime Log Reference

### SetLoadingScreen Types

| Type | Name | TM State Change | TVC Behavior |
|------|------|-----------------|--------------|
| 0 | `SceneLoad` | Varies — may stay Hidden or stay Showing | May create new TVC (Awake + AboutToShow) |
| 1 | `DirectorNotDownloaded` | Hidden → Showing | Creates/shows TVC, cc and ccNative created |
| 2 | `DirectorNotInstantiated` | Hidden → Showing | Reuses existing TVC |

### Section Enter Phases

| Phase | Name | Typical TM Result |
|-------|------|-------------------|
| 0 | `PrepareSectionActivation` | `Ok` |
| 1 | `CheckIfAllReadyForActivation` | `Ok` (polls repeatedly while resources load) |
| 2 | `ActivateSection` | `Ok` (then calls `Hide()`) |
| 3 | `SectionActivated` | `Ok` |

### Section Leave Phases

| Phase | Name | Typical TM Result |
|-------|------|-------------------|
| 0 | `CheckingIfOkToLeave` | `Ignore` |
| 1 | `WaitUntilReadyToDeactivate` | `NotOk` → `Ok` (polls while blur animates) |
| 2 | `Deactivate` | `Ignore` |
| 3 | `CheckIfDeactivationIsCompleted` | `Ignore` |
| 4 | `SectionWasDeactivated` | — |

### TransitionManager State Machine

| TM State | ccState | Description |
|----------|---------|-------------|
| Hidden | -1 | Initial state, no canvas controller |
| Hidden | 3 | Hidden with canvas controller present |
| Hidden | 4 | Hidden, canvas was previously shown |
| Showing | 0 | Starting to show, canvas initializing |
| Showing | 1 | Loading/showing, blur tween animating |
| Shown | 4 | Fully shown, blur complete |

### Section IDs

| Section ID | Hex | Purpose |
|-----------|-----|----------|
| `73596745` | `0x462FF49` | Login section |
| `385452890` | `0x16F8B1AA` | Main game/station section |
| `-1545897473` | `0xA3843CAF` | Reload intermediate section (forced, inside `Reload()`) |
| `1937864029` | `0x7370F2BD` | Reentry section (forced, after websocket disconnect error dialog) |
| `-100955065` | `0xF9FFCBB7` | View section (e.g., alliance) |
| `-934817094` | `0xC898416A` | View section |
| `-1300757605` | `0xB2718FAB` | View section |
| `2138751318` | `0x7F60E1D6` | View section |
| `1742145916` | `0x67D7F4BC` | View section |
| `-763928158` | `0xD2834C22` | View section |
| `1180221277` | `0x4650F95D` | View section |
| `492322110` | `0x1D5791DE` | View section (post-reload) |
| `-72573081` | `0xFBAE0B57` | View section (post-reload) |

### Key Timings

| Step | Duration |
|------|----------|
| `MonoSingleton.PrepareAllForReload` | ~10ms (pre-reload), ~21ms (in Reload) |
| `PrimeApp.ClearStaticEvents` | ~133ms |
| `PrimeApp.ReloadHub` (including `TM.OnApplicationReload`) | ~561ms |
| `MonoSingleton.ReloadAll` | ~5ms |
| Async gap between `Reload()` return and `DoReload()` | ~2.3s |
| Pre-reload `PrepareAllForReload` to `Reload()` | ~2.5 min (user input — error dialog relaunch button) |
| Full reload (`Reload()` to `LoadingGame` stage) | ~6.5s |
| `TM.OnDisable` to `TM.OnDestroy` (game shutdown) | ~140ms |
