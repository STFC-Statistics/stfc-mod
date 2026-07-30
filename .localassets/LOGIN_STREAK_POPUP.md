# Login Streak Startup Popup

## Overview

On game startup (once loading completes), the game can display a **Login Streak popup** — a 7-day daily reward tracker. This is the primary post-load popup in STFC.

---

## Trigger Mechanism

The popup is driven by a central lifecycle event:

```
LifecycleManagerEvents.GameSessionStartedEvent  (LifecycleManagerEvents.cs)
  └─► LoginStreakManager.OnGameSessionStartedEventHandler()
        └─► LoginStreakManager.TryShowStreakPopupForGameSessionStarted()
              └─► ShouldShowStreakPopup()  +  IsTodayStreakShown()
                    └─► ShowStreakPopup()
                          └─► LoginStreakPopupViewController (UI)
```

`GameSessionStartedEvent` fires when the game finishes loading and the session begins. 53 different managers subscribe to it; `LoginStreakManager` is the one that handles the popup.

**Relevant file:** `Assembly-CSharp/Digit/Prime/LoginStreak/LoginStreakManager.cs`

Key constants:
```csharp
private const int FullStreakDays = 7;
private const string LastShownStreakGameDayKey = "login_streak_popup_last_shown_streak_game_day";
```

---

## UI Structure

**File:** `Assembly-CSharp/Digit/Prime/LoginStreak/LoginStreakPopupViewController.cs`

```csharp
private Animator _animator;
private GenericButtonWidget _closeButton;
private LoginStreakDayWidget[] _dayWidgets;   // one per day (7 total)
private TimerWidget _timerWidget;
```

---

## How the "Already Shown" State Is Saved

Storage goes through three layers:

### 1. LoginStreakManager → PersistentPrefsManager

Reads/writes via key `"login_streak_popup_last_shown_streak_game_day"` through `PersistentPrefsManager`'s typed API (`GetInt`, `SetInt`, etc.).

**File:** `Assembly-CSharp/Digit/Prime/PersistentPrefs/PersistentPrefsManager.cs`

### 2. PersistentPrefsData — Protobuf local store

```csharp
private readonly RepeatedField preferences_;   // list of PersistentPref key-value entries
private const string _USER_PREFS_SEPARATOR = "~:::~";  // used for string-list values
```

`PersistentPrefsData` is a **Protobuf message** (`IMessage`, `IDeepCloneable`) containing a flat list of typed key-value pairs.

**File:** `Digit.Client.PrimeLib.Runtime/Digit/Prime/PersistentPrefs/PersistentPrefsData.cs`

### 3. Cloud sync (GameSparks)

```csharp
private void LoadPersistentPrefsFromCloud() { }
private void SavePersistentPrefsToCloud(...) { }
```

- Loaded fresh from cloud on every `GameSessionStarted`
- Saved back periodically via `_saveInterval` + `UpdateGateKeeper`
- One-time migration from Unity `PlayerPrefs` for legacy data (`MigrateIntFromPlayerPrefs`, etc.)

**Result:** The value survives device reinstalls — it is cloud-backed, not local-only.

---

## Other Startup-Capable Popups

These can also appear on or shortly after game load:

| Class | Description |
|---|---|
| `ReturnPlayerManager.ShowVideoPopup()` | Video popup for lapsed/returning players |
| `HudPromotionLoadAndShow.OnEnterSectionHandle()` | Promotional offer on HUD section enter |
| `HUDAllianceChestAndNewsLoadAndShow` | Alliance chest, meta event, PLC offer, free chest |

During the login sequence itself (before the game loads):

| Field in `LoginSequence` | Description |
|---|---|
| `_errorPopup` | Login error popup |
| `_tosPopup` | Terms of Service popup |
| `_gdprHandler` | GDPR consent popup |

---

## Blocking the Popup (Mod Implementation)

### Recommended: Hook `TryShowStreakPopupForGameSessionStarted`

This is the narrowest, most targeted hook — the exact `public` entry point called on session start. Returning `false` skips both `ShouldShowStreakPopup()` and `ShowStreakPopup()` with no side effects.

```cpp
// mods/src/patches/parts/login_streak.cc

static bool LoginStreakManager_TryShowStreakPopup_Hook(auto original, Il2CppObject* _this) {
    return false;
}

void InstallLoginStreakHooks() {
    static auto klass = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.LoginStreak", "LoginStreakManager");
    static auto method = klass.GetMethodInfo("TryShowStreakPopupForGameSessionStarted");
    SPUD_STATIC_DETOUR(method, LoginStreakManager_TryShowStreakPopup_Hook);
}
```

Wire up in `patches.cc` with a `bool suppressLoginStreakPopup` config toggle (see `Config` pattern in `config.h/.cc`).

### Alternative: Hook `ShowStreakPopup`

One level deeper — intercepts the actual show call regardless of caller. Useful if multiple code paths can trigger the popup. Downside: `private` method, slightly less stable across updates.

```cpp
static bool LoginStreakManager_ShowStreakPopup_Hook(auto original, Il2CppObject* _this) {
    return false;
}
```

### Avoid: Spoofing `IsTodayStreakShown` → `true`

Semantically incorrect — tells the game "already shown" rather than "don't show". Can affect pip notifications, cheat paths, and any other code that reads that state.

---

## HUD Promotional Elements on Startup

### What They Are

Two classes compose the promotional HUD bar that populates on game start:

| Class | Namespace | Elements |
|---|---|---|
| `HudPromotionLoadAndShow` | `Digit.Prime.HUD` | `_saleElement`, `_eventElement` — sale bundles and live events |
| `HUDAllianceChestAndNewsLoadAndShow` | `Digit.Prime.HUD` | `_chestElement`, `_metaEventElement`, `_plcOfferElement`, `_returnPlayerElement` |

Each `PromotionElement` has an `IsActive` property and a `State` (`PromotionState`). They are populated in `InitializeContext()` / `SetupContext()` when the HUD section is entered, and updated reactively as events/offers change.

`HudPromotionLoadAndShow` also has an `OnEnterSectionHandle()` method which can **auto-open the promotional popup** when the HUD section is first entered (i.e., on game start), before the player interacts with anything.

### Architecture

```
HUD section entered
  └─► HudPromotionLoadAndShow.OnEnterSectionHandle()   ← auto-popup gate
        └─► InitializeContext() / SetupContext()
              ├─► _saleElement.SetBundle(...)
              └─► _eventElement.SetTournament(...)

  └─► HUDAllianceChestAndNewsLoadAndShow.InitializeContext()
              ├─► SetFreeChestElement()
              ├─► SetMetaEventElement()
              ├─► SetPlcOfferElement()
              └─► SetReturnPlayerElement()
```

---

## Blocking the Promo Popup (Mod Implementation)

### Recommended: Hook `HudPromotionLoadAndShow.OnEnterSectionHandle`

This is the gate that decides whether the promotional popup **auto-opens** when the game loads into the HUD. Returning a failed `CheckFeedback` prevents it from opening automatically while leaving all other promo logic intact (the HUD bar buttons still work when clicked manually).

```cpp
// mods/src/patches/parts/promo_popup.cc

static CheckFeedback HudPromotionLoadAndShow_OnEnterSectionHandle_Hook(
    auto original, Il2CppObject* _this, Il2CppObject* status, int phase, Il2CppObject* storage) {
    // Return default (zero-initialised) CheckFeedback — treated as "blocked/no-op"
    return CheckFeedback{};
}

void InstallPromoPopupHooks() {
    static auto klass = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "HudPromotionLoadAndShow");
    static auto method = klass.GetMethodInfo("OnEnterSectionHandle");
    SPUD_STATIC_DETOUR(method, HudPromotionLoadAndShow_OnEnterSectionHandle_Hook);
}
```

### Alternative: Hook `InitializeContext` on Both Classes

This prevents the promo elements from being populated at all — the HUD bar slots stay empty and no auto-popup fires. More aggressive than the above (removes the HUD buttons entirely, not just the auto-popup).

```cpp
static void HudPromotionLoadAndShow_InitializeContext_Hook(auto original, Il2CppObject* _this) {
    // Skip — don't call original
}

static void HUDAllianceChestAndNewsLoadAndShow_InitializeContext_Hook(auto original, Il2CppObject* _this) {
    // Skip — don't call original
}

void InstallPromoHooks() {
    {
        static auto klass = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "HudPromotionLoadAndShow");
        static auto method = klass.GetMethodInfo("InitializeContext");
        SPUD_STATIC_DETOUR(method, HudPromotionLoadAndShow_InitializeContext_Hook);
    }
    {
        static auto klass = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.HUD", "HUDAllianceChestAndNewsLoadAndShow");
        static auto method = klass.GetMethodInfo("InitializeContext");
        SPUD_STATIC_DETOUR(method, HUDAllianceChestAndNewsLoadAndShow_InitializeContext_Hook);
    }
}
```

> **Note:** `InitializeContext` is `protected virtual` and inherited from `LoadAndShowUI`. Hooking it on the concrete subclass targets only that subclass, so no other UI is affected.

