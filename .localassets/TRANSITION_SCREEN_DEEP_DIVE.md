# Transition Screen Deep Dive — Architecture, Controls, and Hang Analysis

## 1. System Overview

The transition screen is a **distributed system** spanning four namespaces, coordinating scene loading, asset bundle downloads, section lifecycle gating, post-processing blur, and UI overlay.

```
SectionManager (Digit.Client.Sections)
  ├─ ChangeSectionCoroutine() — phases with CheckFeedback gating
  ├─ OnEnterSection / OnLeaveSection delegates
  └─ NotOk = stall forever until callback resolves

TransitionManager (Digit.Prime.LoadingScreen)
  ├─ BlurController — CommandBuffer post-processing
  ├─ _loadingScreenPrefab — UI canvas instance
  ├─ State: Showing → Shown → Hidden
  └─ Hooks OnEnterSection / OnLeaveSection

SceneTransitionManager (Digit.Client.SceneManagement)
  ├─ BehaviourTree-driven scene load queue
  ├─ _sceneLoadActionInProgress gate
  └─ LEVEL / LEVEL_ASYNC / ADDITIVE / ADDITIVE_ASYNC

TransitionViewController (Digit.Prime.LoadingScreen)
  ├─ _animator — Show / Hide / IsDownloading states
  ├─ _scroller — downloading bundle list
  ├─ _progress / _stateLocalizer — text
  └─ Asset bundle event handlers
```

---

## 2. Core Classes

### 2.1 `SectionManager` — The Orchestrator

**Namespace:** `Digit.Client.Sections`

**Critical Properties:**
| Property | Type | Purpose |
|----------|------|---------|
| `_sectionChangeInProgress` | `bool` | Prevents overlapping section changes |
| `_activationState` | `SectionStatus` | Target section being activated |
| `_currentlyActiveCoroutine` | `Coroutine` | Active `ChangeSectionCoroutine` |
| `_recheckDelayInSec` | `float` | Delay between retries when callback returns `NotOk` |

**Delegates:**
| Delegate | Hooked By |
|----------|---------|
| `OnEnterSection(status, phase, storage) → CheckFeedback` | TransitionManager, SectionChangeInstrumentation, all SectionDirectorBases |
| `OnLeaveSection(status, phase, storage) → CheckFeedback` | Same as above |
| `OnSectionChangeCompleted(status)` | Cleanup / analytics |
| `OnSectionChangeExitedEarly(status)` | Timeout / abort |

**Section Leave Phases (`SectionLeavePhases`):**
| Phase | Value | Description |
|-------|-------|-------------|
| `_01_CheckingIfOkToLeave` | `0` | Ask listeners: safe to leave? |
| `_01_5_WaitUntilReadyToDeactivate` | `1` | Stall until ready |
| `_02_Deactivate` | `2` | Actually deactivate |
| `_03_CheckIfDeactivationIsCompleted` | `3` | Verify cleanup |
| `_04_SectionWasDeactivated` | `4` | Old section fully gone |

**Section Enter Phases (`SectionEnterPhases`):**
| Phase | Value | Description |
|-------|-------|-------------|
| `_01_PrepareSectionActivation` | `0` | Preload assets, instantiate prefabs |
| `_02_CheckIfAllReadyForActivation` | `1` | Stall until preloads complete |
| `_03_ActivateSection` | `2` | Show new section |
| `_04_SectionActivated` | `3` | New section fully active |

**`CheckFeedback` enum:**
| Value | Meaning |
|-------|---------|
| `Ignore = 0` | Don't care |
| `NotOk = 1` | **NOT READY — stall and retry** |
| `Ok = 2` | Proceed |
| `SectionDropped = 3` | Section was dropped, skip remaining phases |

**Critical rule:** If **any** callback returns `NotOk`, `ChangeSectionCoroutine()` yields `WaitForSeconds(_recheckDelayInSec)` and retries the **same phase forever** until it gets `Ok`.

---

### 2.2 `TransitionManager`

**Namespace:** `Digit.Prime.LoadingScreen`  
**Role:** Owns blur, loading screen prefab, and gates section changes.

**Fields:**
| Field | Type | Purpose |
|-------|------|---------|
| `_settings` | `TransitionSettings` | ScriptableObject config |
| `_loadingScreenPrefab` | `GameObject` | UI prefab to instantiate |
| `_material` | `Material` | Blur shader material |
| `_itemsToHideSetAsset` | `AssetBundleResource` | RuntimeSet of CanvasGroups to dim |
| `_blurTime` | `float` | Blur tween duration |
| `_waitFrames` | `int` | Frames to wait before blur removal |
| `_itemsToHideSet` | `CanvasGroupRuntimeSet` | Resolved UI dim targets |
| `_canvasController` | `CanvasController` | Instantiated loading screen canvas |
| `_currentState` | `State` | `Showing` (0), `Shown` (1), `Hidden` (2) |
| `_loadStart` | `Coroutine` | The `Load()` coroutine handle |
| `_textureOverride` | `Texture2D` | Static image instead of blur |
| `_sharedMaterial` | `Material` | Runtime material instance |

**State Machine:**
```
Hidden ──[SetLoadingScreen()]──► Showing ──[Load() completes]──► Shown
  ▲                                                             │
  └────────[Hide() / CanHide()==true]────────────────────────────┘
```

**Key Methods:**
| Method | Role |
|--------|------|
| `SetLoadingScreen(status, type, messagingType)` | Spawns prefab, starts blur, sets `Showing`, kicks `Load()` |
| `Load()` | IEnumerator. Waits for scene/resources, then `Showing → Shown` |
| `Hide(status)` | Dismisses loading screen. Only works if `CanHide()` is true |
| `CanHide()` | Returns whether it's safe to hide |
| `OnEnterSection(...)` | Shows loading screen during `_01_PrepareSectionActivation` |
| `OnLeaveSection(...)` | May hide loading screen during leave |
| `UpdateMessagingType(messagingType)` | Switches text between `Default` and `DownloadError` |

---

### 2.3 `BlurController`

**Namespace:** `Digit.Prime.LoadingScreen`  
**Role:** GPU-based blur via `CommandBuffer`.

**Fields:**
| Field | Type | Purpose |
|-------|------|---------|
| `_blitBlurToScreen` | `CommandBuffer` | Camera render injection |
| `_blurTweenValue` | `float` | Current blur strength (0 = clear, 1 = full) |
| `_blurTweenTarget` | `float` | Target blur strength |
| `_waitFrames` | `float` | Frames to stall before blur removal starts |
| `_textureOverride` | `Texture2D` | Static image override |
| `_material` | `Material` | Blur shader material |
| `_blurTime` | `float` | Seconds for full tween |
| `ItemsToHideSet` | `CanvasGroupRuntimeSet` | CanvasGroups whose alpha → 0 during blur |

**Methods:**
| Method | Role |
|--------|------|
| `SetBlur()` | Snap blur to full strength |
| `SetClear()` | Snap blur to zero |
| `TickBlurTransition()` | Frame lerp `_blurTweenValue → _blurTweenTarget` |
| `RemoveBlur()` | Coroutine. Wait `_waitFrames`, then tween to 0 over `_blurTime` |
| `ForceCompletion()` | Instantly snap to target (emergency unstuck) |
| `IsClear()` | `_blurTweenValue == 0` |
| `IsTweeningBlur()` | Currently increasing blur |
| `IsTweeningClear()` | Currently decreasing blur |

---

### 2.4 `TransitionViewController` — UI Layer

**Namespace:** `Digit.Prime.LoadingScreen`

