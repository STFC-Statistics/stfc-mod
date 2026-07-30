# STFC Client Structure & UI Analysis

> Generated from decompiled output — Jun 29, 2026

---

## Overall Architecture

The game is built by **Digit Game Studios (Scopely)** under the `Digit.*` namespace. The codebase has three clearly separated tiers:

| Namespace | Role |
|---|---|
| `Digit.Client.*` | Reusable client framework (UI engine, assets, core systems) |
| `Digit.Prime.*` | Game-specific STFC logic (features, HUD, navigation) |
| `Digit.PrimeServer.*` | Server communication (models, services, events, parsers) |

---

## UI Architecture

### Core UI Pattern: ViewController + Widget

- **`Digit.Client.UI.Widget`** — base UI component, binds to data via `IDataContextProvider` / `IRefreshHandler`. All UI elements extend this.
- **`Digit.Client.UI.ViewController`** — screen-level controller, manages lifecycle and navigation.
- **`Digit.Client.UI.CanvasController`** — wraps Unity Canvas. Multiple canvases can be alive simultaneously.
- **`Digit.Client.UI.ScreenManager`** — the root UI orchestrator:
  - Maintains a `List<CanvasController> _screens` stack
  - Fires lifecycle delegates: `_aboutToShowEventDelegates`, `_didShowEventDelegates`, `_aboutToHideEventDelegates`, `_didHideEventDelegates`, `_aboutToDestroyCanvasEventDelegates`
  - Handles screen orientation, safe-frame padding, canvas scale factor
  - Has a `UIFrameManager` and dedicated loading screen, message box, autosave spinner

### Screen Loading Pattern — `LoadAndShowUI`

Screens are not kept in memory — they use a lazy `LoadAndShowUI` / `GenericLoadAndShowUI` pattern (async asset bundle load → show). This is used pervasively across all HUD panels.

---

## HUD Structure (`Digit.Prime.HUD`)

The **`HUDManager`** is the root HUD controller, with one lazy loader per panel:

| Loader Field | Controller | Description |
|---|---|---|
| `_fleetBarLoader` | `FleetBarViewController` | Scrollable ship slot list, drydocks, fleet tier-up, armada button |
| `_resourcesLoader` | `HUDResourcesViewController` | In-game currencies (latinum, resources) |
| `_missionsButtonLoader` | `MissionsHudViewController` | Mission progress button + notification popout |
| `_drawerHudLoader` | `HUDDrawerViewController` | Side drawer / hamburger menu |
| `_jobPanelLoader` | `JobQueuePanelViewController` | Active construction / research jobs |
| `_hudChatLoader` | — | Chat button |
| `_rightThumbMenuLoader` / `_leftThumbMenuLoader` | `ThumbMenuViewController` | Context-sensitive radial/thumb buttons |
| `_stationInfoLoadAndShow` | `StationInfoViewController` | Starbase status and repair |
| `_hudAlliancesLoadAndShow` | — | Alliance chest + news |
| `_hudPromotionLoadAndShow` | `HudPromotionLoadAndShow` | Promotion / offer banners |
| `_hudFrameLoadAndShow` | `HudFrameViewController` | Full-screen frame overlay |
| `_hudWaveDefenseLoadAndShow` | — | Wave Defense event HUD overlay |
| `_arenaScoringLoadAndShow` | — | Arena scoring HUD overlay |
| `_surgeScoringLoadAndShow` | — | Surge scoring HUD overlay |
| `_hudFleetCommanderLoadAndShow` | — | Fleet Commander home locator |

### Toast Notification System

Dedicated per-domain observers feed into a central `ToastManager`:

- `ToastAllianceObserver`
- `ToastArmadaObserver`
- `ToastBookmarksObserver`
- `ToastFactionsObserver`
- `ToastInventoryObserver`
- `ToastTerritoryObserver`
- `TournamentToast`
- `TreasuryToastWidget`
- `GalacticAnomalyToastWidget`
- `FactionWarningToastWidget`
- `ArmadaToastWidget`
- `TerritoryToastWidget`