### Recommendation Summary

| Goal | Hook | Effect |
|---|---|---|
| Block only the auto-popup on start | `HudPromotionLoadAndShow.OnEnterSectionHandle` | Popup won't open automatically; HUD bar still works |
| Remove all promo HUD elements | `InitializeContext` on both classes | HUD promo slots stay empty; no popup, no buttons |

---

## HUD Elements Per View

### Architecture Overview

`HUDManager` manages all HUD elements via a bitmask system:

- **`HUDElements`** — enum of 18 named elements (each has an integer index 0–17)
- **`HUDElementsMask`** — bitmask (`1 << index`) that encodes which elements are visible
- **`HUDSettings`** — Unity `ScriptableObject` holding a `HUDElementsMask` + `HUDPanelsState` per view
- **`HUDSettingsHandler`** — holds one `HUDSettings` per view context, applies them via `SetHUDState()`
- **`PanelState`** — per-element state: `Ignore (-999)`, `Collapsed (-1)`, `Open (0)`, `Expanded (1)`

The actual bitmask values are configured in the Unity Editor (baked into asset bundles) and cannot be read from decompiled code. The table below reflects the structural intent from the code.

---

### The 18 HUD Elements

| Index | Key | Element | Notes |
|---|---|---|---|
| 0 | `player_profile` | **PlayerProfile** | Player avatar / level button |
| 1 | `chat` | **Chat** | Chat panel |
| 2 | `resources` | **Resources** | Resources bar (dilithium, tritanium, etc.) |
| 3 | `offers` | **Offers** | Promotional HUD bar (sale, events, chest) |
| 4 | `jobs` | **Jobs** | Job queue panel (build, research, refine) |
| 5 | `missions` | **Missions** | Missions button + notification |
| 6 | `navigation` | **Navigation** | Galactic navigation button |
| 7 | `station_status` | **StationStatus** | Station shield / warning indicator |
| 8 | `fleet_bar` | **FleetBar** | Fleet management bar (all docks) |
| 9 | `left_thumb` | **LeftThumb** | Left joystick-style action menu |
| 10 | `right_thumb` | **RightThumb** | Right joystick-style action menu |
| 11 | `alliance_help` | **AllianceHelp** | Alliance help request button |
| 12 | `loyalty` | **Loyalty** | Loyalty points display |
| 13 | `hud_frame` | **HudFrame** | Outer HUD chrome / frame |
| 14 | `wave_defense` | **WaveDefense** | Wave Defense panel |
| 15 | `locators_and_fleet_commander` | **LocatorsAndFleetCommander** | Home locator + Fleet Commander button |
| 16 | `arena_scoring` | **ArenaScoring** | Arena live scoring panel |
| 17 | `surge_scoring` | **SurgeScoring** | Surge live scoring panel |

> **Note:** `HUDPanelsState` also tracks a **`ServerClash`** slot (offset 0x38, between `HudFrame` and `WaveDefense`) that is not present in `HUDElements` — it appears as a panel-state-only entry, possibly a legacy or hidden element.

---

### View Contexts (from `HUDSettingsHandler`)

Each field in `HUDSettingsHandler` maps to a named `HUDSettings` ScriptableObject that is applied when that view is entered:

| View Context | Field | Active when |
|---|---|---|
| **Default** | `_defaultHudSettings` | Normal galaxy / system view |
| **Navigation** | `_navigationHudSettings` | Navigation mode is active |
| **Prescan** | `_prescanHudSettings` | Prescan panel is open (normal view) |
| **Starbase** | `_starbaseHudSettings` | Inside the player's starbase |
| **Battle — Default** | `_battleViewDefaultHudSettings` | Standard combat view |
| **Battle — Territory** | `_battleViewTerritoryHudSettings` | Territory combat |
| **Battle — Arena** | `_battleViewArenaHudSettings` | Arena combat |
| **Battle — Surge** | `_battleViewSurgeHudSettings` | Surge combat |
| **Battle — Wave Defense** | `_battleViewWaveDefenseHudSettings` | Wave Defense combat |
| **Battle Prescan — Default** | `_battleViewPrescanHudSettings` | Combat + prescan open |
| **Battle Prescan — Territory** | `_battleViewPrescanTerritoryHudSettings` | Territory combat + prescan |
| **Battle Prescan — Arena** | `_battleViewPrescanArenaHudSettings` | Arena combat + prescan |
| **Battle Prescan — Surge** | `_battleViewPrescanSurgeHudSettings` | Surge combat + prescan |
| **Wave Defense** | `_waveDefenseHudSettings` | Wave Defense mode (non-combat phase) |
| **Wave Defense + Prescan** | `_waveDefensePrescanOpenHudSettings` | Wave Defense + prescan open |
| **Arena** | `_arenaHudSettings` | Arena scoring view |
| **Arena + Prescan** | `_arenaPrescanHudSettings` | Arena + prescan open |
| **Surge** | `_surgeHudSettings` | Surge scoring view |
| **Surge + Prescan** | `_surgePrescanHudSettings` | Surge + prescan open |

---

### Logical Element Groupings per View Type

Since actual bitmask values live in asset bundles, the table below is inferred from the view purpose and which loaders are registered in `HUDManager`:

| Element | Default | Starbase | Battle | Arena | Surge | Wave Defense |
|---|---|---|---|---|---|---|
| PlayerProfile | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Chat | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Resources | ✅ | ✅ | ❌ | ❌ | ❌ | ✅ |
| Offers | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| Jobs | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| Missions | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| Navigation | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| StationStatus | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| FleetBar | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| LeftThumb | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| RightThumb | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| AllianceHelp | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| Loyalty | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| HudFrame | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| WaveDefense | ❌ | ❌ | ✅* | ❌ | ❌ | ✅ |
| LocatorsAndFleetCommander | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| ArenaScoring | ❌ | ❌ | ✅* | ✅ | ❌ | ❌ |
| SurgeScoring | ❌ | ❌ | ✅* | ❌ | ✅ | ❌ |

> ✅* = only shown in the matching battle variant (e.g. ArenaScoring only in `_battleViewArenaHudSettings`)

### Relevant Source Files

| File | Role |
|---|---|
| `HUD/HUDElements.cs` | Defines the 18 element IDs and string keys |
| `HUD/HUDElementsMask.cs` | Bitmask type for element sets |
| `HUD/HUDPanelsState.cs` | Per-element `PanelState` struct |
| `HUD/HUDSettings.cs` | ScriptableObject: mask + panel states per view |
| `HeadsUpDisplay/HUDSettingsHandler.cs` | Holds all view-specific settings, applies them |
| `HUD/HUDManager.cs` | Central manager: owns all loaders, applies masks |

---

## HUD Element Screen Positions

> Exact pixel coordinates are baked into Unity prefab assets and are not readable from decompiled code. However, `RectTransform` anchor data **was** extracted from the `hud/prefabs` AssetBundle using UnityPy.
>
> **Evidence legend:**
> - **Prefab-confirmed** — `RectTransform.anchorMin/anchorMax` directly extracted from `hud/prefabs` AssetBundle
> - **Confirmed** — explicit directional field names in C# (e.g., `_leftThumbMenuLoader`) or relative-position enums (`OnTheLeft` / `OnTheRight`)
> - **Inferred** — deduced from behavioral relationships in code (e.g., reacts to fleet-bar visibility)
> - **Unconfirmed** — no explicit evidence; placement based on UI convention

### Layout Diagram (Default / Galaxy / System View)

```
┌─────────────────────────────────────────────────────────────────────┐
│ ┌────────────┐                                      ┌─────────────┐│  TOP
│ │ PlayerProf │  ┌────────────────────────────┐      │  Resources  ││
│ │ (avatar)   │  │        HudFrame            │      │ (parsteel,  ││
│ └────────────┘  │  banner / challenge timer  │      │  dilithium,  ││
│  ┌──────────┐  │  crisis timer / boss HP    │      │  tritanium) ││
│  │  Loyalty │  └────────────────────────────┘      └─────────────┘│
│  │ (pts bar)│                                                      │
│  └──────────┘                                                      │
│                                                                    │
│ ┌─────────────┐                                          ┌────────┐│  MID
│ │ Jobs panel  │                                          │Station ││
│ │ (queues)    │              (game viewport)             │Status  ││
│ │ Missions    │                                          │(shield)││
│ │ (buttons)   │                                          └────────┘│
│ └─────────────┘                                                    │
│                                                                    │
│  ┌──────────┐  ┌─────────────────┐  ┌──────────────────────────┐   │  LOW
│  │ ChatIcon │  │  Navigation     │  │ LocatorsAndFleetCommander│   │
│  │ (top-L)  │  │  (bottom-right) │  │ (home, cmdr ability)     │   │
│  └──────────┘  └─────────────────┘  └──────────────────────────┘   │
│ ┌──────────┐ ┌──────────────────────────┐ ┌──────────────────────┐│
│ │ LeftThumb│ │        FleetBar          │ │     RightThumb       ││  BOTTOM
│ │ (actions)│ │  (dock 1, 2, 3 … n)     │ │     (actions)        ││
│ └──────────┘ └──────────────────────────┘ └──────────────────────┘│
└─────────────────────────────────────────────────────────────────────┘
```