**Animator Parameters (AnimVars):**
| Hash | Name | Type | Purpose |
|------|------|------|---------|
| `IsDownloadingHash` | `IsDownloading` | Bool | Shows/hides download panel |
| `ShowIndividual` | `ShowIndividual` | Trigger | Pop-in single bundle name |
| `HideIndividual` | `HideIndividual` | Trigger | Hide bundle name popup |

**Serialized Fields:**
| Field | Type | Purpose |
|-------|------|---------|
| `_staticOverride` | `Image` | Static full-screen image |
| `_factionIcons` | `GameObject[]` | Faction-specific artwork |
| `_animator` | `Animator` | Show/Hide/Downloading state machine |
| `_scroller` | `SmartScroller` | Downloading bundle name list |
| `_progress` | `TextLocalizer` | Percentage text |
| `_stateLocalizer` | `TextLocalizer` | Status text |
| `_defaultProgressContext` | `LocaleTextContext` | Default localized string |
| `_downloadErrorProgressContext` | `LocaleTextContext` | Error string |

**Download Tracking:**
| Field | Type | Purpose |
|-------|------|---------|
| `_isDownloading` | `bool` | Any bundle actively downloading |
| `_minDownloadingAmmount` | `long` | Threshold before showing download UI |
| `_currentDownloadingAmmount` | `long` | Bytes downloaded this frame |
| `_totalDownloadingAmmout` | `long` | Total bytes to download |
| `_downloadingBundleContexts` | `List` | Active download jobs |
| `_inProgress` | `List<ushort>` | Bundle IDs in progress |
| `_sortedInProgress` | `List<ushort>` | Sorted display list |
| `_lastProgress` | `Dictionary<ushort, float>` | Per-bundle last known % |
| `_bytesDownloaded` | `float` | Accumulated bytes for math |
| `_showingIndividual` | `bool` | Bundle name popup visible? |
| `_individualShowTime` | `float` | How long current bundle name shown |
| `_maxIndividualShowTime` | `static int` | Max time before auto-hide |
| `_isHiding` | `bool` | Loading screen animating out? |

**Event Handlers:**
| Method | Trigger | Action |
|--------|---------|--------|
| `OnAssetBundleDidBeginDownloadEventCallback(ID, downloading)` | Bundle download starts | Add to `_inProgress`, show download UI |
| `DidAssetBundleDownloadCompleteEvent(ID, error)` | Bundle download ends | Remove from `_inProgress`, update progress |
| `SectionManagerOnOnEnterSection` | Enter phases | Set progress text per phase |
| `SectionManagerOnOnLeaveSection` | Leave phases | Set progress text per phase |

---

### 2.5 `SectionDirectorBase`

**Namespace:** `Digit.Client.Sections`  
**Role:** Base for every section (Navigation, Shop, Chat, Starbase, etc.).

**Critical Fields:**
| Field | Type | Purpose |
|-------|------|---------|
| `_myState` | `State` | `NotLoaded` → `PreLoaded` → `Active` → `Sleeping` → `Idle` |
| `_mySectionId` | `SectionID` | Owned section |
| `_hasBackButton` | `bool` | Back-button logic applies? |
| `_highMemorySetting` / `_lowMemorySetting` | `MemorySetting` | Memory behavior per device tier |
| `_hdLoadingScreenSetting` / `_mdLoadingScreenSetting` / `_ldLoadingScreenSetting` | `LoadingScreenSetting` | When to show loading screen |
| `_sectionLoadFailed` | `bool` | True after `MAX_LOAD_RETRIES` failures |
| `_currentLoadRetryCount` | `int` | Current retry (0–3) |
| `_errorMessageBoxContext` | `MessageBoxContext` | Popup on load failure |

**`LoadingScreenSetting`:**
| Value | Meaning |
|-------|---------|
| `Always = 0` | Always show loading screen |
| `IfNotSleeping = 1` | Only show if section not already in memory |
| `Never = 2` | Never show |

**`MemorySetting`:**
| Value | Meaning |
|-------|---------|
| `PreInstantiate = 0` | Load early |
| `KeepAfterLoad = 1` | Stay in memory after use |
| `Default = 2` | Standard cleanup |

**Constant:** `MAX_LOAD_RETRIES = 3`

---

### 2.6 `SceneTransitionManager`

**Namespace:** `Digit.Client.SceneManagement`

**Fields:**
| Field | Type | Purpose |
|-------|------|---------|
| `_bt` | `BehaviourTreeOwner` | NodeCanvas BT driving scene changes |
| `_bb` | `Blackboard` | Shared BT state |
| `_sceneStructure` | `SceneStructure` | Scene hierarchy metadata |
| `_sceneChangeRequestQueue` | `SceneChangeRequestQueue` | FIFO queue |
| `_sceneLoadActionInProgress` | `bool` | Unity actively loading a scene |
| `_currentReq` | `SceneChangeRequest` | Current request |
| `_stats` | `SceneChangeStats` | Timing stats |

**`ChangeType`:** `LEVEL`, `LEVEL_ASYNC`, `ADDITIVE`, `ADDITIVE_ASYNC`, `ADDITIVE_UNLOAD`, `IGNORE`

---

### 2.7 `SceneAssetPreloadSectionDirector`

**Namespace:** `Digit.Client.Sections`  
**Role:** Preloads a scene asset bundle before section enter.

| Field | Type | Purpose |
|-------|------|---------|
| `_sceneAsset` | `AssetBundleResource` | Scene asset bundle |
| `_sceneInstance` | `GameObject` | Instantiated scene root |
| `OnSceneInstanceSet` | `Action` | Fired when scene ready |

---

### 2.8 `SectionChangeInstrumentation`

**Namespace:** `Prime.Diagnostics.Instrumentations`  
**Role:** Traces section changes, detects timeout, logs blocking callbacks.

| Field | Type | Purpose |
|-------|------|---------|
| `SECTION_CHANGE_TIMEOUT` | `static readonly float` | Timeout threshold (seconds) |
| `_sectionChangeTimer` | `float` | Elapsed time since change started |
| `_sectionChangeTimeoutEventFired` | `bool` | True if timeout already fired |
| `CurrentPhaseBlockingCallbacks` | `Dictionary` | Callbacks returning `NotOk` |
| `OnSectionChangeTimeoutEvent` | `Action` | Fires on timeout |
| `_tracer` | `Tracer` | DataDog / OTel tracer |

---

## 3. Normal Transition Flow

```
TriggerSectionChange(nextSectionID)
    │
    ▼
ChangeSectionCoroutine() starts
    │
    ├─► [LEAVE] current section
    │    ├─ _01_CheckingIfOkToLeave → callbacks return Ok
    │    ├─ _01_5_WaitUntilReadyToDeactivate
    │    ├─ _02_Deactivate
    │    ├─ _03_CheckIfDeactivationIsCompleted
    │    └─ _04_SectionWasDeactivated
    │
    ├─► TransitionManager.OnLeaveSection() → may hide blur
    │
    ├─► [ENTER] next section
    │    ├─ _01_PrepareSectionActivation
    │    │   └─ TransitionManager.OnEnterSection() → Show loading screen
    │    │   └─ SceneAssetPreloadSectionDirector.Preload() → load bundle
    │    │   └─ SectionDirectorBase.PreInstantiate() → instantiate
    │    │
    │    ├─ _02_CheckIfAllReadyForActivation
    │    │   └─ TransitionManager checks: scene loaded? bundles done?
    │    │   └─ If not → returns NotOk → coroutine waits
    │    │
    │    ├─ _03_ActivateSection
    │    │   └─ Director shows canvas
    │    │   └─ BlurController.RemoveBlur() starts
    │    │
    │    └─ _04_SectionActivated
    │        └─ TransitionManager.Hide()
    │        └─ Loading screen animates out
    │
    └─► OnSectionChangeCompleted → cleanup
```

