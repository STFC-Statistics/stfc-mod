# Notification Service Mod

## Overview

The notification service intercepts the game's in-game toast/banner notifications and forwards them as **native Windows desktop notifications** (toast notifications via WinRT). This allows players to receive system-level pop-ups for game events even when the game is minimized or not in focus.

**Platform:** Windows only (no-op on macOS/Linux).

## Source Files

| File | Purpose |
|---|---|
| `mods/src/patches/notification_service.h` | Public API: `notification_init()` and `notification_handle_toast()` |
| `mods/src/patches/notification_service.cc` | Core notification logic: title mapping, text localization, placeholder substitution, Windows toast delivery |
| `mods/src/patches/battle_notify_parser.h` | Public API: `battle_notify_parse()` |
| `mods/src/patches/battle_notify_parser.cc` | Battle-specific body extraction: player/enemy names, ship hull resolution, PVP detection |
| `mods/src/patches/parts/disable_banners.cc` | Hook installation: hooks `ToastObserver.EnqueueToast` and `ToastObserver.EnqueueOrCombineToast` |

## Architecture

### Hook Point

The mod hooks two methods on `ToastObserver` (namespace `Digit.Prime.HUD`, assembly `Assembly-CSharp`):

- `ToastObserver.EnqueueToast(Toast*)`
- `ToastObserver.EnqueueOrCombineToast(Toast*, uintptr_t)`

Every time the game queues a toast, `notification_handle_toast()` is called **before** the original method runs. This means notifications fire even if the toast is subsequently suppressed by the disable-banners feature.

### Initialization

`notification_init()` is called once during `InstallToastBannerHooks()`. It resolves three IL2CPP methods at runtime:

1. **`LocaleUtilities.Localize(LocaleTextContext, bool, bool)`** — static method, primary localization path. Handles parameter substitution internally.
2. **`LanguageManager.Localize(out string, LocaleTextContext)`** — instance method, fallback localization path.
3. **`System.Object.ToString()`** — used for converting IL2CPP objects to strings during placeholder substitution.

On Windows, it also initializes the WinRT single-threaded apartment for toast notifications.

## Notification Flow

### `notification_handle_toast(Toast* toast)`

1. **Config filter** — Checks if the toast's `State` enum value is in `Config::Get().notify_banner_types` (user-configurable list in TOML). If not in the list, returns early.

2. **Title resolution** — Maps the toast `State` integer to a human-readable title via `toast_state_title()`. Covers ~50 toast types:
   - Standard notifications
   - Faction events (warning, level up/down, discovered)
   - Combat (incoming attack, fleet battle, station battle, victory, defeat)
   - Armada events (created, canceled, incoming attack, battle won/lost)
   - Territory/takeover events
   - Treasury and warchest progress
   - Achievements and challenges
   - Surge events
   - Queue/lease events
   - Cross-alliance armada events
   - Dynamic crisis and galactic anomaly events

3. **Body resolution** (two strategies):
   - **Battle toasts** (Victory, Defeat, PartialVictory, StationVictory, StationDefeat, StationBattle, IncomingAttack, FleetBattle, ArmadaBattleWon, ArmadaBattleLost, AssaultVictory, AssaultDefeat) ? `battle_notify_parse()` extracts detailed battle info.
   - **All other toasts** ? `resolve_toast_text()` localizes the toast's text via the game's IL2CPP localization system.

4. **Event category prefix** — For event-based toasts (Achievement, Tournament, ChainedEventScored, TreasuryProgress, TreasuryFull, WarchestProgress, WarchestFull, FactionWeeklyEventsProgress, FactionWeeklyEventsComplete), reads `EventModel.category_` at offset `0x1E8` and prepends the category name (e.g., "Daily Goals - ...", "Battle Pass Season - ...").

5. **Rich text stripping** — `strip_unity_rich_text()` removes Unity rich text tags (`<color=#FF0000>`, `<b>`, `</size>`, etc.) from the body text.

6. **Windows delivery** — `show_system_notification()` creates a WinRT `ToastNotification` with the title and body, and shows it via `ToastNotificationManager::CreateToastNotifier("Star Trek Fleet Command")`.

## Battle Notify Parser

### `battle_notify_parse(Toast* toast)`

Only processes battle-type toast states (listed above). For all other states, returns empty string.

Extracts data from `Toast.Data` (cast to `BattleResultHeader*`) and builds a `BattleSummaryData` struct containing:

- **`playerName`** — from `BattleResultHeader.PlayerUserProfile.Name`
- **`enemyName`** — from `BattleResultHeader.EnemyUserProfile.Name`; if empty (NPC/marauder), falls back to localizing via `UserProfile.LocaId` using the key `marauder_name_only_{0}` / `navigation`
- **`playerShip`** — resolved from `BattleResultHeader.PlayerShipHullId` via `SpecService.GetHull()`
- **`enemyShip`** — resolved from `BattleResultHeader.EnemyShipHullId` via `SpecService.GetHull()`
- **`isPvp`** — detected from `BattleResultHeader.BattleType` (Fleet, Base, ArmadaBase, ArmadaAsb, ArmadaMta, PvpCuttingBeam, PvpChainShot)

### Ship Hull Name Resolution

`resolve_hull_name()` uses a multi-tier fallback:

1. **Numeric locaId** — `HullSpec.IdRefs.locaId` (offset `0x40`) ? `localize_hull_name(int64_t)` via `ship_name_{0}` / `ships`
2. **String locaStringId** — `HullSpec.IdRefs.locaStringId` (offset `0x10`) ? `localize_hull_name(string)` via `ship_name_{0}` / `ships`
3. **Manual parse** — `parse_hull_key()` converts the raw `HullSpec.Name` key (e.g., `Hull_L30_Destroyer_Klingon_LIVE` ? `Lv.30 Destroyer Klingon`)

Ship names are then normalized to Title Case via `normalize_ship_name()`.

### Body Format

```
PlayerName (PlayerShip) vs EnemyName (EnemyShip)
```

For non-PVP battles with a localized enemy name, the enemy ship class is omitted:

```
PlayerName (PlayerShip) vs Localized Enemy Name
```

## Localization

### `resolve_toast_text(Toast* toast)`