### Per-Element Position Notes

| Element | Screen Region | Key Evidence |
|---|---|---|
| **PlayerProfile** | Top-left | **Prefab-confirmed** — `HUD_Drawer_CanvasV2` → `DrawerContainer` top-left size=(382,136); `OpenDrawer` top-left size=(377,79.8) |
| **Resources** | Top-right | **Prefab-confirmed** — `ResourcesListContainer01` top-right size=(425.1,42) containing Parsteel/Dilithium/Tritanium widgets; swap button top-right |
| **Offers** (`HUDAllianceChestAndNews`) | Top-right | **Prefab-confirmed** — `HUD_AllianceHelpAndNews_Canvas` → `ButtonContainer` top-right pos=(-14,-205.4); `PromotionImage` right-stretch; PLC/MetaEvent/ReturnPlayer containers top-left within parent |
| **AllianceHelp** | Top-right (shared canvas with Offers) | **Prefab-confirmed** — Same `HUD_AllianceHelpAndNews_Canvas`; `AllianceHelpSuccessHolder` center; `AllianceHelpIcon` bottom-left; `OpenDrawer` top-right size=(322.7,79.8) |
| **Loyalty** | Top-left | **Prefab-confirmed** — `HUD_Loyalty_Canvas` → `Container` top-left pos=(342.2,-80.6) size=(130,50) with progress bar, pip, icon |
| **HudFrame** | Full-screen overlay / top bar | **Prefab-confirmed** — `HUD_Frame_Canvas` → `BodyContainer` full-screen; `BossHPContainer`/`ChallengeMissionContainer`/`ServerClashContainer`/`RegionalSpaceContainer` all have `BannerContainer` top-center |
| **Jobs** | Left side | **Prefab-confirmed** — `HUD_JobsPanel_Canvas` → `OpenPanelButton` top-left pos=(-0.3,-283.7); `Jobs_PanelContainer` left-stretch pos=(8.9,13.4) size=(392,-513.6) |
| **Missions** | Bottom-left | **Prefab-confirmed** — `HUD_Missions_Canvas` → `Layout` bottom-left pos=(0,176) size=(552.2,100); `MissionButton` top-left; `ChallengesButton`/`AchievementsButton`/`DailyGoalsButton`/`OutpostsButton` all bottom-left |
| **StationStatus** | Bottom-right | **Prefab-confirmed** — `Station_Info_Canvas` → `StationCombatInformation_Container` bottom-right pos=(0,179) size=(333,138.7); `ShieldContainer` bottom-right; `ConsumablesContainer` bottom-right |
| **Navigation** | Bottom-right | **Prefab-confirmed** — `HUD_NavigationButton_Canvas` → `LargeButton` bottom-right pos=(-66.3,90.8) size=(130,131); `SmallButton` bottom-right pos=(-176.8,76.8) |
| **Chat** | Top-center feed + top-left icon | **Prefab-confirmed** — `HUD_ChatFeed_Canvas` → `ChatFeed_Container` top-center size=(440,96.1); `HUD_Preview_ChatContent` → `ChatIcon` top-left pos=(16.4,-7.8) |
| **LocatorsAndFleetCommander** | Bottom-right / lower-right | **Prefab-confirmed** — `HUD_HomeLocatorAndFleetCommander_Canvas` → `HomeLocationWidget` bottom-right pos=(-184.8,382.2); `Container` bottom-right pos=(0,338.3); `FleetCommanderAbilityWidget` mid-right |
| **FleetBar** | Bottom-centre | **Prefab-confirmed** — `ShipBar_Canvas` → `EmptyArmada` bottom-center pos=(0,229.8); `ShipBarContainer` bottom-center size=(857.6,168.9) |
| **LeftThumb** | Bottom-left corner | **Confirmed** — named `_leftThumbMenuLoader` in `HUDManager`; thumb-reach position |
| **RightThumb** | Bottom-right corner | **Confirmed** — named `_rightThumbMenuLoader` in `HUDManager`; thumb-reach position (prefab not in `hud/prefabs` bundle) |
| **WaveDefense** | Top-center | **Prefab-confirmed** — `HUD_WaveDefense_Canvas` → `BannerContainer` top-center pos=(0,-21) size=(482,42); `WaveNumber`/`NumberOfPlayers`/`LeaderName`/`TimeToStart` all top-center |
| **ArenaScoring** | Full-screen overlay (top-right) | **Prefab-confirmed** — `HUD_ArenaScoring_Canvas` → `ScoreContainer` full-screen; `TimerContainer` top-right; `DynamicResources` top-right |
| **SurgeScoring** | Full-screen overlay (top-right) | **Prefab-confirmed** — `HUD_SurgeScoring_Canvas` → `ScoreContainer` top-right pos=(-225,-30) size=(460,62); `TimerContainer` top-right; `WarmupContainerCenter` center |

### Toast Position

Toasts are managed by `ToastManager` / `ToastViewController` and appear as a **top-center notification strip**. The `DismissDirection` enum (`Up`, `Left`, `Right`) indicates they are individually dismissed by swiping in any of those directions. The `_allowableTimeSinceCreation = 5s` window at session start suppresses early toasts.

**Prefab-confirmed:** `ToastViewController` anchor=(0.5,1.0) **top-center** pos=(1.0,-126.0) size=(800.0,78.0) — a wide strip across the top of the screen.

---

## PlayerProfile — Detailed Breakdown

The `PlayerProfile` HUD element has two distinct states controlled by `PanelState` / `HudVisibilityController`:

| State | What shows |
|---|---|
| `Collapsed` / `Open` | Compact HUD button (always on-screen) |
| `Expanded` | Full profile sheet slides in |

---

### Compact HUD Button (always visible)

Rendered by **`UserProfileWidget`** (`PlayerProfile/UserProfileWidget.cs`).

```
┌────────────────────────────────────┐
│ ┌───────┐  Player Name             │
│ │ Frame │  [PlayerTitle]           │
│ │+Avatar│  Lv. 45                  │
│ └───────┘  Server-42               │
│ ● (avatar pip)  ○ (title pip)      │
└────────────────────────────────────┘
```

| Sub-element | Field | Notes |
|---|---|---|
| Avatar + decorative frame | `_profileFrameAndAvatar` (`FrameAndAvatarWidget`) | Cosmetic frame around the avatar image |
| Player name | `_name` (TextLocalizer) | Style controlled by `NameStyle` enum |
| Player title | `_playerTitleWidget` (`PlayerTitleWidget`) | Equipped title displayed below name |
| Level | `_level` (TextLocalizer) | Player ops level number |
| Server name | `_serverName` (TextLocalizer) | Server identifier (e.g. "Server 42") |
| Rival server badge | `_serverRivalWidget` (`ServerRivalWidget`) | Shown if player is on a rival/regional server |
| Avatar/frame pip | `_avatarAndFramePipWidget` | Notification dot — new cosmetic avatar or frame available |
| Title pip | `_playerTitlePipWidget` | Notification dot — new player title available |

**`NameStyle` enum** (`UserProfileWidget`) — controls what text appears next to the avatar:

| Value | Text shown |
|---|---|
| `NameOnly` | Just player name |
| `NameAndLevel` | Name + level |
| `NameWithTag` | `[TAG] Name` |
| `NameWithDiplomacy` | Name with diplomacy symbol prefix |
| `NameWithRivalServer` | Name + rival server number |
| `NameWithTagAndRivalServer` | `[TAG] Name` + rival server |
| `AllInfo` | Everything |

---

### Full Profile Panel (opened on tap)

Rendered by **`PlayerProfileViewController`** (`PlayerProfile/PlayerProfileViewController.cs`). Contains four sub-widgets:

#### 1. Player Info (`UserProfileWidget` — same widget as HUD button, but in expanded mode)
- Avatar, frame, name, level, server — same fields as above

#### 2. Player Stats (`PlayerStatsWidget`)
- `_mainGroup` — primary stats row (power, etc.)
- `_statsList` — scrollable list of all player stats
- Animates in with a loading state (`_loadingCustomId`)

#### 3. Actions (`PlayerProfileActionsWidget`)

| Button | Field | Shown when |
|---|---|---|
| Alliance Invite | `_allianceInviteButton` | Viewing another player not in your alliance |
| Send Message | `_sendMessageButton` | Viewing another player |
| Help | `_helpButton` | Alliance help context |
| Settings | `_settingsButton` | Own profile only |
| View Screen | `_viewScreenButton` | Videos feature enabled (pip: new videos) |
| Social Media | `_socialMediaScreenButton` | Social link enabled |

> Edit buttons (`_avatarEditButton`, `_playerTitleEditButton`) and `_utcTime` (UTC clock) are also present — own-profile only.