**Blur Timeline:**
```
SetLoadingScreen() → BlurController.SetBlur() → _blurTweenValue = 1 (instant)
                   → State = Showing
                   → Load() coroutine starts

Load() completes   → State = Shown

Hide() called      → BlurController.RemoveBlur()
                   → Wait _waitFrames frames
                   → _blurTweenTarget = 0
                   → TickBlurTransition() each frame
                   → IsClear() → destroy loading screen GameObject
```

---

## 4. UI Element Inventory — Exact Prefab Data

### 4.1 `TransitionScreen_Canvas` — Main Loading Screen (from `sharedassets0.assets`, path_id=1346)

This is the **full-screen transition prefab** instantiated by `TransitionManager._loadingScreenPrefab`.

| Element | Anchor | Position | Size | Description |
|---------|--------|----------|------|-------------|
| **TransitionScreen_Canvas** | `bottom-left` | `(0, 0)` | `(0, 0)` | Root canvas (has `RaycastBlockerRaycaster`) |
| **LoadingScreenText** | `center` | `(0, -234.7)` | `(1334, 119)` | Main loading title text (e.g., "Loading…") |
| **LoadingIndicatorContainer** | `bottom-right` | `(-45.2, 34.4)` | `(59.8, 50)` | Spinner container — bottom-right corner |
| &nbsp;&nbsp;`ClockWise` | `center` | `(0, 0)` | `(39, 39)` | Clockwise spinner |
| &nbsp;&nbsp;`AntiClockWise` | `center` | `(0, 0)` | `(33, 33)` | Counter-clockwise spinner |
| **Numbers_BottomLeft** | `bottom-left` | `(300.2, 32.6)` | `(545, 27.3)` | Progress numbers area |
| **IndividualProgressContainer** | `bottom-left` | `(680.8, 750)` | `(1306.3, 704)` | **Bundle download scroller** (Smart Scroller) |
| &nbsp;&nbsp;`Viewport` | `full-screen` | `(0, 0)` | `(0, 0)` | Scroll viewport |
| &nbsp;&nbsp;&nbsp;&nbsp;`Content` | `full-screen` | `(0, 0)` | `(0, 0)` | Scrolling content |
| &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`IndividualProgressItem` | `bottom-left` | `(0, 0)` | `(0, 19.7)` | Per-bundle row template |
| &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`IndividualProgressNumber` | `top-left` | `(145.4, -9.9)` | `(23.6, 19.7)` | Bundle % text |
| &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`IndividualProgressName` | `top-left` | `(0, -9.9)` | `(130.8, 19.7)` | Bundle name text |
| &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`IndividualProgress:` | `top-left` | `(138.1, -9.9)` | `(4.6, 19.7)` | Colon separator |
| **LogoContainer** | `center` | `(0, 0)` | `(586.4, 248)` | **Faction logo container** — centered |
| **StatusContainer** | `bottom-right` | `(-81.8, 32.6)` | `(0, 47.8)` | Status bar — bottom-right |
| &nbsp;&nbsp;`Separater` | `bottom-left` | `(0, 0)` | `(0, 22)` | Divider line |
| &nbsp;&nbsp;`MB_Progress` | `top-left` | `(290.8, -24.4)` | `(75.7, 23.9)` | Progress % text (e.g., "45%") |
| &nbsp;&nbsp;`DownloadingText` | `top-left` | `(184.2, -24.4)` | `(185.2, 46.2)` | "Downloading…" status label |
| **InitializingText** | `bottom-right` | `(-81.8, 28.8)` | `(0, 26.8)` | Initializing status — overlaps StatusContainer |
| **InputBlocker** | `full-screen` | `(0, 0)` | `(0, 0)` | Invisible full-screen touch blocker |
| **BGContainer** | `full-screen` | `(0, 0)` | `(0, 0)` | Background / blur container |
| &nbsp;&nbsp;`BG` | `full-screen` | `(-37, 100)` | `(1313, 2103)` | Background image — massive, full screen + overscan |
| **Button** | `bottom-right` | `(-185.4, 48.1)` | `(370.8, 96.3)` | Action button (e.g., "Cancel", "Retry") |
| **LoadingTipsContainer** | `center` | `(0, -180)` | `(1024, 100)` | **Loading tips carousel** — see Section 4.3 |
| &nbsp;&nbsp;`TipTextBg` | `mid-stretch` | `(0, 25)` | `(0, 0)` | Tip background panel |
| &nbsp;&nbsp;&nbsp;&nbsp;`TipText` | `bottom-left` | `(0, 0)` | `(900, 0)` | Tip text content (auto-height) |

### 4.2 `PreBundledLoading_Canvas` — Early Spinner (from `level0`, path_id=98)

Minimal spinner shown **before asset bundles are loaded** — no text, no logo, no scroller.

| Element | Anchor | Position | Size | Description |
|---------|--------|----------|------|-------------|
| **PreBundledLoading_Canvas** | `full-screen` | `(0, 0)` | `(0, 0)` | Root canvas |
| **LoadingSpinnerContainer** | `full-screen` | `(0, 0)` | `(0, 0)` | Spinner parent |
| &nbsp;&nbsp;`Anticlockwise` | `center` | `(0, 0)` | `(118, 118)` | Large counter-clockwise spinner |
| &nbsp;&nbsp;`Clockwise` | `center` | `(0, 0)` | `(128, 128)` | Large clockwise spinner |

### 4.3 `LoadingTipsContainer` — Loading Tip Carousel

**Controller:** `LoadingTipViewController` (`Prime.LoadingScreen`) — separate from `TransitionViewController`.

| Element | Anchor | Position | Size | Description |
|---------|--------|----------|------|-------------|
| **LoadingTipsContainer** | `center` | `(0, -180)` | `(1024, 100)` | Root container — centered, below logo |
| **TipTextBg** | `mid-stretch` | `(0, 25)` | `(0, 0)` | Background panel (horizontal stretch) |
| **TipText** | `bottom-left` | `(0, 0)` | `(900, 0)` | `TextMeshProUGUI` — auto-height, 900px wide |

**`LoadingTipViewController` Fields:**
| Field | Type | Purpose |
|-------|------|---------|
| `_textMeshPro` | `TextMeshProUGUI` | Text renderer (linked to `TipText`) |
| `_loadingScreenTipTextLocalizer` | `TextLocalizer` | Localization wrapper |
| `_loadingTipAnimatorController` | `Animator` | Drives `TipFadeIn` / `TipFadeOut` |
| `_loadingTipBandKey` | `string` | Localization prefix for **level-banded** tips |
| `_loadingTipBackupKey` | `string` | Fallback key if banded tips fail |
| `_shortTipWaitTime` | `float` | Short display duration between tips |
| `_longTipWaitTime` | `float` | Long display duration between tips |
| `_playerLevel` | `int` | Player level (used to filter tip band) |
| `_firstLoad` | `int` | First-ever load flag |

**Behavior:**
- `StartTipsRotation()` → `RotateToNewTip(float timeToWait)` coroutine
- Waits `timeToWait` → `TipFadeOut()` → `SetRandomTipLocalisedText()` → `TipFadeIn()` → loops
- `SetRandomTipLocalisedText()` tries `_loadingTipBandKey` (level-banded), falls back to `_loadingTipBackupKey`
- Registered with `SectionManager.OnLeaveSection` — does **not** block (returns `CheckFeedback.Ok`)

### 4.4 `BlockingSpinner_Canvas` — Reusable Spinner Widget (from `shared_ui` bundle)