### HUD Visibility & Unlock System

HUD uses a bitmask `HUDElementsMask` to control visibility. Panels can be individually locked/unlocked based on player progression via `HUDUnlockState` / `HUDUnlockType`. Fog-of-HUD (`_fogOfHUDDisabled`) gates early-game element visibility.

---

## Game World / Navigation (`Digit.Prime.Navigation`)

- **`NavigationSectionManager`** — drives scene transitions between Galaxy and System views, handles zoom animation, Mirror Universe entry points, galaxy extents
- **`GameWorldManager`** — owns the full game world state
- **`NavigationZoom`** — zoom levels controlling what entities are rendered
- **`NavigationCameraEvents`** / **`CameraManager`** — camera pan, zoom, and transitions
- **`NavigationTransitionAnimator`** — animates between galaxy ↔ system views

### Map Entity Widgets

| Widget | Represents |
|---|---|
| `NavigationFleetEntity` / `NavigationFleetWidget` | Player and other fleets |
| `NavigationStarEntityWidget` | Star systems |
| `NavigationStarbaseWidget` | Player starbases |
| `NavigationArmadaWidget` | Active armadas |
| `NavigationOutpostWidget` | Outposts |
| `NavigationTerritoryEntity` / `NavigationTerritoryEntityWidget` | Territory zones |
| `NavigationCourseEntity` | In-flight course paths |
| `NavigationGalaxyFleetEntity` | Fleets visible in galaxy view |

---

## Game Features (`Digit.Prime.*`)

Major feature modules, each with their own directors, ViewControllers, and widgets:

| Module | Key Classes |
|---|---|
| **Ships** | `ShipManagementViewController`, `ShipConstructionDetailsViewController`, `ShipScrappingFilterWidget`, `FleetLocalViewController` |
| **Alliances** | Members, starbase, territory capture (takeover system), applications, loyalty |
| **Officers** | Assignment, promotion, traits, synergy, away teams |
| **Research** | Trees, projects, prestige research |
| **Missions** | Active/available, objectives, FTUE tutorial chain |
| **Shop** | Bundle system, chained offers, refinery, PLC offers, Latinum/DRP bundles, web store, battle pass rewards |
| **Combat** | Armadas (PvE + cross-alliance), Wave Defense, Arena, Surge, Ship vs Ship |
| **Chat** | Full system over WebSocket (`WebSocketSharp`) |
| **Notifications** | Pip system per feature area |
| **Events** | Tournaments, Meta Events, Dynamic Crisis, Battle Pass, Challenge Track, Minigames |
| **Other** | Officers Away Teams, Warchest, Museum, Artifact Hall, HailingFrequencies, ViewScreen, SlideShow, Server Transfer |

---

## Server-Side Services (`Digit.PrimeServer.Services`)

Client-side service layer that communicates with the backend:

| Service | Responsibility |
|---|---|
| `GameWorldService` / `SystemService` | World state, system data, galaxy nodes |
| `DeploymentService` / `FleetService` / `FleetCommanderService` | Fleet deployment and movement |
| `ScanningService` | Quick-scan and detailed scan of entities |
| `ArmadaService` | Armada attack lifecycle |
| `InventoryService` / `SpecService` | Inventory and all static game specs |
| `StarbaseService` / `ResearchService` | Base building and research |
| `ConnectivityService` | WebSocket / SignalR real-time connection |
| `TerritoryCaptureService` / `ThreatLevelService` | PvP territory systems |
| `BuffService` | Active buff management |
| `MirrorUniverseService` | Mirror Universe event |
| `WaveDefenseService` | Wave Defense event |
| `AllianceMemberService` / `AllianceApplicationService` / `AllianceStarbaseService` | Alliance management |
| `PartyService` | Party / co-op grouping |
| `JobService` | Construction / research job queue |
| `UserProfileService` | Player profile and settings |
| `FactionsService` | Faction standing |