#### 4. Alliance Area (`PlayerProfileAllianceAreaWidget`)

| Sub-element | Field | Notes |
|---|---|---|
| Alliance name | `_allianceName` | Full alliance name |
| Alliance tag | `_allianceTag` | Short `[TAG]` |
| Alliance icon | `_allianceIcon` (`ImageSelector`) | Alliance emblem |
| League badge | `_leagueImageSelector` + `_leagueTextLocalizer` | Novice / standard league indicator |
| Join button | `_allianceJoinButton` | Visible if `CanJoin` and player has no alliance |
| Go to button | `_allianceGoToButton` | Navigate to target's alliance |
| Diplomacy symbol | `_allianceTagAndDiplomacyTextContext` | Shown when alliance diplomacy is set |

**`TargetType`** enum controls which layout variant renders:

| Value | Scenario |
|---|---|
| `UserNoAlliance` | Viewing own profile, no alliance |
| `UserWithAlliance` | Viewing own profile, in alliance |
| `OtherInUserAlliance` | Viewing a member of your own alliance |
| `OtherInOtherAlliance` | Viewing a player in a different alliance |
| `OtherNotInAlliance` | Viewing a player with no alliance |

---

### Relevant Source Files

| File | Role |
|---|---|
| `PlayerProfile/PlayerProfileLoadAndShow.cs` | HUD loader — wraps `GenericLoadAndShowUI` |
| `PlayerProfile/PlayerProfileDirector.cs` | Section director — manages enter/exit of the profile screen |
| `PlayerProfile/PlayerProfileViewController.cs` | Full panel: aggregates all four sub-widgets |
| `PlayerProfile/PlayerProfileDataContext.cs` | Reactive context: `TargetProfile` + `TargetStats` |
| `PlayerProfile/PlayerProfileManager.cs` | Singleton — `ShowPlayerProfile(userId)` entry point |
| `PlayerProfile/UserProfileWidget.cs` | Compact avatar/name/level widget (HUD button + panel header) |
| `PlayerProfile/PlayerStatsWidget.cs` | Scrollable stats list |
| `PlayerProfile/PlayerProfileActionsWidget.cs` | Action buttons (invite, message, settings, view screen) |
| `PlayerProfile/PlayerProfileAllianceAreaWidget.cs` | Alliance info, join/go-to buttons, league badge |

---

## Loyalty — Detailed Breakdown

The `Loyalty` HUD element (index 12, key `loyalty`) has two distinct states:

| State | What shows |
|---|---|
| `Collapsed` / `Open` | Compact HUD button — tier + progress bar |
| `Expanded` | Full loyalty screen |

There are **two parallel loyalty tracks** sharing the same widget structure, distinguished by `IsAllianceLoyalty`:
- **Personal Loyalty** — individual points earned by playing
- **Alliance Loyalty** (`AllianceLoyaltyManager`) — points earned collectively by alliance activity

---

### Compact HUD Button

Rendered by **`HudLoyaltyViewController`** (`HUD/HudLoyaltyViewController.cs`).

```
┌───────────────────────────────┐
│  Tier 4                       │
│  [████████░░░░░░░░░░░░░░░░░░] │  ← progress bar
│  ● (ready to collect badge)   │
│  [lock icon if not unlocked]  │
└───────────────────────────────┘
```

| Sub-element | Field | Notes |
|---|---|---|
| Tier label | `_currentLoyaltyTier` (TextLocalizer) | Displays current tier number |
| Progress bar | `_progressBarWidget` (`ProgressBarWidget`) | Points towards next tier |
| Container | `_loyaltyContainer` | GameObject shown/hidden by feature unlock state |
| Unlock status | `_loyaltyUnlockedStatusWidget` (`PreviewableModuleLockedStatusWidget`) | "Module locked" preview if loyalty not yet unlocked |
| Open button | `_openLoyaltyScreenButton` | Tapping opens the full loyalty screen |
| Animator param | `_hasBundlesToCollectParamInfo` | Triggers pulse/glow animation when rewards are waiting |

**Events the HUD button reacts to** (via `LoyaltyManager` static events):

| Event | Handler | Effect |
|---|---|---|
| `LoyaltyPointsUpdatedEvent` | `LoyaltyPointsUpdatedEventHandler` | Redraws progress bar |
| `LoyaltyTierBundlesUpdatedEvent` | `LoyaltyTierBundlesUpdatedEventHandler` | Triggers "ready to collect" animation |
| `LoyaltyTieredUpEvent(int tierId)` | `LoyaltyTieredUpEventHandler` | Plays tier-up animation |

---

### Context Data (`HudLoyaltyContext`)

| Property | Type | Notes |
|---|---|---|
| `CurrentTier` | `int` | Current tier number |
| `CurrentPoints` | `long` | Points accumulated towards next tier |
| `PointsRequired` | `long` | Points needed to reach next tier |
| `MaxTierReached` | `bool` | Hides progress bar when at max |
| `TierProgressData` | `ProgressData` | Struct for progress bar fill value |
| `IsAllianceLoyalty` | `bool` | Routes to personal or alliance loyalty data |
| `AllianceLoyaltyResourceId` | `long` | Resource ID used for alliance track |

---

### Full Loyalty Screen

Rendered by **`LoyaltyViewController`** (`Loyalty/LoyaltyViewController.cs`).

```
┌────────────────────────────────────────────┐
│  [Current Loyalty Points Resource widget]  │
│  Tier 4    [████████████░░░░░░░] X to next │
│  [Claim Daily Bundle]  [Buy Loyalty Pts]   │
│  ─────────────────────────────────────     │
│  Tier 1  ✓ Claimed   [normal rewards]      │
│  Tier 2  ✓ Claimed   [buff rewards]        │
│  Tier 3  ► Current   [rewards] [CLAIM]     │  ← scrollable
│  Tier 4  🔒 Locked   [rewards]             │
│  Tier 5  🔒 Locked   [rewards]             │
│  ─────────────────────────────────────     │
│  [ℹ Info]   [Timer: 12:34:56]             │
└────────────────────────────────────────────┘
```

| Sub-element | Field | Notes |
|---|---|---|
| Resource display | `_resourcesWidget` (`ResourceWidget`) | Shows total loyalty points held |
| Tier label | `_currentTier` (TextLocalizer) | "Tier X" |
| Progress bar | `_loyaltyTierProgressWidget` (`SimpleProgressBarWidget`) | Fill towards next tier |
| Points label | `_pointsToNextTierLabel` (TextLocalizer) | "X points to next tier" |
| Tier list | `_smartScroller` (`SmartScrollerBase`) | Scrollable list of `LoyaltyTierWidget` rows |
| Claim daily button | `_claimDailyBundleButtonWidget` | Collect daily loyalty bundle; hidden during cooldown |
| Buy points button | `_buyLoyaltyPointsButtonWidget` | Premium store button |
| Daily cooldown timer | `_timerWidget` (`TimerWidget`) | Countdown until next daily bundle |
| Info button | `_infoButton` | Opens `_infoSlideshowDialogContext` — feature explanation |
| Reward animation | `_rewardsController` (`FlyByRewardsController`) | Fly-by particles on claim |

**Daily bundle states** (`LoyaltySectionContext.DailyBundleState`):

| State | Shown |
|---|---|
| `ReadyToCollect` | Claim button active |
| `CooldownTimerActive` | Timer widget counting down |
| `NotAvailable` | Neither shown |

---

### Tier Row (`LoyaltyTierWidget`)

Each row in the scrollable list renders one loyalty tier:

| Sub-element | Field | Notes |
|---|---|---|
| Tier number | `_tierNumberLocalizer` | "Tier 1", "Tier 2", etc. |
| Banner text | `_bannerLocalizer` | "Current", "Locked", "Claimed", etc. |
| Normal rewards | `_tierNormalRewardWidgets` (`StaticListContainer`) | Items / resources rewarded |
| Buff rewards | `_tierBuffRewardWidgets` (`StaticListContainer`) | Passive buff rewards |
| Claim button | `_claimRewardButton` | Active only when `CurrentRewardClaim` or `PreviousRewardClaim` |
| Reward animation | `_rewardsController` | Per-tier fly-by on claim |

**`LoyaltyTierWidgetState`** enum — drives the tier row animator:

| State | Meaning |
|---|---|
| `LockedState` | Tier not yet reached |
| `PreviousReward` | Tier passed, reward already claimed |
| `PreviousRewardClaim` | Tier passed, reward **not yet claimed** |
| `CurrentReward` | Active tier, reward not ready |
| `CurrentRewardClaim` | Active tier, reward ready to claim |
| `NextReward` | Next tier (preview state) |

---

### Pip Notification (`LoyaltyPipManager`)

The notification dot on the HUD button is driven by `LoyaltyPipManager` (`Notifications/LoyaltyPipManager.cs`):

- Watches `LoyaltyTierBundlesUpdatedEvent`, `LoyaltyTieredUpEvent`, and `StaticSyncCompleted`
- `GetNumLoyaltyBundlesToCollect()` — returns the count of unclaimed tier bundles; drives the pip count