Embedded inside popups (e.g., `RelocationPopUp_Canvas`, `GenericInfoPopupWithHyperlinks_Canvas`) — not the full-screen transition.

| Element | Anchor | Position | Size | Description |
|---------|--------|----------|------|-------------|
| **BlockingSpinner_Canvas** | `bottom-left` | `(0, 0)` | `(0, 0)` | Root |
| **FullscreenInputBlocker** | `full-screen` | `(0, 0)` | `(120, 120)` | Blocks all input behind spinner |
| **Spinner_ClockWise** | `center` | `(0, 0)` | `(128, 128)` | Clockwise spinner |
| **Spinner_AntiClockWise** | `center` | `(0, 0)` | `(118, 118)` | Counter-clockwise spinner |

### 4.5 `LoadingContainer` — Embedded Mini-Spinner (from `shared_ui` bundle)

Small `25x25` to `75x75` spinner reused inside buttons, popups, and panels across the game.

| Element | Anchor | Position | Size | Notes |
|---------|--------|----------|------|-------|
| **LoadingContainer** | `full-screen` | `(0, 0)` | `(0, 0)` | Root |
| **Clockwise** | `center` | `(0, 0)` | `25-75` | Size varies by parent |
| **AntiClockwise** / **Anticlockwise** | `center` | `(0, 0)` | `33-100` | Size varies by parent |
| **LoadingBG** (some variants) | `full-screen` | `(0, 0)` | `(0, 0)` | Background |

### 4.6 Screen Layout Visualization

```
┌─────────────────────────────────────────────────────────────┐
│  [BG] — full-screen blurred snapshot / static image          │
│  [InputBlocker] — invisible full-screen touch blocker        │
│                                                             │
│                    [Faction Logo]                            │
│                 LogoContainer (center, 586x248)              │
│                                                             │
│                                                             │
│              [LoadingScreenText]                             │
│            "Loading..." (center, -234.7y, 1334x119)          │
│                                                             │
│                                                             │
│              ╔═══════════════════════════╗                  │
│              ║  "Destroy enemy ships...  ║                  │
│              ║   to earn Battle Pass     ║                  │
│              ║   points."                ║                  │
│              ╚═══════════════════════════╝                  │
│           LoadingTipsContainer (center, -180, 1024x100)       │
│                                                             │
│  [Numbers_BottomLeft]         [StatusContainer]              │
│   "45%"                       [DownloadingText]             │
│   (300, 32.6)                 [MB_Progress]                 │
│                                                             │
│  [IndividualProgressContainer] — Bundle scroller             │
│   (680, 750) — 1306 x 704                                  │
│   • BundleName: 45%                                         │
│   • BundleName: 23%                                         │
│                                                             │
│  [LoadingIndicatorContainer]          [Button]               │
│   Spinner (59x50)                    Cancel/Retry            │
│   (bottom-right)                   (bottom-right)           │
└─────────────────────────────────────────────────────────────┘
```

### 4.7 Text Content Mapping

| Phase | Typical Text |
|-------|--------------|
| `_01_PrepareSectionActivation` | "Preparing…" |
| `_02_CheckIfAllReadyForActivation` | "Loading…" |
| `_03_ActivateSection` | "Almost ready…" |

| Messaging Type | Text |
|----------------|------|
| `Default = 0` | Normal localized progress |
| `DownloadError = 1` | "Download failed. Retrying…" |

### 4.8 `LoadingTipText` — Source and Custom Tip Injection

#### 4.8.1 Current Source of Loading Tip Text

The tip text is **not hardcoded** — it is resolved at runtime from the game's **localization tables** via `TextLocalizer` (`_loadingScreenTipTextLocalizer`, offset `0x28`).

**Flow inside `SetRandomTipLocalisedText()`:**

```
1. TrySetBandedLoadingTips(FeaturesConfig)
   → Builds a locale key from _loadingTipBandKey + _playerLevel
   → E.g., "loading_tips_band_3" → random entry from that band's locale table
   → Uses LoadingScreenTipsKey = "loading_screen_tips_key" to locate the band table

2. If banded lookup fails → fallback to _loadingTipBackupKey
   → _loadingScreenTipTextLocalizer.SetContext(new LocaleTextContext(backupKey, category))
   → TextLocalizer resolves the key against the locale table
```

**Key Constants:**

| Constant | Value | Purpose |
|----------|-------|---------|
| `LoadingScreenTipsKey` | `"loading_screen_tips_key"` | Locates the banded tip table in locale data |
| `PlayerLevelKey` | `"player_level"` | Context key for player level when filtering bands |
| `FirstLoadKey` | `"first_load"` | Context key for first-load flag |

**Important:** `_loadingTipBandKey` and `_loadingTipBackupKey` are **serialized `[SerializeField]` strings** set in the Unity Editor on the prefab. They are **not** visible in the decompiled C# code — you'd need to inspect the prefab in Unity or read the MonoBehaviour serialized data from `sharedassets0.assets` to get their exact values.

The locale tables themselves are typically **server-side data** loaded at startup (or baked into asset bundles as ScriptableObjects / JSON). This means you cannot simply edit a text file to change tips.

---

#### 4.8.2 How to Add Custom Tip Text — Client-Side Patches

Since the locale tables are server-side / inaccessible, the practical approach is to **hook `SetRandomTipLocalisedText()`** and inject your own text after normal selection.

**Option A: Direct `_textMeshPro.text` Override (Recommended)**

Hook `SetRandomTipLocalisedText()` post-call and occasionally replace the text:

```csharp
// Hook: Prime.LoadingScreen.LoadingTipViewController.SetRandomTipLocalisedText()
private void Hooked_SetRandomTipLocalisedText(LoadingTipViewController __instance)
{
    // Call original first
    Original_SetRandomTipLocalisedText(__instance);

    // 20% chance to inject custom tip
    if (UnityEngine.Random.value < 0.2f)
    {
        // _textMeshPro is at field offset 0x20
        var textMeshPro = __instance.GetField<TextMeshProUGUI>(0x20);
        textMeshPro.text = "Your custom loading tip here!";
        textMeshPro.color = Color.cyan; // optional styling
    }
}
```

**Why this works:**
- `_textMeshPro` is a writable `TextMeshProUGUI` (offset `0x20`)
- `RotateToNewTip()` calls `TipFadeIn()` after `SetRandomTipLocalisedText()`, so injected text fades in normally
- The animator fade-out operates on the component, not the string content

**Option B: Append to Rotation Cycle**

Replace or wrap the `RotateToNewTip()` coroutine to insert custom tips on specific cycles (e.g., every 5th tip).

**Option C: `TextLocalizer.OverrideLocalizedText()`**

```csharp
var localizer = __instance.GetField<TextLocalizer>(0x28); // _loadingScreenTipTextLocalizer
localizer.OverrideLocalizedText("Your custom tip");
```

**Caveat:** `SetRandomTipLocalisedText()` likely calls `SetContext()` which may clear the override. Test persistence across cycles.

**Option D: Modify Prefab Serialized Fields**

Requires asset bundle rebuilding:
1. Open `sharedassets0.assets` → `TransitionScreen_Canvas` → `LoadingTipsContainer`
2. Change `_loadingTipBackupKey` to a new locale key
3. Add that key to the game's locale tables
4. Rebuild the bundle

---

#### 4.8.3 Comparison of Approaches

| Approach | Difficulty | Persistence | Risk |
|----------|-----------|-------------|------|
| **A** — Direct `_textMeshPro.text` override | Easy | Per-cycle (re-rolls each time) | Low |
| **B** — Custom coroutine wrapper | Medium | Full control | Medium |
| **C** — `OverrideLocalizedText()` | Easy | May be cleared by `SetContext` | Low |
| **D** — Asset bundle mod | Hard | Permanent | High (anti-cheat) |