1. Gets `Toast.TextLocaleTextContext` (the LTC for the toast's text).
2. **Primary path**: Calls `LocaleUtilities.Localize(LTC, true, false)` — handles parameter substitution internally.
3. **Fallback path**: Calls `LanguageManager.Instance().Localize(out string, LTC)` — returns a template that may still have `{N}` placeholders.
4. **Placeholder substitution**: If the result contains `{N}` placeholders, `format_placeholders()` substitutes them using:
   - `Toast.TextParameters` (offset `0x18`) — first priority
   - `LocaleTextContext._textParameters` (offset `0x40`) — fallback
   - `LocaleTextContext._identifierParameters` (offset `0x38`) — last resort

### Placeholder Formatting

`format_with_array()` iterates over an `Il2CppArray*` of objects, calls `ToString()` on each (via virtual dispatch on `System.Object.ToString()`), and replaces `{0}`, `{1}`, etc. in the template.

## Safety

All IL2CPP pointer dereferences are wrapped in SEH (`__try/__except`) on Windows via the `seh_call()` template. This catches access violations from bad/stale pointers — the mod logs a warning and degrades gracefully rather than crashing the game.

Specific SEH-protected operations:
- `seh_to_string_raw()` — object to string conversion
- `build_battle_data()` — player/enemy profile access, battle type, hull IDs
- `resolve_event_category()` — reading `EventModel.category_`
- `localize()` in battle parser — creating LTC, invoking localization methods

## Configuration

### `notify_banner_types` (in TOML config)

A list of toast state integers specifying which toast types should trigger Windows notifications. Only toasts whose `State` is in this list will produce desktop notifications.

Example:
```toml
notify_banner_types = [4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100]
```

### Related config: `disabled_banner_types`

A separate list controlling which toast types are suppressed in-game (handled by `disable_banners.cc`). The notification service runs **before** the disable check, so a toast can trigger a desktop notification even if its in-game banner is disabled.

## Toast State Enum Values

The mod references toast states by their integer values. Key states (from `toast_state_title()`):

| State | Name | Title |
|---|---|---|
| 0 | Standard | Notification |
| 1 | FactionWarning | Faction Warning |
| 2 | FactionLevelUp | Faction Level Up |
| 3 | FactionLevelDown | Faction Level Down |
| 4 | FactionDiscovered | Faction Discovered |
| 5 | IncomingAttack | Incoming Attack! |
| 6 | IncomingAttackFaction | Incoming Faction Attack! |
| 7 | FleetBattle | Fleet Battle |
| 8 | StationBattle | Station Under Attack! |
| 9 | StationVictory | Station Victory! |
| 10 | Victory | Victory! |
| 11 | Defeat | Defeat |
| 12 | StationDefeat | Station Defeat |
| 13 | Tournament | Event Progress |
| 14 | ArmadaCreated | Armada Created |
| 15 | ArmadaCanceled | Armada Canceled |
| 16 | ArmadaIncomingAttack | Armada Under Attack! |
| 17 | ArmadaBattleWon | Armada Victory! |
| 18 | ArmadaBattleLost | Armada Defeated |
| 19 | DiplomacyUpdated | Diplomacy Updated |
| 20 | JoinedTakeover | Territory Capture Joined |
| 21 | CompetitorJoinedTakeover | Competitor Joined Territory |
| 22 | AbandonedTerritory | Territory Abandoned |
| 23 | TakeoverVictory | Takeover Victory! |
| 24 | TakeoverDefeat | Takeover Defeat |
| 25 | TreasuryProgress | Treasury Progress |
| 26 | TreasuryFull | Treasury Full |
| 27 | Achievement | Achievement |
| 28 | AssaultVictory | Assault Victory! |
| 29 | AssaultDefeat | Assault Defeat |
| 30 | ChallengeComplete | Challenge Complete |
| 31 | ChallengeFailed | Challenge Failed |
| 32 | StrikeHit | Strike Hit |
| 33 | StrikeDefeat | Strike Defeat |
| 34 | WarchestProgress | Warchest Progress |
| 35 | WarchestFull | Warchest Full |
| 36 | PartialVictory | Partial Victory |
| 37 | ArenaTimeLeft | Arena Time Warning |
| 38 | ChainedEventScored | Event Progress |
| 39 | FleetPresetApplied | Fleet Preset Applied |
| 40 | SurgeWarmUpEnded | Surge Started |
| 41 | SurgeHostileGroupDefeated | Surge Hostiles Defeated |
| 42 | SurgeTimeLeft | Surge Time Warning |
| 43 | QueueForLeaseActivated | Queue Activated |
| 44 | QueueForLeaseExpired | Queue Expired |
| 45 | PermanentQueuePurchased | Permanent Queue Purchased |
| 46 | OutpostStartedOrEnded | Outpost Update |
| 47 | CrossAllianceArmadaVictory | Cross-Armada Victory! |
| 48 | CrossAllianceArmadaDefeat | Cross-Armada Defeated |
| 49 | CrossAllianceArmadaPartialVictory | Cross-Armada Partial Victory |
| 50 | FactionWeeklyEventsProgress | Faction Weekly Event Progress |
| 51 | FactionWeeklyEventsComplete | Faction Weekly Event Complete |
| 52 | ArmadaPlayerBlocked | Armada Player Blocked |
| 53 | ArmadaPlayerUnblocked | Armada Player Unblocked |
| 54 | DynamicCrisisUpdate | Dynamic Crisis Update |
| 55 | DynamicCrisisFailed | Dynamic Crisis Failed |
| 56 | DynamicCrisisCompleted | Dynamic Crisis Completed |
| 57 | GalacticAnomalySystemEntered | Galactic Anomaly Entered |

## Event Categories

Event-based toasts (Achievement, Tournament, etc.) read `EventModel.category_` at offset `0x1E8`:

| Value | Category |
|---|---|
| 0 | Standard |
| 1 | Daily Goals |
| 2 | Daily Milestone |
| 3 | Leaderboard |
| 4 | Stat |
| 5 | Battle Pass Season |
| 6 | Battle Pass Event |
| 7 | Treasury Progress |
| 8 | Treasury Reward |
| 9 | Server Clash |
| 10 | Webstore Event |
| 11 | Player Lifecycle |
| 12 | Field Training |
| 13 | FT Category |
| 14 | Cutscenes |
| 15 | Minigame |
| 16 | Minigame Stage |
| 17 | Warchest |
| 18 | Alliance Game |
| 19 | Alliance Game Task |
| 20 | Meta Event |
| 21 | Meta Event Objective |
| 22 | Invasion |
| 23 | Loop Museum |
| 24 | Loop Museum Task |
| 25 | PLC BP Season |
| 26 | PLC BP Event |
| 27 | Progression Reward |
| 28 | Faction Weekly Events |

## Key IL2CPP Offsets

| Class | Field | Offset | Type |
|---|---|---|---|
| Toast | TextParameters | 0x18 | Il2CppArray* |
| Toast | Data | — | Il2CppObject* (BattleResultHeader or EventModel) |
| LocaleTextContext | _identifierParameters | 0x38 | Il2CppArray* |
| LocaleTextContext | _textParameters | 0x40 | Il2CppArray* |
| EventModel | category_ | 0x1E8 | int32 (EventCategories._flagValue) |
| HullSpec | IdRefs | 0x90 | pointer |
| IdRefs | locaStringId | 0x10 | Il2CppString* |
| IdRefs | locaId | 0x40 | int64 |
| UserProfile | Name | — | Il2CppString* |
| UserProfile | LocaId | — | int64 |

## Battle Types (PVP detection)

| Enum | Value | PVP? |
|---|---|---|
| Fleet | — | Yes |
| Base | — | Yes |
| ArmadaBase | — | Yes |
| ArmadaAsb | — | Yes |
| ArmadaMta | — | Yes |
| PvpCuttingBeam | — | Yes |
| PvpChainShot | — | Yes |
| All others | — | No |