---

### Relevant Source Files

| File | Role |
|---|---|
| `HUD/HUDLoyaltyLoadAndShow.cs` | HUD loader — `InitializeContext()` wires up the `HudLoyaltyContext` |
| `HUD/HudLoyaltyViewController.cs` | Compact HUD button — tier label, progress bar, animations |
| `HUD/HudLoyaltyContext.cs` | Data context for the HUD button |
| `Loyalty/LoyaltyViewController.cs` | Full loyalty screen — tiers, daily bundle, rewards |
| `Loyalty/LoyaltySectionContext.cs` | Full screen data context — tier list, daily bundle state |
| `Loyalty/LoyaltyTierWidget.cs` | One tier row — number, banner, rewards, claim button |
| `Loyalty/LoyaltyTierWidgetContext.cs` | Data for one tier row |
| `Loyalty/LoyaltyTierWidgetState.cs` | Enum: 6 visual states per tier |
| `Loyalty/LoyaltyManager.cs` | Singleton — points, tiers, bundles, purchase events |
| `Alliances/AllianceLoyaltyManager.cs` | Alliance loyalty track — milestones, resource ID |
| `Notifications/LoyaltyPipManager.cs` | Drives the notification pip count on the HUD button |

---

## Detailed Breakdowns — Remaining 16 HUD Elements

Grouped by screen region. Only `PlayerProfile` and `Loyalty` have standalone detailed sections above; everything below is new.

---

### Group 1: Top Bar Cluster

#### Resources (`HUDResourcesViewController`)

Top-right resource bar (~425px wide). Primary currencies (parsteel, dilithium, tritanium) displayed horizontally.

| Sub-element | Field | Notes |
|---|---|---|
| Resource list | `_resourcesList` (`BaseListContainer`) | Primary: dilithium, tritanium, parsteel |
| Hard currency | `_hardCurrencyList` (`BaseListContainer`) | Latinum / premium currency |
| Swap button | `_swapResourcesButton` (`Button`) | Toggles T1/T2/dynamic visibility; shown when player level >= `_levelToShowSwapRss` |
| Territory scores | `_territoryScoresHudWidget` (`TerritoryScoresHudWidget`) | Alliance territory score overlay; gated by `_showTerritoryScores` |
| Show animator | `_stateShowTriggerAnimatorHash` | Slide-in trigger |
| Hide animator | `_stateHideTriggerAnimatorHash` | Slide-out trigger |

**Resource visibility tiers:**

| Tier | Value | Resources shown |
|---|---|---|
| `T1ResourcesVisible` | 0 | Basic: parsteel, tritanium, dilithium |
| `T2ResourcesVisible` | 1 | Advanced: e.g. gas, ore, crystal variants |
| `DynamicResourcesVisible` | 2 | Event/activity-specific currencies |

**Reactive behavior:** Listens to `UIFleetBarHideEventHandler` / `UIFleetBarShowEventHandler`. When the fleet bar expands, the resources bar slides up to make room; when the fleet bar collapses, the resources bar slides back down.

---

#### HudFrame (`HudFrameViewController`)

Full-screen top overlay for global event banners, challenge timers, and boss health.

| Sub-element | Field | Notes |
|---|---|---|
| Server clash banner | `_serverClashBannerLabel` (`TextLocalizer`) | Cross-server tournament announcement text |
| Challenge timer | `_challengeTimer` (`TimerWidget`) | Countdown for active challenge events |
| Crisis timer | `_crisisTimer` (`TimerWidget`) | Dynamic crisis event countdown |
| Crisis title | `_crisisTitleText` (`TextLocalizer`) | Crisis event name |
| Boss health bar | `_bossHealthBar` (`ServerBossHealthBar`) | Server boss armada HP display |
| Animator | `_animator` | State machine driving which banner is visible |

**Animator parameters:**
- `_stateAnimatorParameterName = "State"` — which banner mode is active (clash, challenge, crisis, boss)
- `_galacticAnomalyActiveAnimatorParameterName = "GalacticAnomalyActive"` — galactic anomaly modifier overlay

The controller switches between banner modes reactively based on which global events are currently live. Only one primary banner is shown at a time.

---

#### Offers (`HudAllianceAndNewsViewController` + `HudPromotionLoadAndShow`)

Top-right vertical column of promotional buttons and event entries.

**From `HudAllianceAndNewsViewController`:**

| Sub-element | Field | Notes |
|---|---|---|
| Alliance help | `_allianceHelpButton` (`SemaphoreButtonListener`) | "Help All" — speeds up alliance member jobs |
| News | `_newsButton` (`GenericButtonWidget`) | In-game news / patch notes |
| View screen | `_viewScreenButton` (`GenericButtonWidget`) | Video ads / view screen for rewards |
| Meta event promo | `_metaEventPromotion` (`PromotionElementWidget`) | Active meta event banner |
| Free chest | `_chestPromotion` (`PromotionElementWidget`) | Alliance free chest claim |
| PLC offer | `_plcOfferPromotion` (`PromotionElementWidget`) | Premium limited-time offer |
| Return player | `_returnPlayerPromotion` (`PromotionElementWidget`) | Return player incentive |
| Visibility | `_buttonVisibilityController` (`VisibilityController`) | Master show/hide for the whole column |
| State animator | `_buttonStateAnimator` (`Animator`) | Transitions between button states |
| Help feedback | `_helpSucceededLocalizer` | "Help sent to X allies" toast text |

**Special button positioning** (`SpecialButtonState`):
- `Hide` — no special button shown
- `OnTheLeft` — special button positioned left of main cluster
- `OnTheRight` — special button positioned right of main cluster

**From `HudPromotionLoadAndShow`:**

| Sub-element | Field | Notes |
|---|---|---|
| Sale element | `_saleElement` (`PromotionElement`) | Current shop sale bundle |
| Event element | `_eventElement` (`PromotionElement`) | Active tournament / event entry |

**Event subscriptions:**
- `OnEventsAvailable` — populates event element
- `OnBundleListChanged` / `OnBundlesStateChanged` — updates sale element
- `OnClaimableTournamentRewardEvent` — highlights claimable tournament rewards
- `GameActivityAssignedEventHandler` / `GameActivityEndedEventHandler` — arena / surge activity state
- `PartyJoinedEventHandler` / `PartyLeftEventHandler` / `PartyReadyEventHandler` — party/lobby state

---

#### AllianceHelp (`HudAllianceAndNewsViewController`)

Shares the same view controller as **Offers** (above). The alliance help button is a distinct sub-element within the top-right column.

| Sub-element | Field | Notes |
|---|---|---|
| Help button | `_allianceHelpButton` (`SemaphoreButtonListener`) | One-tap "Help All" for alliance job queues |
| Help succeeded text | `_helpSucceededLocalizer` | Feedback after helping |

When tapped, it fires help requests for all alliance members' active jobs and shows a success toast with the count of allies helped.

---

### Group 2: Left-Side Panels

#### Jobs (`JobQueuePanelViewController`)

Left-side panel showing active job queues (builds, research, ship upgrades, refinery). Prefab-confirmed: `HUD_JobsPanel_Canvas` → `Jobs_PanelContainer` left-stretch pos=(8.9,13.4) size=(392,-513.6); `OpenPanelButton` top-left.

| Sub-element | Field | Notes |
|---|---|---|
| Visibility | `_visibilityController` (`VisibilityController`) | Master show/hide |
| Open/close button | `_openCloseButton` (`Button`) | Toggle panel visibility |
| Contract/expand | `_contractExpandButton` (`GenericButtonWidget`) | Collapse to compact view |
| Job list (full) | `_jobList` (`BaseListContainer`) | Complete list of active jobs |
| Job list (compact) | `_jobListCollapsed` (`BaseListContainer`) | Summary view when collapsed |
| Animator | `_animator` | List-collapse animation |
| Confirmation gate | `_requestConfirmation` | Whether to prompt before spending resources |

**Actions:**
- `RequestAction(JobActionType, JobWidget)` — speed up (instant finish), cancel, or ask for alliance help on a job
- `ShowJobQueuePanelEvent` — static event that external systems can fire to force-open the panel
- `FadeOutList` coroutine — smooth list transitions when jobs complete and are removed

Jobs are displayed as scrollable rows; each row shows the job type, target building/ship, time remaining, and action buttons.

---

#### Missions (`MissionsHudViewController`)

Bottom-left button cluster for missions, achievements, dailies, outposts, and challenges. Prefab-confirmed: `HUD_Missions_Canvas` → `Layout` bottom-left pos=(0,176) size=(552.2,100); all buttons (`MissionButton`, `AchievementsButton`, `DailyGoalsButton`, `ChallengesButton`, `OutpostsButton`) bottom-left anchored.