**Recommended:** **Option A** — one post-call hook on a single private method, directly writes `TMP_Text`, and your custom tip fades in/out naturally with the existing animator.

**Exact Hook Location:**
```@/output/decompiled/.../Prime.LoadingScreen.LoadingTipViewController.c
private void SetRandomTipLocalisedText()
```

Hook with your IL2CPP patching framework (MelonLoader, BepInEx) or runtime method replacement using the Il2CppInspector function pointer.

---

## 5. Hang Analysis — Why the Transition Screen Gets Stuck

### 5.1 Category A: Section Change Callback Stall

**Mechanism:** `ChangeSectionCoroutine()` calls delegates and checks `CheckFeedback`. If **any** callback returns `NotOk`, the coroutine yields `WaitForSeconds(_recheckDelayInSec)` and retries the same phase **indefinitely**.

**Stall scenarios:**
1. **Server response stall:** A director's `OnSectionEnterPrepareActivate()` waits for alliance data / player profile and returns `NotOk` every frame.
2. **Asset bundle preload stall:** `SceneAssetPreloadSectionDirector` waits for `_sceneAsset` bundle that never downloads.
3. **Director deadlock:** Director A won't leave until Director B is ready, but B can't become ready until A leaves.
4. **TransitionManager itself stalls:** If `TransitionManager.OnEnterSection()` or `OnLeaveSection()` returns `NotOk` because its internal state is broken, the entire section change blocks.

**Detection:** `SectionChangeInstrumentation.CurrentPhaseBlockingCallbacks` tracks exactly which delegates are returning `NotOk`.

---

### 5.2 Category B: TransitionManager State Machine Deadlock

**States:** `Showing` (0) → `Shown` (1) → `Hidden` (2)

**Stall `Showing → Shown`:**
- `SetLoadingScreen()` sets `Showing` and starts `Load()` coroutine.
- `Load()` waits for:
  - `SceneTransitionManager._sceneLoadActionInProgress == false`
  - `TransitionViewController._isDownloading == false`
  - `SectionDirectorBase._myState` is `PreLoaded` or `Active`
- **If any condition never becomes true, `Load()` never finishes → `Showing` forever.**

**Stall `Shown → Hidden`:**
- `Hide()` calls `CanHide()` first.
- `CanHide()` returns false if:
  - `_currentState != Shown`
  - `BlurController.IsTweeningBlur()` is true
  - Some internal resource is still pending
- **If `CanHide()` is permanently false, the loading screen is never dismissed.**

---

### 5.3 Category C: BlurController Corruption

**Mechanism:** `CommandBuffer` blur tween driven by `TickBlurTransition()` every frame.

**Stall conditions:**
1. **`RemoveBlur()` never starts:** `Hide()` is never called.
2. **`RemoveBlur()` waits forever:** `_waitFrames` delay. If game is paused / 0 FPS, it never advances.
3. **`TickBlurTransition()` stops being called:** `MonoBehaviour` destroyed or `Update()` disabled.
4. **`_blurTweenTarget` mismatch:** Code sets target to `1` but never resets to `0`.
5. **CommandBuffer leak:** Camera destroyed and recreated → old `CommandBuffer` orphaned, blur persists.

**Emergency fix:** `BlurController.ForceCompletion()` snaps value to target, but only helps if tween logic is the blocker.

---

### 5.4 Category D: Asset Bundle Download Hang

**Mechanism:** `TransitionViewController` tracks `_inProgress` bundles.

**Stall conditions:**
1. **Download never starts:** Download scheduler is deadlocked.
2. **Download never completes:** Network down, invalid CDN URL (404), corrupt bundle retry loop, or event callback lost.
3. **`_isDownloading` stuck true:** Race condition leaves flag true even after all downloads complete. Animator stays in `IsDownloading` state.
4. **Progress math NaN:** If `_totalDownloadingAmmout` is 0, progress calculations produce NaN, breaking comparison logic.

---

### 5.5 Category E: Scene Loading Hang

**Mechanism:** `SceneTransitionManager` uses a NodeCanvas `BehaviourTree` to drive scene loads.

**Stall conditions:**
1. **BT infinite loop:** A node waits for a condition that never becomes true.
2. **Additive scene unload deadlock:** `QueueUpAdditiveSceneUnloads()` queues unloads, but `DontDestroyOnLoad` scenes block the queue.
3. **`_sceneLoadActionInProgress` stuck true:** Unity's `OnSceneLoaded` callback never fires (leaked operation, null callback).
4. **Request queue overflow:** Rapid section changes queue up. If one stalls, all subsequent changes block.

---

### 5.6 Category F: CanvasController Null Reference

**Mechanism:** `TransitionManager.UpdateMessagingType()` guard:

```csharp
if (_canvasController != null) {
    // Update messaging
} else {
    // il2cpp_codegen_object_new() — no-op
}
```

**If `_canvasController` is null:** The loading screen prefab was never instantiated or was destroyed. The loading screen is "shown" logically but no UI exists.

**Why null?**
- `_loadingScreenPrefab` failed to instantiate (missing prefab)
- Prefab lacks `CanvasController` component
- Memory reclaim destroyed the prefab while transition logic still references it
- `OnDestroy()` called prematurely

---

### 5.7 Category G: Section Director Load Failure

**Mechanism:** `MAX_LOAD_RETRIES = 3`. After 3 failures:
- `_sectionLoadFailed = true`
- `HandleSectionLoadFailure()` called
- `_errorMessageBoxContext` shown

**Stall conditions:**
1. **Error message box hidden:** Message box UI is broken → infinite loading screen with no error popup.
2. **Retry loop:** `HandleSectionLoadFailure()` resets retry count and retries, but underlying issue (missing bundle) persists → infinite loop.
3. **State corruption:** After failure, `_myState` set to `Idle` instead of `NotLoaded`. Section manager thinks it's ready when it's broken.

---

### 5.8 Category H: Memory Reclaim Deadlock

**Mechanism:** Low-memory devices trigger `SectionDirectorMemoryManager` to reclaim sections.

**Stall conditions:**
1. **Memory manager reclaims the section being entered:** The target section's prefab is destroyed while `ChangeSectionCoroutine()` is trying to activate it.
2. **Memory manager reclaims TransitionManager's prefab:** The loading screen canvas is destroyed mid-transition.
3. **Race between reclaim and preload:** `PreInstantiate()` is running while memory reclaim is dropping assets. The director ends up in `NotLoaded` state with no assets.

---

### 5.9 Category I: TransitionViewController Animator Stall

**Mechanism:** `Animator` drives Show / Hide transitions.

**Stall conditions:**
1. **Animator missing parameter:** Code sets `IsDownloadingHash` but the AnimatorController doesn't have that parameter → no state change.
2. **Animator transition has no exit time:** A transition loops back to itself with no exit condition.
3. **`_isHiding` stuck true:** Code sets `_isHiding = true` but the animator's Hide state never fires its completion event.
4. **Individual show timeout ignored:** `_individualShowTime` exceeds `_maxIndividualShowTime`, but the update loop doesn't process the timeout because `_showingIndividual` is false.

---

### 5.10 Category J: Input / Frame Rate Stall

**Mechanism:** Multiple coroutines rely on `Update()` or `WaitForEndOfFrame()`.