### Data Models (`Digit.PrimeServer.Models`)

788 model files covering every game entity: `Ship`, `Fleet`, `Station`, `Officer`, `Mission`, `Research`, `Buff`, `Territory`, `Armada`, `Alliance`, `Inventory`, `Bundle`, `HullSpec`, `WeaponSpec`, `ComponentSpec`, and hundreds more.

---

## Third-Party Integrations

| Library | Purpose |
|---|---|
| **Scopely SDK** (`Scopely.*`) | Analytics, attribution, Firebase, in-app messaging, remote config, rule engine |
| **Adjust SDK** | Mobile attribution |
| **Bugsnag** | Crash reporting |
| **NodeCanvas** | Behaviour trees (NPC / AI logic) |
| **DOTween (`DG`)** | UI and game animations |
| **MessagePack** | Binary serialization (server comms) |
| **Newtonsoft.Json** | JSON fallback parsing |
| **WebSocketSharp** | WebSocket client (real-time server events) |
| **Antlr4** | Grammar parser (likely chat filter or rule expressions) |
| **TextMeshPro (`TMPro`)** | All text rendering |
| **OpenTelemetry** | Observability / tracing |
| **Xsolla** | Alternative payment gateway |
| **Google Play Games** | Android auth / achievements |

---

## Summary

The client is a well-layered Unity IL2CPP game with a clean MVVM-style UI framework (`Widget` / `ViewController` / `ScreenManager`), a lazy-load panel architecture across the HUD, a reactive server service layer over WebSocket/SignalR, and a very large feature surface (~100 distinct game systems under `Digit.Prime`).

The `Digit.Client` layer is a reusable framework that is game-agnostic; all STFC-specific logic sits in `Digit.Prime` and `Digit.PrimeServer`.

---

*Source: Decompiled from GameAssembly.dll using Il2CppInspectorRedux + PyGhidra pipeline*

---

## Search Implementation

### How Search Is Built

There is a **shared, reusable search system** built on three layers:

1. **`InputFieldWidget`** — the visible text input (wraps `TMP_InputField`). Fires `Action<string> _inputValueChangedDelegates` on every keystroke.
2. **`InputFieldContext`** — data context that binds the input field to its owning view controller.
3. **`TextSuggestionListWidget`** — a dropdown suggestion list that appears below the input field, populated in real time. Two concrete suggesters exist:
   - **`SystemNameTextSuggester`** — translates all system names off-thread (`WaitCallback`), resolves typed text to a node ID via `GetNodeIdForSystemName(string)`
   - **`ItemTextSuggester`** — suggests inventory item names

`ResearchManager` also exposes `IsSearchEnabledInSection(SectionID)` and a `_searchEnabledSections[]` array, meaning search is **gated per-section** at the manager level — not every screen activates it even if it could.

---

### Views With a Search Input

#### 1. Officer Assignment (`OfficerAssignmentViewController`)
The most feature-complete search:
- `InputFieldWidget _inputField` — live text search bar
- `InputFieldContext _inputFieldContext` — binding
- `List _searchedNonCrewOfficers` — the filtered result list (separate from `_allOfficers`, `_nonCrewOfficers`, `_sortedNonCrewOfficers`)
- `_noItemsMessageGO` / `_noItemsTextLocalizer` — "no results" empty state
- `LocaleTextContext _textSuggestionNameLocaleContext` — suggestion label
- Displayed alongside a **filter panel** (`_filtersTogglesContainer`, `_abilityFilterTogglesContainer`, `_filterAmountContainer`, `_filterButton`) — search and filters coexist