| Sub-element | Field | Notes |
|---|---|---|
| Missions button | `_missionsButton` (`Button`) | Main story / event missions |
| Achievements button | `_achievementsButton` (`GenericButtonWidget`) | Achievement tracker |
| Achievement pip | `_achievementPipWidget` | Unclaimed achievements count |
| Outposts button | `_outpostsButton` (`GenericButtonWidget`) | Outpost management |
| Daily goals button | `_dailyGoalsButton` (`GenericButtonWidget`) | Daily milestone tracker |
| Challenges button | `_challengesButton` (`Button`) | Active challenges |
| Notification popout | `_notificationController` (`MissionsNotificationPopoutWidget`) | Slide-out mission update panel |
| Daily widget | `_dailyHudWidget` (`MissionDailyMilestoneHUDWidget`) | Daily progress bar |
| Rewards | `_rewardsController` (`FlyByRewardsController`) | Reward fly-by animation on claim |
| Dailies animator | `_dailiesStateAnimator` | Daily goals expand/collapse states |

**Button priority system** (`MissionsHUDButtonPriority`):
Only one button is prominently shown at a time based on priority logic. The controller evaluates which mission/achievement/daily/challenge has the highest priority and surfaces that button.

- `_timeToShowPopout` — seconds before auto-dismissing the popout
- `_timeToShowCompletePopout` — seconds before auto-dismissing completion popout

---

#### StationStatus (`StationInfoViewController` + `StationStatusViewController` + `StationRepairViewController` + `StationWarningViewController`)

Bottom-right cluster of widgets showing starbase state (shield, consumables, bookmarks, repair). Prefab-confirmed: `Station_Info_Canvas` → `StationCombatInformation_Container` bottom-right pos=(0,179) size=(333,138.7).

**`StationInfoViewController`** (main panel):

| Sub-element | Field | Notes |
|---|---|---|
| Shield timer | `_shieldTimer` (`TimerWidget`) | Peace shield countdown |
| Cease-fire timer | `_ceaseFireTimer` (`TimerWidget`) | Cease-fire countdown |
| Repair widget | `_stationRepairHUDWidget` (`StationRepairHUDWidget`) | Starbase repair status summary |
| Shield button | `_shieldButton` (`GenericButtonWidget`) | Activate peace shield |
| Consumables button | `_consumablesButton` (`GenericButtonWidget`) | Active consumables list |
| Consumables count | `_numberOfActiveConsumables` (`TextLocalizer`) | How many consumables are active |
| Bookmarks button | `_bookmarksButton` (`GenericButtonWidget`) | System bookmarks quick-access |
| Consumable unlock | `_consumableUnlockedStatusWidget` | Lock preview if consumables not unlocked |
| Shield unlock | `_shieldUnlockedStatusWidget` | Lock preview if shield not unlocked |

**`StationStatusViewController`** (shield-only wrapper):
- `_shieldTimerWidget` — dedicated shield timer display
- `_stateController` (`CenterStateViewController`) — central status icon/text

**`StationRepairViewController`** (repair overlay):
- `_repairButton` — opens the full repair panel
- `_repairTimer` (`SimpleTimerWidget`) — repair countdown
- `_repairJobCountLabel` — number of queued repair jobs
- `_repairNowButtonContext` — instant-repair button (spends premium currency)

**`StationWarningViewController`** (attack warning):
- `_attackerProfileWidget` (`UserProfileWidget`) — shows attacker name, avatar, level
- `_toastButton` — dismisses the attack warning toast

The warning panel appears when the starbase is under attack, showing the attacker's profile widget and a dismiss button.

---

### Group 3: Bottom Bar Cluster

#### FleetBar (`FleetBarViewController`)

Bottom-centre horizontal carousel of fleet dock slots.

| Sub-element | Field | Notes |
|---|---|---|
| Fleet list | `_fleetbarListContainer` (`SelectableList`) | Horizontal carousel of dock slots |
| Spawner | `_listContainerSpawner` (`DynamicListContainer`) | Dynamic slot creation/removal |
| Fleet panel | `_fleetPanelController` (`FleetLocalViewController`) | Expanded fleet detail panel (opens on slot tap) |
| Empty armada | `_emptyArmadaWidget` (`EmptyArmadaWidget`) | Shown when player has not joined an armada |
| Next drydock | `_nextDrydockModuleWidget` (`GotoModuleWidget`) | Button to unlock the next drydock slot |
| Premium drydock | `_premiumDryDockButton` (`GotoModuleWidget`) | Premium instant-unlock button |
| Visibility | `_hudVisibilityController` (`HudVisibilityController`) | Collapse/expand controls |
| Animator | `_animator` | State transitions for expand/collapse |

**Events:**
- `SelectedElementChangedEvent` — fired when player selects a different fleet slot
- `PlayerFleetsAddedEventHandler` — rebuilds the list when new fleets are acquired
- `PlayerArmadaUpdatedEventHandler` / `PlayerArmadaRemovedEventHandler` — armada join/leave updates
- `OnTierUpCompleteSuccess` / `OnTierCompleteFail` — fleet tier-up feedback animations

Each dock slot displays the ship icon, level, and an empty/locked state. Tapping a slot selects that fleet; tapping again or using a dedicated button expands the fleet detail panel.

---

#### LocatorsAndFleetCommander (`HUDHomeLocatorAndFleetCommanderViewController`)

Bottom-right / lower-right area above the fleet bar. Combines navigation home indicator and fleet commander display. Prefab-confirmed: `HUD_HomeLocatorAndFleetCommander_Canvas` → `HomeLocationWidget` bottom-right pos=(-184.8,382.2); `Container` bottom-right pos=(0,338.3); `FleetCommanderAbilityWidget` mid-right.

**Home Locator:**

| Sub-element | Field | Notes |
|---|---|---|
| System name | `_systemLocationWidget` (`SystemLocationWidget`) | Current system name |
| Home pointer | `_homePointer` (`Transform`) | Arrow pointing towards home system |
| Home button | `_homeButton` (`GenericButtonWidget`) | Tap to navigate home |
| Visibility | `_homeLocatorVisController` (`VisibilityController`) | Shows/hides based on camera position |

The home pointer rotates to point at `_homeSystemPosition` (system view) or `_homeGalaxyPosition` (galaxy view) using `CalculateIndicatorToHomeRotation`. It is hidden when the home system is inside the camera frustum (`IsPositionOutsideCameraFrustum` returns false).

**Fleet Commander:**

| Sub-element | Field | Notes |
|---|---|---|
| Commander portrait | `_fleetCommanderWidget` (`OfficerInfoWidget`) | Equipped fleet commander officer portrait |
| Commander button | `_fleetCommanderButton` (`GenericButtonWidget`) | Opens commander details screen |
| Ability widget | `_fleetCommanderAbilityWidget` (`FleetCommanderAbilityWidget`) | Active ability display |
| Use ability button | `_useAbilityButton` (`GenericButtonWidget`) | Trigger commander ability |
| Ability VFX | `_abilityActiveVFX` (`RectTransform`) | Visual effect rectangle during ability active |
| Ability timer | `_abilityTimerWidget` (`SimpleTimerWidget`) | Ability cooldown / duration countdown |
| Size scaling | `_sizePerDock` | Scales element width proportionally to dock slot count |

**Ability states** (`SetAbilityState`):
- Determines whether `_useAbilityButton` is interactable
- Drives `_abilityStateParamHash` and `_abilityTimerParamHash` animator parameters
- When ability is active, `_abilityActiveVFX` plays and `_abilityTimerWidget` counts down

---

#### Navigation (`HudNavigationLoadAndShow`)

Bottom-centre / bottom-left navigation button. Minimal loader class.

| Property | Notes |
|---|---|
| Loader type | Inherits from generic `HUDLoadAndShow` base |
| `InitializeContext()` | Wires up navigation HUD context |
| `OnEnterSection` | Gates visibility by current game section |

Tapping the button opens the galactic navigation / galaxy map screen. Only visible in default/galaxy/system views (hidden during combat and other modes).

---

#### Chat (`HudChatShowAndLoad`)

Top-center chat feed panel + top-left chat icon. `HUD_ChatFeed_Canvas` → `ChatFeed_Container` top-center size=(440,96.1); `HUD_Preview_ChatContent` → `ChatIcon` top-left pos=(16.4,-7.8). Also a bottom-left chat button (full panel opener) in `HudChatShowAndLoad`.

| Property | Notes |
|---|---|
| Loader type | Inherits from generic `HUDLoadAndShow` base |
| `InitializeContext()` | Wires up chat HUD context |

Tapping opens the chat panel (alliance, global, direct messages). Positioned near the `LeftThumb` menu for thumb accessibility.

---

#### LeftThumb (`ThumbMenuViewController` + `HudThumbButtonLoadAndShow`)

Bottom-left corner radial/expandable action menu.