**Stall conditions:**
1. **Game paused with `Time.timeScale = 0`:** Coroutines using `WaitForSeconds()` never advance. The transition uses `unscaledDeltaTime` in some places but not all.
2. **Frame rate drops to 0:** `_waitFrames` countdown never decrements.
3. **Application focus lost:** `OnApplicationFocus(false)` may pause downloads or scene loads.

---

## 6. Recovery Mechanisms & Timeouts

### 6.1 Existing Safeguards

| Mechanism | Location | Behavior |
|-----------|----------|----------|
| `SECTION_CHANGE_TIMEOUT` | `SectionChangeInstrumentation` | Traces timeout event after threshold seconds |
| `_sectionChangeTimeoutEventFired` | `SectionChangeInstrumentation` | Prevents duplicate timeout events |
| `MAX_LOAD_RETRIES = 3` | `SectionDirectorBase` | After 3 failures, shows error popup |
| `BlurController.ForceCompletion()` | `BlurController` | Emergency snap-to-target |
| `_recheckDelayInSec` | `SectionManager` | Prevents tight-loop CPU burn when stalled |
| `CancelAllSectionChanges()` | `SectionManager` | External cancel API |

### 6.2 What the Timeout Does (and Doesn't) Do

`SectionChangeInstrumentation.OnSectionChangeTimeoutEvent` fires diagnostics/logging, but **it does NOT force-resolve the stall**. The coroutine is still blocked on `NotOk`. The timeout is observability-only unless another system listens to the event and forces a state change.

---

## 7. Suggested Fixes / Workarounds

## 7. Concrete Hook Points — How to Patch Each Fix

These are the **exact classes, methods, and field offsets** (from Redux/Il2Cpp) where you should insert the suggested workarounds. All offsets assume the base image address is already subtracted (Redux computes `offset = addr_val - base`).

---

### 7.1 Fix A: Section Change Callback Stall

**Hook 1 — `SectionManager.ChangeSectionCoroutine()`**

**File:** `Digit/Client/Sections/SectionManager.cs`  
**Target:** The `MoveNext()` of `<ChangeSectionCoroutine>d__73` (the coroutine state machine)  
**Patch:** After the delegate invocation loop that checks `CheckFeedback`, add a per-phase timer.

```csharp
// In the MoveNext() of <ChangeSectionCoroutine>d__73:
// After collecting all feedback values:
private bool MoveNext()
{
    // ... existing phase logic ...

    // ADD THIS:
    float phaseStartTime = Time.unscaledTime;   // track when we entered this phase
    while (true)
    {
        CheckFeedback feedback = InvokeCallbacksAndCollectFeedback();
        if (feedback == CheckFeedback.Ok || feedback == CheckFeedback.SectionDropped)
            break;

        // NEW: global timeout per phase
        if (Time.unscaledTime - phaseStartTime > 30.0f)
        {
            LogStalledCallbacks();   // log which delegates returned NotOk
            ForceFeedbackToOk();     // force continue with warning
            break;
        }

        yield return new WaitForSeconds(_recheckDelayInSec);
    }
    // ... continue to next phase ...
}
```

**Hook 2 — `SectionManager.OnEnterSection` / `OnLeaveSection` delegate invocation**

**File:** Same coroutine  
**Target:** The loop that iterates `Delegate[] callbacks` and calls `Invoke()`  
**Patch:** Wrap each callback in a timed wrapper:

```csharp
// Before invoking each callback:
float callbackStart = Time.unscaledTime;
var result = callback.Invoke(status, phase, storage);
if (Time.unscaledTime - callbackStart > 5.0f)
{
    LogSlowCallback(callback.Target, callback.Method, Time.unscaledTime - callbackStart);
}
```

**Hook 3 — `TransitionManager.OnEnterSection()` / `OnLeaveSection()`**

**File:** `Digit/Prime/LoadingScreen/TransitionManager.cs`  
**Target:** `OnEnterSection(SectionStatus, SectionEnterPhases, SectionStorage)` and `OnLeaveSection(...)`  
**Patch:** Add a safety return:

```csharp
private CheckFeedback OnEnterSection(SectionStatus status, SectionEnterPhases phase, SectionStorage storage)
{
    // If we've been in Showing state for > 60s, force Ok to unblock
    if (_currentState == State.Showing && _loadStartTime > 0 && Time.unscaledTime - _loadStartTime > 60f)
    {
        LogWarning("TransitionManager.OnEnterSection forcing Ok after 60s stall");
        return CheckFeedback.Ok;
    }

    // ... existing logic ...
}
```

**Required field addition:**
- Add `private float _loadStartTime;` to `TransitionManager` (set it in `SetLoadingScreen()`).

---

### 7.2 Fix B: TransitionManager State Deadlock

**Hook 1 — `TransitionManager.Update()`**

**File:** `Digit/Prime/LoadingScreen/TransitionManager.cs`  
**Target:** `Update()` method (called every frame)  
**Patch:** Add emergency hide watchdog:

```csharp
private void Update()
{
    // ... existing blur tick etc. ...

    // NEW: emergency hide watchdog
    if (_currentState == State.Shown && _canvasController != null)
    {
        float shownDuration = Time.unscaledTime - _shownTimestamp;
        if (shownDuration > 45.0f)
        {
            LogWarning($"TransitionManager emergency hide after {shownDuration}s");
            Hide(/*current status*/);
        }
    }
}
```

**Required field addition:**
- Add `private float _shownTimestamp;` (set in `Load()` when state transitions to `Shown`).

**Hook 2 — `TransitionManager.CanHide()`**

**File:** Same  
**Target:** `CanHide()` method  
**Patch:** Add an override:

```csharp
private bool CanHide()
{
    // If blur has been at full strength for > 30s, allow hide anyway
    if (_currentState == State.Shown && BlurController != null && BlurController.IsTweeningBlur())
    {
        if (Time.unscaledTime - _blurFullTimestamp > 30f)
        {
            LogWarning("CanHide() forcing true — blur has been stuck at full for 30s");
            BlurController.ForceCompletion();
            return true;
        }
    }

    // ... existing logic ...
}
```

**Required field addition:**
- Add `private float _blurFullTimestamp;` (set in `SetLoadingScreen()` or when `SetBlur()` is called).

---

### 7.3 Fix C: BlurController Corruption

**Hook 1 — `TransitionManager.OnDestroy()`**

**File:** `Digit/Prime/LoadingScreen/TransitionManager.cs`  
**Target:** `OnDestroy()` method  
**Patch:** Always dispose the blur:

```csharp
private void OnDestroy()
{
    // NEW: emergency cleanup
    if (BlurController != null)
    {
        BlurController.ForceCompletion();
        BlurController.Dispose();
    }

    // ... existing cleanup ...
}
```

**Hook 2 — `BlurController.TickBlurTransition()`**

**File:** `Digit/Prime/LoadingScreen/BlurController.cs`  
**Target:** `TickBlurTransition()`  
**Patch:** Add a max-tween safeguard:

```csharp
public void TickBlurTransition()
{
    // ... existing lerp logic ...

    // NEW: if tween target is unreachable for > 10s, force snap
    if (Mathf.Abs(_blurTweenTarget - _blurTweenValue) > 0.001f)
    {
        if (_tweenStuckTimer <= 0f)
            _tweenStuckTimer = Time.unscaledTime;
        else if (Time.unscaledTime - _tweenStuckTimer > 10f)
        {
            LogWarning("Blur tween stuck — forcing completion");
            ForceCompletion();
            _tweenStuckTimer = 0f;
        }
    }
    else
    {
        _tweenStuckTimer = 0f;
    }
}
```

**Required field addition:**
- Add `private float _tweenStuckTimer;` to `BlurController`.

**Hook 3 — `BlurController.RemoveBlur()` coroutine**