#### 2. Inventory List (`InventoryListViewController`)
- `InputFieldWidget _inputField` + `InputFieldContext _inputFieldContext`
- `List<InventoryItem> _searchResults` + `bool _searchSetupComplete`
- `virtual LocaleTextContext GetTextSuggestionNameLocaleContext()` — overridable per subclass
- Applies across all inventory tabs (items, officers, etc.)

#### 3. Officer Roster (`OfficerRosterViewController`)
- No own `_inputField` declared (inherits via list base)
- Overrides `GetTextSuggestionNameLocaleContext()` — custom suggestion label text
- Has filter button + filter amount badge alongside search

#### 4. Shop / Store (`ShopSceneManager`)
- `bool _includeBundleContentsForSearch` — when true, searches inside bundle item contents, not just bundle names
- `Dictionary<long, List<StoreSearchElement>> _bundleSearchElementsCache` — pre-built per-bundle search index
- `LocaleTextContext _textSuggestionNameLocaleContext` — suggestion prompt label
- This is a **cached index search**, not a live list filter — bundles are indexed at load time

#### 5. Ship Construction — Inventory Selection (`ShipConstructionInventoryListViewController`)
- Overrides `GetTextSuggestionNameLocaleContext()` with a ship-slot-specific label
- Inherits `InputFieldWidget` search from the list base

#### 6. Research (`ResearchManager` + sections)
- `_searchEnabledSections[]` — allowlist of `SectionID`s where search activates
- `_clearSuggestionsOnSectionExit` — clears the suggestion list when leaving
- `_researchProjectNameContext` — localised context for the suggestion display
- `IsSearchEnabledInSection(SectionID)` — queried before showing the input field at all
- The search navigates to a research node when matched

#### 7. Bookmarks (`BookmarksViewController`)
- `GenericButtonWidget _searchCoordinatesButton` — a **button**, not a free-text field
- `CoordinateSearchContext _coordinateSearchContext` — triggers a coordinate entry popup
- `BookmarkCategory _filter` + `_filteredListCount` — category filter tab alongside it
- Opens a **separate input popup** (modal) rather than an inline search bar

---

### The `InputFieldPopupViewController` Pattern

Several screens don't embed an input inline — instead they open a **modal popup**:

- **Alliance announcement** (`AllianceMainViewController`) — `_inputFieldPopupPrefab` + `InputFieldPopupViewControllerContext`
- **Game settings dual-slider** (`DualSliderOptionWidget`) — same popup pattern for entering numeric values
- **Bookmark rename** (`BookmarkDetailsViewController`) — `InputFieldWidget _nameInputField` inside the popup

The popup (`InputFieldPopupViewController`) contains:
- `InputFieldWidget _inputField`
- `TextSuggestionListWidget _textSuggestionWidget` — suggestions shown inside the modal
- `TextMeshProUGUI _inputFieldTextComponent`
- `GenericButtonWidget _confirmButton`
- Character limit display + invalid-character validation

---

### Search Summary

| View | Search Type | Suggestions | Filter Panel |
|---|---|---|---|
| **Officer Assignment** | Inline `InputFieldWidget` | `ItemTextSuggester` | Yes (type + ability filters) |
| **Inventory List** | Inline `InputFieldWidget` | `ItemTextSuggester` | No |
| **Officer Roster** | Inherited inline field | Custom label override | Yes (filter badge) |
| **Shop** | Cached `StoreSearchElement` index | Item name suggester | No |
| **Ship Construction Inventory** | Inherited inline field | Custom label override | No |
| **Research** | Section-gated inline field | `SystemNameTextSuggester` or item | No (section-level gate) |
| **Bookmarks** | Coordinate button → modal popup | `SystemNameTextSuggester` | Yes (category tabs) |
| **Alliance / Settings** | Modal `InputFieldPopupViewController` | `TextSuggestionListWidget` | No |

In all inline cases the **suggestion list drops below the input field** and is populated on each keystroke. The result list is filtered client-side from a cached collection — no server round-trip for the search itself.