| Sub-element | Field | Notes |
|---|---|---|
| Sub-action list | `_subActionWidgets` (`BaseListContainer`) | Buttons that appear on tap/expand |
| Notification pip | `_pipWidget` | Aggregate notification count for all sub-actions |
| Large button icon | `_largeButtonIcon` (`Image`) | Main thumb menu icon |
| Alliance games icon | `_allianceGamesActiveIcon` / `_allianceIcon` | Swaps between standard alliance and alliance-games icon |
| VFX container | `_vfxContainer` + `_vfx` | Colored glow effect transform |
| VFX colors | `_attackingColor`, `_defendingColor`, `_takeoverColor` | Red = attacking, blue = defending, purple = takeover |

**State tracking:**
- `_takeoverActive` — territory takeover in progress
- `_armadaActive` — armada currently active
- `_defendingArmadaActive` — defending an armada attack
- Animator parameter `_isAllianceGameActiveAnimatorParameterInfo` — toggles alliance-games icon state

The thumb menu expands radially or as a list when tapped, revealing context-sensitive action buttons (attack, defend, dock, scan, etc.).

---

#### RightThumb (`ThumbMenuViewController` + `HudThumbButtonLoadAndShow`)

Bottom-right corner radial/expandable action menu.

Structurally identical to **LeftThumb** (same `ThumbMenuViewController` class), but positioned on the opposite side for right-hand thumb reach. The two thumbs may show different action sets depending on context (e.g., left thumb for movement/navigation, right thumb for combat/actions).

---

### Group 4: Combat Overlays

#### WaveDefense (`HUDWaveDefenseViewController`)

Top-centre overlay during wave defense events.

| Sub-element | Field | Notes |
|---|---|---|
| Wave number | `_waveNumberText` (`TextLocalizer`) | Current wave "Wave X of Y" |
| Player count | `_numberOfPlayersText` (`TextLocalizer`) | Defending player count |
| Ship max | `_numberOfShipsMaxText` (`TextLocalizer`) | Max allowed ships for wave |
| Ship current | `_numberOfShipsCurrentText` (`TextLocalizer`) | Current deployed ships |
| Leader name | `_leaderNameText` (`TextLocalizer`) | Wave defense leader name |
| Countdown | `_timeToStart` (`SimpleTimerWidget`) | Time until next wave starts |
| Animator | `_animator` | State machine for overlay transitions |

**Animator parameters:**
- `GalacticAnomalyActive` — galactic anomaly modifier is active for this wave
- `State` — overall wave defense UI state (waiting, active, complete)
- `ShipCountAlert` — warning animation when ship count is critically low
- `WaveDefenseStatus` — wave in progress / waiting / complete

Only visible in `WaveDefense` and `WaveDefense + Prescan` view contexts.

---

#### ArenaScoring (`HUDArenaScoringLoadAndShow`)

Top-centre overlay during Arena PvP activities.

| Property | Notes |
|---|---|
| Loader type | Minimal `HUDLoadAndShow` inheritor |
| `InitializeContext()` | Sets up arena scoring HUD context |
| Event subscriptions | `GameActivityAssignedEventHandler`, `GameActivityEndedEventHandler` |

Shows/hides based on `GameActivityType.Arena`. Displays live scoring, team standings, and match timer. Only visible in `Battle — Arena` and `Arena` view contexts.

---

#### SurgeScoring (`HUDSurgeScoringLoadAndShow`)

Top-centre overlay during Surge events.

| Property | Notes |
|---|---|
| Loader type | Minimal `HUDLoadAndShow` inheritor |
| `InitializeContext()` | Sets up surge scoring HUD context |
| Event subscriptions | `GameActivityAssignedEventHandler`, `GameActivityEndedEventHandler` |

Shows/hides based on `GameActivityType.Surge`. Displays surge scoring, event progress, and phase timer. Only visible in `Battle — Surge` and `Surge` view contexts.

---

## Transient Overlays & Non-HUD UI

These UI components appear on screen but are **not** part of the 18 `HUDElements` enum. They are managed by separate systems or instantiated inside other HUD canvases.

### Toast Notifications (`ToastManager` / `ToastViewController`)

**Top-center** horizontal notification strip (prefab-confirmed: `ToastViewController` top-center, 800px wide).

- `ToastState` enum — 59 distinct toast types (faction warnings, battle results, armada events, tournament updates, etc.)
- `DismissDirection` — `Up`, `Left`, `Right` swipe to dismiss
- Suppressed for 5 seconds after game session start (`_allowableTimeSinceCreation`)
- Dynamic sub-types: `DynamicCrisisToastWidget`, `ArmadaToastWidget`, `FactionWarningToastWidget`, `ToastArenaObserver`, `ToastSurgeObserver`

### Action Prompt Popup (`ActionPromptPopupViewController`)

Center-screen modal for confirmations, conversions, and ship-ability prompts.

| Sub-element | Field |
|---|---|
| Resource cost | `_resourceWidget` |
| Confirm / Cancel / Close | `_rightButton`, `_leftButton`, `_closeButton` |
| Title + message | `_title`, `_message` |
| Wormhole context | `_wormholeExistsTextContext` |
| Ship abilities | `_shipAbilitiesSettings` |

Supports `DoConversionPopup` for resource conversion flows.

### Prescan / Scan UI (`QuickScanLoadAndShow` / `ScanEngageWidget`)

Appears when engaging a target in combat. Managed by `ScanningManager` in the `Navigation` namespace.

- `QuickScanHashedGameEvents` — events for scan start/cancel
- `ScanEngageWidget` — engage button overlay
- `PreScanTargetWidget` / `DeepScanResultWidget` — target info display
- `ScanEngageButtonsWidget` — action buttons (attack, scan, etc.)

### Center State Indicator (`CenterStateViewController`)

Small central status icon/text driven by `SystemInfoState`. Used by `StationStatusViewController` and others via `SectionStateCheck` delegate.

---

## HUD Prefab Superset — Elements Beyond the 18 `HUDElements`

The `hud/prefabs` AssetBundle contains **32 Canvas roots** and **4,709 GameObjects**. Many of these are sub-components of the 18 HUD elements, but several represent **independent UI canvases** not tracked by `HUDElementsMask` or `HUDPanelsState`.

### Popup Canvases (Root-Level)

| Canvas | Position | Description | Trigger |
|---|---|---|---|
| `LoginStreakPopup_Canvas` | Center | 7-day login reward streak popup | Daily login |
| `ActionPromptPopup_Canvas` | Center | Confirmation modal (resource costs, ship abilities) | Player action |
| `StationRepairPopup_Canvas` | Center | Starbase repair confirmation + cost | Station repair tap |
| `OneClickSpeedupPopup_Canvas` | Center | Speed-up token spend dialog | Job speed-up tap |
| `InventoryPopUp_Canvas` | Center | Inventory / item bag viewer | Inventory button |
| `QueueForLease_Canvas` | Center | Officer lease / hiring popup | Officer hire flow |

### Notification Canvases

| Canvas | Position | Description |
|---|---|---|
| `HUD_Notification_Canvas` | Top-center | Toast notification master container |
| `FactionWeeklyEvents_Message_Container` | Full-screen | Faction weekly event notification toast |
| `DynamicCrisis_Completed_Container` | Full-screen | Crisis completed notification |
| `Faction_DiscoveredContainer` | Full-screen | New faction discovered notification |
| `Faction_PromotionContainer` | Full-screen | Faction rank promotion notification |

### Player Identity & Location

| Element | Parent Canvas | Position | Description |
|---|---|---|---|
| `HUD_PlayerId_Canvas` | Root | Bottom-left | Current system / location name display |
| `LocationText` | `HUD_PlayerId_Canvas` | Full-screen | Text showing player's current system |

### Fleet Bar Sub-Elements (Inside `ShipBar_Canvas`)

| Element | Position | Description |
|---|---|---|
| `ShipActionPanel` | Bottom-left | Expanded action buttons when a ship slot is selected |
| `Abilitybuttons` | Top-right | Ship ability toggle buttons |
| `EmptyArmada` | Bottom-center | UI when player is not in an armada |
| `GoButtonContainer` | Mid-right | "Go" button for armada/assignment launch |
| `ShipBar_CargoCollect` | Center | Cargo collection state indicator |

### Drawer / Profile Sub-Elements (Inside `HUD_Drawer_CanvasV2`)

The drawer is a **top-left** anchored panel (382.3 x 136.2 px) that expands from the compact player profile button. It contains the player avatar, power rating, menu shortcuts, and locked-status previews.