**File:** `Digit/Prime/LoadingScreen/BlurController.cs`  
**Target:** `MoveNext()` of `<RemoveBlur>d__14`  
**Patch:** Cap the `_waitFrames` wait:

```csharp
private bool MoveNext()
{
    // ... existing state machine ...

    // In the wait-frames state:
    int waitedFrames = 0;
    while (waitedFrames < _waitFrames)
    {
        waitedFrames++;
        yield return null;

        // NEW: cap wait at 300 frames (~5s @ 60fps)
        if (waitedFrames > 300)
        {
            LogWarning("RemoveBlur waitFrames capped at 300");
            break;
        }
    }

    // ... continue to tween out ...
}
```

---

### 7.4 Fix D: Asset Bundle Download Hang

**Hook 1 — `TransitionViewController.Update()`**

**File:** `Digit/Prime/LoadingScreen/TransitionViewController.cs`  
**Target:** `Update()` method  
**Patch:** Add download stall watchdog:

```csharp
private void Update()
{
    // ... existing update logic ...

    // NEW: download stall detection
    if (_isDownloading)
    {
        if (_downloadStallTimer <= 0f)
            _downloadStallTimer = Time.unscaledTime;
        else if (Time.unscaledTime - _downloadStallTimer > 30f)
        {
            LogWarning("Download stall detected — forcing _isDownloading = false");
            _isDownloading = false;
            _animator.SetBool(AnimVars.IsDownloadingHash, false);

            // Switch to error text
            if (_stateLocalizer != null && _downloadErrorProgressContext != null)
                _stateLocalizer.SetContext(_downloadErrorProgressContext);
        }

        // Reset stall timer if bytes actually changed
        if (_bytesDownloaded > _lastBytesDownloaded)
        {
            _downloadStallTimer = 0f;
            _lastBytesDownloaded = _bytesDownloaded;
        }
    }
    else
    {
        _downloadStallTimer = 0f;
    }
}
```

**Required field additions:**
- `private float _downloadStallTimer;`
- `private float _lastBytesDownloaded;`

**Hook 2 — `DidAssetBundleDownloadCompleteEvent()`**

**File:** `Digit/Prime/LoadingScreen/TransitionViewController.cs`  
**Target:** `DidAssetBundleDownloadCompleteEvent(ushort ID, string error)`  
**Patch:** If `error != null`, don't leave the bundle in `_inProgress`:

```csharp
private void DidAssetBundleDownloadCompleteEvent(ushort ID, string error)
{
    // ... existing logic ...

    // NEW: if error, remove from _inProgress immediately
    if (!string.IsNullOrEmpty(error))
    {
        _inProgress.Remove(ID);
        _downloadingBundleContexts.Remove(/* matching context */);
        UpdateDisplayList(_inProgress);
    }

    // If _inProgress is now empty, force _isDownloading = false
    if (_inProgress.Count == 0 && _isDownloading)
    {
        _isDownloading = false;
        _animator.SetBool(AnimVars.IsDownloadingHash, false);
    }
}
```

---

### 7.5 Fix E: Scene Loading Hang

**Hook 1 — `SceneTransitionManager.Update()`**

**File:** `Digit/Client/SceneManagement/SceneTransitionManager.cs`  
**Target:** `Update()` or the BehaviourTree tick loop  
**Patch:** Add scene load watchdog:

```csharp
private void Update()
{
    // ... existing BT tick ...

    // NEW: scene load watchdog
    if (_sceneLoadActionInProgress && _currentReq != null)
    {
        if (_sceneLoadStartTime <= 0f)
            _sceneLoadStartTime = Time.unscaledTime;
        else if (Time.unscaledTime - _sceneLoadStartTime > 60f)
        {
            LogError("Scene load watchdog fired — forcing _sceneLoadActionInProgress = false");
            _sceneLoadActionInProgress = false;
            _currentReq = null;

            // Show error dialog
            if (_errorMessageBoxContext != null)
                ShowErrorPopup(_errorMessageBoxContext);
        }
    }
    else
    {
        _sceneLoadStartTime = 0f;
    }
}
```

**Required field addition:**
- `private float _sceneLoadStartTime;`

**Hook 2 — `SceneTransitionManager.OnSceneLoaded()`**

**File:** Same  
**Target:** `OnSceneLoaded(Scene scene, LoadSceneMode mode)`  
**Patch:** Ensure `_sceneLoadActionInProgress` is always cleared, even on error:

```csharp
private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
{
    // ... existing logic ...

    // NEW: defensive reset
    _sceneLoadActionInProgress = false;
    _sceneLoadStartTime = 0f;
}
```

---

### 7.6 Fix F: CanvasController Null Reference

**Hook — `TransitionManager.SetLoadingScreen()`**

**File:** `Digit/Prime/LoadingScreen/TransitionManager.cs`  
**Target:** `SetLoadingScreen(...)`  
**Patch:** Guard instantiation:

```csharp
public void SetLoadingScreen(SectionStatus status, TransitionType type, TransitionMessagingType messagingType = 0)
{
    // ... existing logic ...

    // NEW: defensive null check after instantiation
    if (_canvasController == null)
    {
        LogError("SetLoadingScreen: _canvasController is null after prefab instantiation!");

        // Attempt fallback — create a blank canvas
        GameObject go = new GameObject("FallbackLoadingCanvas");
        _canvasController = go.AddComponent<CanvasController>();
    }

    // ... rest of setup ...
}
```

**Hook 2 — `TransitionManager.UpdateMessagingType()`**

**File:** Same  
**Target:** `UpdateMessagingType(TransitionMessagingType)`  
**Patch:** Early return with fallback:

```csharp
public void UpdateMessagingType(TransitionMessagingType messagingType)
{
    if (_canvasController == null)
    {
        LogWarning("UpdateMessagingType: _canvasController is null — skipping UI update");
        return;
    }

    // ... existing logic ...
}
```

---

### 7.7 Fix G: Section Director Load Failure

**Hook — `SectionDirectorBase.HandleSectionLoadFailure()`**

**File:** `Digit/Client/Sections/SectionDirectorBase.cs`  
**Target:** `HandleSectionLoadFailure(SectionStatus, SectionStorage)`  
**Patch:** Cap retries and ensure state is valid:

```csharp
protected override void HandleSectionLoadFailure(SectionStatus status, SectionStorage storage)
{
    _sectionLoadFailed = true;

    // NEW: cap retries — do not retry again
    _currentLoadRetryCount = MAX_LOAD_RETRIES + 1;

    // Show error dialog
    if (_errorMessageBoxContext != null)
        ShowErrorPopup(_errorMessageBoxContext);

    // NEW: force state to NotLoaded so SectionManager can retry from clean slate
    set_MyState(State.NotLoaded);
}
```

**Hook 2 — `SectionDirectorBase.OnSectionEnterPrepareActivate()`**

**File:** Same  
**Target:** `OnSectionEnterPrepareActivate(...)` in derived directors  
**Patch:** In every derived director that does network calls, add a hard timeout:

```csharp
// Example pattern for any SectionDirectorBase subclass:
protected override bool OnSectionEnterPrepareActivate(SectionStatus status, SectionStorage storage, out CheckFeedback result)
{
    if (_prepareStartTime <= 0f)
        _prepareStartTime = Time.unscaledTime;

    if (Time.unscaledTime - _prepareStartTime > 20f)
    {
        LogError("OnSectionEnterPrepareActivate timeout — forcing Ok with degraded state");
        result = CheckFeedback.Ok;
        return true;
    }

    // ... existing async preparation logic ...
}
```

---

### 7.8 Fix H: Memory Reclaim Deadlock

**Hook — `SectionDirectorMemoryManager` (any reclaim method)**

**File:** `Digit/Client/Sections/SectionDirectorMemoryManager.cs`  
**Target:** The method that calls `DropSection()` or `Unload()` on directors  
**Patch:** Skip reclaim if a section change is in progress:

```csharp
// Before reclaiming a director:
if (SectionManager.Instance != null && SectionManager.Instance.IsSectionChangeInProgress)
{
    LogInfo("Skipping memory reclaim — section change in progress");
    return;
}

// ... existing reclaim logic ...
```

**Hook 2 — `TransitionManager.OnEnable()` / `OnDisable()`**

**File:** `Digit/Prime/LoadingScreen/TransitionManager.cs`  
**Target:** `OnEnable()` and `OnDisable()`  
**Patch:** Prevent reclaim from destroying the transition canvas during active transition:

```csharp
private void OnDisable()
{
    // NEW: if transition is active, abort the disable
    if (_currentState == State.Showing || _currentState == State.Shown)
    {
        LogWarning("TransitionManager.OnDisable blocked — transition is active");
        return;
    }

    // ... existing logic ...
}
```

---

### 7.9 Fix I: TransitionViewController Animator Stall

**Hook — `TransitionViewController.Update()`**

**File:** `Digit/Prime/LoadingScreen/TransitionViewController.cs`  
**Target:** `Update()` method  
**Patch:** Add animator timeout:

```csharp
private void Update()
{
    // ... existing progress / download logic ...

    // NEW: animator state watchdog
    AnimatorStateInfo stateInfo = _animator.GetCurrentAnimatorStateInfo(0);
    if (stateInfo.IsName("Show") || stateInfo.IsName("Hide"))
    {
        if (_animatorStateTimer <= 0f)
            _animatorStateTimer = Time.unscaledTime;
        else if (Time.unscaledTime - _animatorStateTimer > 15f)
        {
            LogWarning($"Animator stuck in {stateInfo.fullPathHash} for 15s — forcing crossfade");
            _animator.CrossFade("Hidden", 0.1f);
            _animatorStateTimer = 0f;
        }
    }
    else
    {
        _animatorStateTimer = 0f;
    }
}
```

**Required field addition:**
- `private float _animatorStateTimer;`

---

### 7.10 Fix J: Input / Frame Rate Stall

**Hook — `SectionManager.ChangeSectionCoroutine()`**

**File:** `Digit/Client/Sections/SectionManager.cs`  
**Target:** The `WaitForSeconds` yield  
**Patch:** Replace with `WaitForSecondsRealtime` or a frame-based fallback:

```csharp
// Instead of:
yield return new WaitForSeconds(_recheckDelayInSec);

// Use:
yield return new WaitForSecondsRealtime(_recheckDelayInSec);
// OR:
float waitUntil = Time.unscaledTime + _recheckDelayInSec;
while (Time.unscaledTime < waitUntil)
    yield return null;
```

**Hook 2 — `TransitionManager.Load()` coroutine**

**File:** `Digit/Prime/LoadingScreen/TransitionManager.cs`  
**Target:** `MoveNext()` of `<Load>d__23`  
**Patch:** Replace time-dependent waits with `WaitForSecondsRealtime`:

```csharp
// In Load() coroutine:
// Replace any WaitForSeconds with WaitForSecondsRealtime
// so it advances even when Time.timeScale = 0
```

**Hook 3 — `BlurController.RemoveBlur()`**

**File:** `Digit/Prime/LoadingScreen/BlurController.cs`  
**Target:** `MoveNext()` of `<RemoveBlur>d__14`  
**Patch:** The frame wait should use a real-time frame counter:

```csharp
// Replace:
for (int i = 0; i < _waitFrames; i++)
    yield return null;

// With:
int frameCounter = 0;
while (frameCounter < _waitFrames)
{
    frameCounter++;
    yield return null;
    // This naturally advances even at 1 FPS, but cap at 300 frames (see Fix C)
}
```

---

## 8. Quick Reference — Which Hook Solves Which Hang

| Hang Symptom | Category | Primary Hook | Secondary Hook |
|--------------|----------|------------|----------------|
| Loading screen forever, no progress change | A | `SectionManager.ChangeSectionCoroutine()` global timeout | `TransitionManager.OnEnterSection()` force Ok |
| Blur visible but screen stuck, no animation | C | `BlurController.TickBlurTransition()` tween watchdog | `TransitionManager.OnDestroy()` ForceCompletion+Dispose |
| Download % stuck, scroller frozen | D | `TransitionViewController.Update()` download stall | `DidAssetBundleDownloadCompleteEvent()` error cleanup |
| Scene never loads, black screen | E | `SceneTransitionManager.Update()` scene load watchdog | `OnSceneLoaded()` defensive reset |
| Loading screen UI missing entirely | F | `TransitionManager.SetLoadingScreen()` null guard | `UpdateMessagingType()` early return |
| Error popup hidden, infinite retry | G | `HandleSectionLoadFailure()` state reset | Director `OnSectionEnterPrepareActivate()` timeout |
| Loading screen disappears mid-transition | H | `SectionDirectorMemoryManager` reclaim skip | `TransitionManager.OnDisable()` active-block |
| Animator stuck in Show/Hide | I | `TransitionViewController.Update()` animator watchdog | — |
| Game paused / 0 FPS during load | J | `WaitForSecondsRealtime` in all coroutines | `BlurController.RemoveBlur()` frame cap |

---

## 9. File Reference

### 9.1 Code Files

| File | Path | Role |
|------|------|------|
| `SectionManager.cs` | `Digit/Client/Sections/` | Central orchestrator |
| `SectionDirectorBase.cs` | `Digit/Client/Sections/` | Base for all sections |
| `SectionDirectorLoadAndShow.cs` | `Digit/Client/Sections/` | Load/show wrapper base |
| `SceneAssetPreloadSectionDirector.cs` | `Digit/Client/Sections/` | Scene bundle preloader |
| `SectionDirectorMemoryManager.cs` | `Digit/Client/Sections/` | Memory reclaim |
| `TransitionManager.cs` | `Digit/Prime/LoadingScreen/` | Loading screen + blur owner |
| `TransitionViewController.cs` | `Digit/Prime/LoadingScreen/` | UI layer (main loading screen) |
| `LoadingTipViewController.cs` | `Prime/LoadingScreen/` | Loading tips carousel |
| `BlurController.cs` | `Digit/Prime/LoadingScreen/` | Post-processing blur |
| `TransitionSetting.cs` | `Digit/Prime/LoadingScreen/` | ScriptableObject config |
| `TransitionType.cs` | `Digit/Prime/LoadingScreen/` | Transition reason enum |
| `TransitionMessagingType.cs` | `Digit/Prime/LoadingScreen/` | Text mode enum |
| `SceneTransitionManager.cs` | `Digit/Client/SceneManagement/` | Unity scene loading wrapper |
| `SectionChangeInstrumentation.cs` | `Prime/Diagnostics/Instrumentations/` | Timeout / diagnostics |

### 9.2 Asset / Prefab Files

| File | Path | Role |
|------|------|------|
| `sharedassets0.assets` | `assets/` (main game data) | Contains `TransitionScreen_Canvas` prefab (path_id=1346) |
| `level0` | `assets/` (main game data) | Contains `PreBundledLoading_Canvas` (path_id=98) |
| `shared_ui` | `Pre-Bundles/` | Contains `BlockingSpinner_Canvas` + `LoadingContainer` widgets |
| `loadingscreen/sprites` | `Pre-Bundles/` | Faction art texture atlas (~985KB) |