**From `HUDDrawerViewController.cs` (C# fields):**

| Serialized Field | Type | Description |
|---|---|---|
| `_drawerBodyViewController` | `DrawerBodyWidget` | Main drawer body controller |
| `_userProfileDataWidget` | `UserProfileDataWidget` | Player name, level, server info |
| `_profileFrameAndAvatar` | `FrameAndAvatarWidget` | Avatar image + decorative frame |
| `_avatarAndFramePipWidget` | `NotificationPipWidget` | Notification pip on avatar/frame |
| `_refineryUnlockedStatusWidget` | `PreviewableModuleLockedStatusWidget` | Refinery lock preview |
| `_minigamesLockedStatusWidget` | `MinigameLockedStatusWidget` | Minigames lock preview |
| `_museumUnlockedStatusWidget` | `PreviewableModuleLockedStatusWidget` | Museum lock preview |
| `_factionsUnlockedStatusWidget` | `PlayerLevelLockedStatusWidget` | Factions lock preview |

**From prefab hierarchy (`HUD_Drawer_CanvasV2` tree):**

| Element | Position | Size | Description |
|---|---|---|---|
| `DrawerContainer` | Top-left | 382.3 x 136.2 | Main drawer content area |
| `GoToProfileButton` | Full-screen | -271.1 x -33.5 | Invisible hit target for tapping profile area |
| `AvatarHitTarget` | Full-screen | -0.1 x -0.1 | Avatar-specific tap zone |
| `PipContainer` (avatar) | Top-right | 40.0 x 40.0 | Notification pip on avatar |
| `MilitaryMightInfo` | Top-right | 299.6 x 49.8 | Player power rating widget |
| `MilitaryMightInfo/Icon` | Mid-left | 48.0 x 48.0 | Power rating icon (e.g., attack/defense symbol) |
| `MilitaryMightInfo/Amount` | Top-left | 200.0 x 31.9 | Numeric power value text |
| `MilitaryMightInfo/BG` | Center | 272.0 x 40.0 | Background panel for power rating |
| `MilitaryMightInfo/Flare` | Full-screen | 42.9 x -9.7 | Glow flare behind power rating |
| `MilitaryMightInfo/BTN` | Mid-right | 37.5 x 37.7 | Info/help button for power rating |
| `MilitaryMightInfo/Mask` | Mid-left | 54.0 x 60.0 | Mask for power rating icon |
| `MilitaryMightInfo/FlyByContainer` | Mid-left | 50.0 x 50.0 | Reward fly-by animation container |
| `FlyByContainer/TrailLeft` | Top-left | 20.0 x 200.0 | Left reward trail |
| `FlyByContainer/TrailMiddle` | Top-left | 20.0 x 200.0 | Middle reward trail |
| `FlyByContainer/TrailRight` | Top-left | 20.0 x 200.0 | Right reward trail |
| `OfficerContainer` | Top-left | 109.9 x 109.9 | Officer/avatar display area |
| `OfficerContainer/AvatarWidget` | Full-screen | - | Avatar image, frame, loading state |
| `OfficerContainer/LevelContainer` | Bottom-center | 75.4 x 22.1 | Player level badge |
| `LockStatusGroup` | Center | 100.0 x 100.0 | Group of lock-status previews |
| `RefineryLockedStatus` | Center | 100.0 x 100.0 | Refinery "locked" preview |
| `MuseumLockedStatus` | Center | 100.0 x 100.0 | Museum "locked" preview |
| `FactionsLockedStatus` | Center | 100.0 x 100.0 | Factions "locked" preview |
| `MinigamesLockStatus` | Center | 100.0 x 100.0 | Minigames "locked" preview |
| `ResourcesListContainer02` | Top-left | 161.9 x 42.0 | Latinum / hard currency display |
| `ResourceWidget_Latinum` | Full-screen | - | Latinum amount + icon + gradient |
| `AddButton` | Top-right | 35.0 x 35.0 | "Add currency" button (opens store) |
| `ButtonContainer` | Top-left | 39.9 x 80.4 | Container for open/close drawer buttons |
| `OpenDrawerButton` | Top-left | 50.0 x 78.0 | Compact drawer trigger button |
| `OpenDrawer` | Top-left | 377.0 x 79.8 | Expanded drawer menu bar |
| `OpenDrawer/Background_Open` | Full-screen | 211.6 x 20.1 | Background for expanded drawer |
| `OpenDrawer/CloseDrawerButton` | Bottom-left | 46.6 x 124.7 | Close drawer button |
| `OpenDrawer/Crew` | Top-left | 80.0 x 80.0 | Crew/officers menu shortcut |
| `OpenDrawer/Ships` | Top-left | 80.0 x 80.0 | Ship management menu shortcut |
| `OpenDrawer/Items` | Top-left | 80.0 x 80.0 | Inventory/items menu shortcut |
| `OpenDrawer/Minigames` | Top-left | 80.0 x 80.0 | Minigames menu shortcut |
| `OpenDrawer/Faction` | Top-left | 80.0 x 80.0 | Factions menu shortcut |
| `OpenDrawer/Museum` | Top-left | 80.0 x 80.0 | Museum menu shortcut |
| `OpenDrawer/Refining` | Top-left | 80.0 x 80.0 | Refinery menu shortcut |
| `InputBlocker` | Top-left | 486.5 x 222.0 | Full-screen input blocker when drawer is open |

**Key observations:**
- The drawer has **two states**: compact (`OpenDrawerButton`, 50x78) and expanded (`OpenDrawer`, 377x79.8 with 7 menu buttons)
- `MilitaryMightInfo` is a **power rating widget** with icon, numeric amount, background, flare, and a help button
- `FlyByContainer` holds **3 reward trails** (left/middle/right) that animate when rewards are earned
- `LockStatusGroup` contains **4 lock-status widgets** for features not yet unlocked (refinery, museum, factions, minigames)
- `ResourcesListContainer02` shows **Latinum** (premium currency) with an "add" button
- The 7 drawer menu buttons are all **80x80** and arranged horizontally: Crew → Ships → Minigames → Faction → Items → Refining → Museum
- `InputBlocker` prevents tapping through the drawer when it's open

### Home Locator & Fleet Commander (Inside `HUD_HomeLocatorAndFleetCommander_Canvas`)

| Element | Position | Description |
|---|---|---|
| `WaveDefenseLocationWidget` | Bottom-right | Reused home-locator widget for wave defense mode |
| `HUD_FleetCommander_ASA_VFX` | Bottom-center | Active Service Ability VFX (glow warp lines) |
| `FleetCommanderAbilityWidget` | Mid-right | Commander ability cooldown / lock state |

### Promotion-Specific (Inside `HUD_Promotion_Canvas`)

| Element | Position | Description |
|---|---|---|
| `FirstTimeSpenderButton` | Top-right | First-time purchase incentive |
| `HUD_SalesItemContainer` | Top-right | Sale bundle with busy dots, timer, pip |

### Chat Channel Sub-Types (Inside `HUD_ChatFeed_Canvas`)

| Content Object | Description |
|---|---|
| `GlobalChatContent` | Global server messages |
| `NewbiesChatContent` | New player channel |
| `AllianceChatContent` | Alliance-only messages |
| `RegionalChatContent` | Regional/territory messages |

Each channel clone is `bottom-left` anchored, 447.8 x 82px, with its own `ChatIcon` and message template.

### Animator State Names (from `hud/animations`)

States that don't map directly to the 18 `HUDElements` but control visible UI:

| State | What It Controls |
|---|---|
| `ShipBar_MirrorUniverse_Timer_Shown` | Mirror Universe event timer on fleet bar |
| `ShipBarCanvas_TakeoverBannerShow` / `Hide` | Territory takeover banner |
| `asaCooldownCompleteNotification` / `asaNoNotification` | Active Service Ability cooldown toasts |
| `Station_FakeShieldHide` / `Preview` / `Unlock` | Fake shield preview for locked station |
| `Station_IncomingAttack` | Attack warning state |
| `HUD_AllianceHelp_ReturnPlayer_OnTheLeft` / `OnTheRight` | Dynamic button positioning |
| `HUD_AllianceHelp_PlcOffer_OnTheLeft` / `OnTheRight` | PLC offer dynamic positioning |

### Summary: The 18 Elements vs. The Prefab Superset

The 18 `HUDElements` enum values cover the **main functional zones** of the HUD. The `hud/prefabs` bundle is a **superset** that includes:

- **6 popup canvases** (login streak, action prompt, repair, speed-up, inventory, lease)
- **1 notification canvas** with 4+ sub-containers
- **1 player identity canvas** (system name)
- **Fleet bar sub-panels** (ship actions, empty armada, cargo collect)
- **Drawer sub-widgets** (military might, reward fly-by)
- **Chat channel clones** (4 channel types)
- **Promotion-specific widgets** (first-time spender, sale items)
- **Animator-driven states** for territory takeover, Mirror Universe, ASA cooldowns, fake shields

These extra elements are **not** toggled via `HUDManager` masks; they are triggered by their own dedicated managers, event systems, or player actions.

### Relevant Source Files

| File | Role |
|---|---|
| `HUD/ToastManager.cs` | Toast suppression window + queue management |
| `HUD/ToastViewController.cs` | Toast display + dismiss handling |
| `HUD/ToastState.cs` | 59 toast type enum |
| `HUD/DismissDirection.cs` | Swipe dismiss directions |
| `HUD/ActionPromptPopupViewController.cs` | Center confirmation modal |
| `Navigation/QuickScanLoadAndShow.cs` | Scan UI loader |
| `Navigation/ScanningManager.cs` | Scan lifecycle |
| `Combat/ScanEngageWidget.cs` | Combat engage overlay |
| `HUD/CenterStateViewController.cs` | Central status indicator |
