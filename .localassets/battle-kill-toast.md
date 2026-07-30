# Battle-Kill Toast — Data Fetching & Toast Creation

## Overview

The battle-kill toast shows in the galaxy view when a ship is killed, displaying victory/loss and which ship fought which enemy. Managed by `ToastBattleResultObserver`, which subscribes to battle events, queues results until profile data is available, then creates `Toast` objects.

## Source Files

- `output/csharp/Assembly-CSharp/Digit.Prime.HUD/ToastBattleResultObserver.cs` — C# stub signatures
- `output/isil_dump/IsilDump/Assembly-CSharp/Digit/Prime/HUD/ToastBattleResultObserver.txt` — ISIL disassembly (4256 lines)
- `output/csharp/Assembly-CSharp/Digit.Prime.HUD/Toast.cs` — Toast data container
- `output/csharp/Assembly-CSharp/Digit.Prime.HUD/ToastState.cs` — Toast state enum
- `output/csharp/Assembly-CSharp/Digit.Prime.HUD/ToastObserver.cs` — Base class with `EnqueueToast`
- `output/csharp/Assembly-CSharp/Digit.Prime.HUD/ToastLocaleSettings.cs` — Locale settings (icons, text contexts)
- `output/csharp/Digit.Client.PrimeLib.Runtime/Digit.PrimeServer.Models/IBattleResultHeader.cs` — Battle result data interface
- `output/csharp/Digit.Client.PrimeLib.Runtime/Digit.PrimeServer.Models/BattleType.cs` — Battle type enum
- `output/csharp/Digit.Client.PrimeLib.Runtime/Digit.PrimeServer.Models/BattleResultType.cs` — Battle result type enum
- `output/csharp/Digit.Client.PrimeLib.Runtime/Digit.PrimeServer.Models/UserProfile.cs` — User profile model
- `output/csharp/Digit.Client.PrimeLib.Runtime/Digit.PrimeServer.Models/FleetDataType.cs` — Fleet data type enum

## Class Structure

```csharp
public sealed class ToastBattleResultObserver : ToastObserver
{
    private ToastLocaleSettings _localeSettings;
    private Dictionary<string, Queue<IBattleResultHeader>> _observedUserProfiles;      // keyed by user ID string
    private Dictionary<long, Queue<IBattleResultHeader>> _observedFleetBattleResults;   // keyed by fleet ID
    private Queue<IBattleResultHeader> _observedStarbaseBattleResult;                   // single queue
    private Dictionary<long, Queue<IBattleResultHeader>> _observedAllianceProfiles;    // keyed by alliance ID
}
```

### Internal Queues (initialized in `.ctor`)

| Field | Key | Purpose |
|-------|-----|---------|
| `_observedUserProfiles` | User ID (string) | Holds results until enemy user profile is fetched |
| `_observedFleetBattleResults` | Fleet ID (long) | Holds results for armada fleet battles until fleet data arrives |
| `_observedStarbaseBattleResult` | N/A | Holds results for starbase battles until starbase updates |
| `_observedAllianceProfiles` | Alliance ID (long) | Holds results until enemy alliance profile is fetched |

## Event Subscriptions (`Start`)

| Event Source | Event | Handler |
|-------------|-------|---------|
| `FleetEvents` | `PlayerFleetsChangedEvent` | `PlayerFleetsChangedEventHandler(List<FleetPlayerData>)` |
| `StarbaseEvents` | `PlayerStarbaseUpdated` | `OnStarbaseUpdated(Starbase)` |
| `BattleResultEvents` | `BattleResultHeadersReceivedEvent` | `BattleResultsReceivedEventHandler(IBattleResultHeader)` |
| `UserProfileEvents` | `OnUserProfileUpdatedEvent` | `OnUserProfilesUpdated(IList<UserProfile>)` |
| `AllianceEvents` | `OnAllianceProfilesUpdatedEvent` | `OnAllianceProfilesUpdated(IList<AllianceProfile>)` |

`OnDestroy()` unsubscribes from all five events.

## Data Model: `IBattleResultHeader`

| Property | Type | Description |
|----------|------|-------------|
| `ID` | `long` | Battle result ID |
| `BattleType` | `BattleType` | Type of battle |
| `PlayerUserProfile` | `UserProfile` | Local player's profile |
| `PlayerArmadaOwnerUserProfile` | `UserProfile` | Armada owner profile |
| `PlayerFleetId` | `long` | Player's fleet ID |
| `PlayerFleetDataType` | `FleetDataType` | DEPLOYED_FLEET, STARBASE, or ARMADA |
| `PlayerFleetCaptain` | `OfficerSpec` | Player's fleet captain |
| `EnemyUserProfile` | `UserProfile` | Enemy player's profile |
| `EnemyArmadaOwnerUserProfile` | `UserProfile` | Enemy armada owner profile |
| `EnemyAllianceProfile` | `AllianceProfile` | Enemy alliance profile |
| `EnemyFleetId` | `long` | Enemy fleet ID |
| `EnemyFleetDataType` | `FleetDataType` | Enemy fleet type |
| `EnemyFleetCaptain` | `OfficerSpec` | Enemy fleet captain |
| `IsPlayerInitiator` | `bool` | Player initiated the battle |
| `IsPlayerWinner` | `bool` | Player won |
| `IsPartialWin` | `bool` | Partial victory |
| `IsPlayerStarbase` | `bool` | Player's side is a starbase |
| `IsEnemyStarbase` | `bool` | Enemy's side is a starbase |
| `IsArmadaBattle` | `bool` | Armada battle |
| `IsPlayerVersusPlayer` | `bool` | PvP battle |
| `BattleResultType` | `BattleResultType` | DEFEAT, VICTORY, or PARTIAL_VICTORY |
| `PlayerShipHullId` | `long` | Player's ship hull ID |
| `SystemId` | `long` | System where battle occurred |

### `BattleType` Enum

| Value | Name |
|-------|------|
| 0 | `FLEET` |
| 1 | `BASE` |
| 2 | `PASSIVE_MARAUDER` |
| 3 | `NPC_INSTANTIATED` |
| 4 | `DOCKING_POINT` |
| 5 | `ACTIVE_MARAUDER_MARAUDER_INITIATOR` |
| 6 | `ACTIVE_MARAUDER_PLAYER_INITIATOR` |
| 7 | `ARMADA_BASE` |
| 8 | `ARMADA_MARAUDER` |
| 9 | `PVE_DOCKING_POINT` |
| 10 | `ARMADA_ASB` |
| 11 | `ARMADA_MTA` |
| 12 | `HAZARD` |
| 13 | `PVE_CUTTING_BEAM` |
| 14 | `PVP_CUTTING_BEAM` |
| 15 | `PVE_CHAIN_SHOT` |
| 16 | `PVP_CHAIN_SHOT` |

### `BattleResultType` Enum

| Value | Name |
|-------|------|
| 0 | `DEFEAT` |
| 1 | `VICTORY` |
| 2 | `PARTIAL_VICTORY` |

### `FleetDataType` Enum

| Value | Name |
|-------|------|
| 0 | `DEPLOYED_FLEET` |
| 1 | `STARBASE` |
| 2 | `ARMADA` |

## Data Flow: Battle Result → Toast

### Step 1: `BattleResultsReceivedEventHandler(IBattleResultHeader)`

Entry point when a battle result arrives from the server.

1. **Null check** on `battleResultHeader` — if null, log error via `ErrorReporter` and return
2. **Expiry check** — call `ToastManager.IsToastExpired("Battle", header)` — if expired, skip
3. **Route based on `IsPlayerVersusPlayer`:**
   - **PvP (true):** Check `PlayerUserProfile.FetchStatus == Fetched`
     - Fetched → call `TryCreateToastForBattle(header)` directly
     - Not fetched → enqueue into `_observedUserProfiles` keyed by player user ID
   - **Not PvP (false):** Get `FleetsManager`, call `GetFleetPlayerData(playerFleetId)`
     - `CurrentState == 64` (ARMADA) → enqueue into `_observedFleetBattleResults` by fleet ID
     - Not armada → call `TryCreateToastForBattle(header)` directly

### Step 2: `TryCreateToastForBattle(IBattleResultHeader)`

Filters by `BattleType` and routes to toast creation.

| BattleType | Action |
|-----------|--------|
| 2 (PASSIVE_MARAUDER), 5, 6, 9 | Localize enemy name via `MarauderNameContext` → `CreateToastForBattle(name, header)` |
| 3 (NPC_INSTANTIATED) | Localize enemy name via `NpcNameContext` → `CreateToastForBattle(name, header)` |
| 13, 14, 15, 16 (CUTTING_BEAM, CHAIN_SHOT) | Localize enemy name via `StationDefeatContext` (offset 0xB0 / 176) → `CreateToastForBattle(name, header)` |
| 10 (ARMADA_ASB) | Get `EnemyAllianceProfile` — if not null and `IsArmadaBattle`, call `CreateToastForBattle(header, allianceProfile)`; otherwise enqueue into `_observedAllianceProfiles` by alliance ID |
| Default (0, 1, 4, 7, 8, 11, 12) | Get `EnemyUserProfile`, localize name → `CreateToastForBattle(name, header)` |

### Step 3: `CreateToastForBattle(header, UserProfile/AllianceProfile)` overloads

These handle the case where the enemy profile may not yet be fetched.

1. Check `enemyProfile.FetchStatus == Fetched` (field offset 0x94, value 1)
2. **If fetched:** call `CreateToastForBattle(enemyProfile.Name, header)` (string overload)
3. **If not fetched:** enqueue `header` into the appropriate dictionary:
   - UserProfile overload → `_observedUserProfiles` by user ID
   - AllianceProfile overload → `_observedAllianceProfiles` by alliance ID

### Step 4: `CreateToastForBattle(string enemyName, IBattleResultHeader header)`

Main toast creation method. Determines toast state, locale context, icon, and text parameters.

1. **Get `IsPlayerWinner`** (interface slot 16) → determines victory vs defeat
2. **Select locale context based on `BattleType`:**
   - BattleType 1 (BASE) → station battle context from `_localeSettings` (offset 0xB8)
   - BattleType 8 (ARMADA_MARAUDER) → armada battle context (offset 0xD8)
   - Other → fleet battle context (offset 0xA0)
3. **Determine toast state** based on `IsPlayerWinner`, `IsPlayerVersusPlayer`, `IsPartialWin`:
   - PvP Victory → `ToastState.Victory` (10) or `FleetVictory` (48) if partial
   - PvP Defeat → `ToastState.Defeat` (11) or `FleetDefeat` (49) if partial
   - PvE Victory → `ToastState.StationVictory` (48)
   - PvE Defeat → `ToastState.StationDefeat` (49)
   - Partial Victory → `ToastState.PartialVictory` (37) or `StationPartialVictory` (50)
4. **Get player ship name:**
   - If not `IsPlayerStarbase`: get `PlayerFleetId`, call `FleetsManager.GetFleetPlayerData`, extract ship name
   - If starbase: skip (no ship name needed)
5. **Create `Toast` object** with:
   - `state`: determined above
   - `textLocaleTextContext`: victory/defeat/partial context from `_localeSettings`
   - `iconIdentifier`: fleet/station/armada icon from `_localeSettings`
   - `callback`: `ToastClicked` handler
   - `data`: the `IBattleResultHeader`
   - `textParameters`: `object[1]` containing only the localized enemy name string (`TextParameters[0]`)
6. **Call `ToastObserver.EnqueueToast(toast)`**

The player ship name is **not** passed in `Toast.TextParameters`; it is read from `FleetsManager.GetFleetPlayerData` inside `CreateToastForBattle` but is not part of the `Toast` payload used by the UI binding.

### Step 5: `UpdateBattleToastState(ToastState, IBattleResultHeader)`

Adjusts toast state based on `IsPartialWin`.

- If state == Victory (10): return StationVictory (48) if partial win, else Victory (10)
- If state == Defeat (11): return StationDefeat (49) if partial win, else Defeat (11)
- Default: return StationPartialVictory (50) if partial win, else PartialVictory (37)

### Step 6: `ToastClicked(Toast battleToast)`

Handles click events on battle toasts.

1. Get `IBattleResultHeader` from `toast.Data`
2. Call `BattleResultsManager.ShowBattleReport(header)` — opens the detailed battle report

### Step 7: `IsPlayerId(string initiatorId)`

Checks if a given ID belongs to the local player.

1. Call `UserProfileManager.GetLocalUserProfile()` to get local player profile
2. Compare `initiatorId` with local player's user ID using `SpanHelpers.SequenceEqual`
3. Return true if they match

## Profile Update Handlers

These handlers drain the internal queues when profile data becomes available.

### `OnUserProfilesUpdated(IList<UserProfile> profiles)`

1. Iterate through updated profiles
2. For each profile, check if `_observedUserProfiles` dictionary has a queue for that user ID
3. If queue exists and has items:
   - Get the `IBattleResultHeader` from the queue
   - If profile is fetched: call `CreateToastForBattle(profile.Name, header)`
   - Remove the processed header from the queue
4. Also calls `Dictionary.Remove` to clean up empty queues

### `PlayerFleetsChangedEventHandler(List<FleetPlayerData> changedFleets)`

1. Iterate through changed fleets
2. For each fleet, check if `_observedFleetBattleResults` dictionary has a queue for that fleet ID
3. If queue exists and has items:
   - Dequeue `IBattleResultHeader`
   - Call `TryCreateToastForBattle(header)` to process it

### `OnStarbaseUpdated(Starbase starbase)`

1. Check if `_observedStarbaseBattleResult` queue has items
2. If yes: dequeue `IBattleResultHeader`
3. Call `TryCreateToastForBattle(header)` to process it

### `OnAllianceProfilesUpdated(IList<AllianceProfile> profiles)`

1. Iterate through updated alliance profiles
2. For each profile, check if `_observedAllianceProfiles` dictionary has a queue for that alliance ID
3. If queue exists and has items:
   - Dequeue `IBattleResultHeader`
   - Call `CreateToastForBattle(header, allianceProfile)` (AllianceProfile overload)

## Toast Object

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

    public Toast(ToastState state, LocaleTextContext textLocaleTextContext, string iconIdentifier,
                 Action<Toast> callback, GeneratedGameEvents.GameEvents showEvent,
                 object data, object[] textParameters);
}
```

## ToastObserver Base Class

```csharp
public abstract class ToastObserver : MonoBehaviour
{
    protected ToastViewController _viewController;

    protected void EnqueueToast(Toast toast);
    protected void EnqueueOrCombineToast(Toast toast, UnityAction<Toast> compareCallback);
    protected void AddFirstOrCombineToast(Toast toast, UnityAction<Toast> compareCallback = null);
    private bool AreToastsAllowed(Toast toast);
    private bool ExistsInQueue(Toast toast, UnityAction<Toast> compareCallback = null);
}
```

## Complete Flow Diagram

```
Server sends battle result
        │
        ▼
BattleResultsReceivedEventHandler(header)
        │
        ├── null? ──────────────────► ErrorReporter log, return
        │
        ├── ToastManager.IsToastExpired? ──► skip, return
        │
        ├── IsPlayerVersusPlayer?
        │   ├── YES → PlayerUserProfile fetched?
        │   │         ├── YES → TryCreateToastForBattle(header)
        │   │         └── NO  → enqueue into _observedUserProfiles[userId]
        │   │
        │   └── NO → FleetPlayerData.CurrentState == ARMADA?
        │             ├── YES → enqueue into _observedFleetBattleResults[fleetId]
        │             └── NO  → TryCreateToastForBattle(header)
        │
        ▼
TryCreateToastForBattle(header)
        │
        ├── BattleType in {2,5,6,9} ──► localize marauder name → CreateToastForBattle(name, header)
        ├── BattleType == 3 ─────────► localize NPC name → CreateToastForBattle(name, header)
        ├── BattleType in {13,14,15,16} ──► return (no toast)
        ├── BattleType == 10 ────────► EnemyAllianceProfile?
        │   ├── not null + IsArmadaBattle → CreateToastForBattle(header, allianceProfile)
        │   └── else → enqueue into _observedAllianceProfiles[allianceId]
        └── default ─────────────────► localize enemy name → CreateToastForBattle(name, header)
        │
        ▼
CreateToastForBattle(header, UserProfile/AllianceProfile)
        │
        ├── FetchStatus == Fetched? → CreateToastForBattle(profile.Name, header)
        └── not fetched → enqueue into appropriate dictionary
        │
        ▼
CreateToastForBattle(string enemyName, header)
        │
        ├── Determine toast state (Victory/Defeat/PartialVictory)
        ├── Select locale context & icon based on BattleType
        ├── Get player ship name from FleetsManager
        ├── Create Toast object
        └── EnqueueToast(toast)
        │
        ▼
Toast displayed in galaxy view
        │
        ├── User clicks toast → ToastClicked → BattleResultsManager.ShowBattleReport
        │
        ▼
Profile data arrives later?
        ├── OnUserProfilesUpdated → drain _observedUserProfiles queues
        ├── PlayerFleetsChangedEventHandler → drain _observedFleetBattleResults queues
        ├── OnStarbaseUpdated → drain _observedStarbaseBattleResult queue
        └── OnAllianceProfilesUpdated → drain _observedAllianceProfiles queues
```

---

# Deep Dive: Enemy Ship Name Fetching & Display

This section traces exactly how the enemy ship name is produced for PvE/NPC battle-kill toasts, including the `LocaleTextContext`, runtime `TextParameters`, and `UserProfileWidget` binding.

## Additional Source Files

- `output/csharp/Assembly-CSharp/Digit.Prime.HUD/BattleResultToastWidget.cs` — toast UI binding stub
- `output/csharp/Assembly-CSharp/Digit.Prime.PlayerProfile/UserProfileWidget.cs` — enemy name widget
- `output/isil_dump/IsilDump/Assembly-CSharp/Digit/Prime/HUD/BattleResultToastWidget.txt` — `OnDidBindContext` ISIL
- `output/isil_dump/IsilDump/Assembly-CSharp/Digit/Prime/HUD/ToastLocaleSettings.txt` — getter offsets
- `output/isil_dump/IsilDump/Assembly-CSharp/Digit/Prime/HUD/Toast.txt` — `Toast` field offsets
- `output/isil_dump/IsilDump/Digit.Client.PrimeLib.Runtime/Digit/PrimeServer/Models/BattleResultHeader.txt` — `get_EnemyUserProfile` ISIL
- `output/isil_dump/IsilDump/Digit.Client.PrimeLib.Runtime/Digit/PrimeServer/Services/DeploymentService.txt` — `GetFleetUserProfile` ISIL
- `output/csharp/Digit.Client.PrimeLib.Runtime/Digit.PrimeServer.Models/JournalFleetSummary.cs` — enemy fleet summary
- `output/redux/cs/il2cpp.cs` — `DeployedFleetType` and `BattleType` enums

## `ToastLocaleSettings` Context Offsets

`ToastBattleResultObserver` stores its settings in `_localeSettings` (`this + 0x40`). The following offsets are read directly by the ISIL getters:

| Field | Offset (hex) | Offset (dec) | Used for |
|-------|--------------|--------------|----------|
| `_factionNameContext` | 0x30 | 48 | Default enemy name localization |
| `_shipNameContext` | 0x68 | 104 | `_fleetName` UI text in `BattleResultToastWidget` |
| `_fleetVictoryContext` | 0x78 | 120 | (available, not directly used for PvE/NPC main text) |
| `_fleetDefeatContext` | 0x80 | 128 | (available, not directly used for PvE/NPC main text) |
| `_victoryContext` | 0x88 | 136 | `_statusText` when player wins |
| `_defeatContext` | 0x90 | 144 | `_statusText` when player loses |
| `_partialVictoryContext` | 0x98 | 152 | `_statusText` on partial win |
| `_fleetBattleIconId` | 0xA0 | 160 | Icon identifier for fleet battles |
| `_stationVictoryContext` | 0xA8 | 168 | Station victory context |
| `_stationDefeatContext` | 0xB0 | 176 | `_statusText` for cutting-beam / chain-shot types (13-16) |
| `_marauderNameContext` | 0xC0 | 192 | Enemy ship name for marauder/PvE types 2, 5, 6, 9 |
| `_npcNameContext` | 0xC8 | 200 | Enemy ship name for `NPC_INSTANTIATED` (3) |
| `_stationNameContext` | 0xD0 | 208 | `_fleetName` when `PlayerFleetDataType == STARBASE` |
| `_armadaOwnerNameContext` | 0x70 | 112 | `_fleetName` when an armada owner profile is present |

## Enemy Ship Name Localization in `TryCreateToastForBattle`

The observer does **not** pass the raw ship hull ID through localization at toast-creation time. Instead it selects a pre-configured `LocaleTextContext` from `_localeSettings` and resolves it with **no runtime parameters**:

```
Move rsi, [rax+192]   ; MarauderNameContext for types 2/5/6/9
... or ...
Move rsi, [rax+200]   ; NpcNameContext for type 3
... or ...
Move rsi, [rax+176]   ; StationDefeatContext for types 13/14/15/16

Move r9, 0
Move r8, 0
Move rdx, 0
Move rcx, rsi
Call LocaleUtilities.Localize
```

`LocaleUtilities.Localize(context, null, null)` resolves the string using only the values baked into the `LocaleTextContext` asset. The returned string is the localized enemy ship name (e.g., `"HOSTILE INTERCEPTOR"`).

That string is then passed as the `enemyName` argument to `CreateToastForBattle(string enemyName, IBattleResultHeader header)`.

## `CreateToastForBattle` → `Toast` Parameter Mapping

Inside `CreateToastForBattle(string enemyName, header)`:

1. Allocates `object[1]`.
2. Sets index `0` to the localized `enemyName` string.
3. Calls the `Toast` constructor with the array as `textParameters`.

`Toast` constructor signature and field layout:

```csharp
public Toast(
    ToastState state,                          // rdx
    LocaleTextContext textLocaleTextContext,     // r8
    string iconIdentifier,                       // r9
    Action<Toast> callback,                      // stack 0x68
    GeneratedGameEvents.GameEvents showEvent,    // stack 0x50
    object data,                                 // stack 0x60
    object[] textParameters)                     // stack 0x58
```

| Toast Field | Offset (hex) | Offset (dec) | Constructor Arg |
|-------------|--------------|--------------|-----------------|
| `State` | 0x10 | 16 | `state` |
| `TextParameters` | 0x18 | 24 | `textParameters` (`object[1] { enemyName }`) |
| `TextLocaleTextContext` | 0x20 | 32 | `textLocaleTextContext` (`_victoryContext`, `_defeatContext`, or `_partialVictoryContext`) |
| `SecondaryTextLocaleTextContext` | 0x28 | 40 | (not set by this constructor) |
| `IconIdentifier` | 0x30 | 48 | `iconIdentifier` |
| `Data` | 0x38 | 56 | `data` (`IBattleResultHeader`) |

So for every PvE/NPC toast:

- **`Toast.TextParameters[0]` = localized enemy ship name string.**
- **`Toast.TextLocaleTextContext` = victory/defeat/partial context** (used by `_statusText`).
- **`Toast.Data` = the original `IBattleResultHeader`.**

## UI Binding: `BattleResultToastWidget.OnDidBindContext`

`BattleResultToastWidget` (`Widget<Toast>`) reads the bound `Toast` and its embedded `IBattleResultHeader`, then binds four key UI elements:

| Widget Field | Type | Bound Value |
|--------------|------|-------------|
| `_statusText` | `TextLocalizer` | `Toast.TextLocaleTextContext` + `Toast.TextParameters[0]` |
| `_fleetName` | `TextLocalizer` | `ToastLocaleSettings._shipNameContext` + `Toast.TextParameters[0]` (normal fleet) |
| `_enemyName` | `UserProfileWidget` | `IBattleResultHeader.EnemyUserProfile` |
| `_allianceName` | `AllianceProfileWidget` | `IBattleResultHeader.EnemyAllianceProfile` |

### `_statusText`

Uses the `Toast.TextLocaleTextContext` (victory/defeat/partial) and substitutes `{0}` with `Toast.TextParameters[0]`, producing the full status line (e.g., `"VICTORY: HOSTILE INTERCEPTOR"`).

### `_fleetName` (enemy ship name display)

For a normal fleet (`PlayerFleetDataType != STARBASE` and no armada owner):

```
Move r8, [rbx+80]      ; Toast
Move r8, [r8+24]       ; Toast.TextParameters
Compare [r8+24], rax   ; length check
Move r8, [r8+32]       ; TextParameters[0]
...
Move rdx, [rbx+168]    ; _toastLocaleSettings
Move rdx, [rdx+104]    ; _shipNameContext
```

So `_fleetName` is set from **`_shipNameContext`** using the same localized enemy ship name that is already in `Toast.TextParameters[0]`.

### `_enemyName` (`UserProfileWidget` binding)

```
Move rdx, typeof(IBattleResultHeader)
Move rcx, 9            ; EnemyUserProfile interface slot
Move rdi, [rbx+136]    ; _enemyName (UserProfileWidget)
Move r8, rbp           ; IBattleResultHeader instance
Call 0x180003830       ; get_EnemyUserProfile
...
Move r10, [rdi]        ; _enemyName vtable
Move r8, rax           ; EnemyUserProfile result
Move rdx, 0
Move rcx, rdi
Move r9, [r10+880]     ; SetContext / SetUserProfile
Call 0
...
Move rax, [rcx]
Move rdx, [rax+912]    ; UpdateDisplay / Refresh
Call 0
```

Confirmation: `_enemyName` receives the `UserProfile` returned by `IBattleResultHeader.EnemyUserProfile`, via the widget’s context-setting virtual method, followed by a refresh/display update call.

## `EnemyUserProfile` Construction for NPCs

`BattleResultHeader.get_EnemyUserProfile` does not return a plain serialized field for NPC battles. It builds the profile on demand by calling `DeploymentService.GetFleetUserProfile`:

```
Move rax, [rbx+48]     ; _enemyFleetSummary (JournalFleetSummary)
Move r9, [rax+30h]     ; FleetRefIds
Move r8, [rax+10h]     ; UserId
Move edx, [rax+28h]    ; DeployedFleetType
Call DeploymentService.GetFleetUserProfile
```

`DeploymentService.GetFleetUserProfile(DeployedFleetType type, string userID, IdRefs refIds)`:

- Looks up the fleet/user in the deployment service.
- Switches on `DeployedFleetType` (Player=1, Marauder=2, NpcInstantiated=3, Sentinel=4, Alliance=5, Challenge=6).
- Returns a `UserProfile` whose `Name` is the NPC identifier string (e.g., `"Rigellian Destroyer"`).
- For PvE/NPC cases the type is `NpcInstantiated` (3) or `Marauder` (2).

The input data comes from `JournalFleetSummary`:

| Property | Meaning |
|----------|---------|
| `UserId` | NPC identifier / commander name string |
| `FleetId` | Enemy fleet ID |
| `DeployedFleetType` | `Player`, `Marauder`, `NpcInstantiated`, etc. |
| `FleetDataType` | `DEPLOYED_FLEET`, `STARBASE`, or `ARMADA` |
| `FleetRefIds` | `IdRefs` for the fleet |
| `CaptainId` | Fleet captain ID |
| `HullIds` | List of ship hull IDs |
| `FinalShipHps` | Final HP values |

## Correlation with Battle-Log CSV

| CSV enemy field | Code source |
|-----------------|-------------|
| `"HOSTILE INTERCEPTOR"` (ship name) | Localized from `ToastLocaleSettings.MarauderNameContext` / `NpcNameContext`, stored in `Toast.TextParameters[0]`, and rendered through `_shipNameContext` in `_fleetName`. |
| `"Rigellian Destroyer"` (commander name) | `JournalFleetSummary.UserId` → `DeploymentService.GetFleetUserProfile` → `UserProfile.Name` → bound to `_enemyName` (`UserProfileWidget`). |

## Quick Reference: Data Flow

```
Server sends IBattleResultHeader
        │
        ▼
JournalFleetSummary (EnemyFleetSummary)
        │
        ├── UserId ────────► DeploymentService.GetFleetUserProfile
        │                      └── UserProfile.Name
        │                              └── _enemyName (UserProfileWidget)
        │
        └── BattleType ────► ToastBattleResultObserver.TryCreateToastForBattle
                               ├── type 2/5/6/9  → MarauderNameContext
                               ├── type 3        → NpcNameContext
                               └── type 13-16    → StationDefeatContext
                                       │
                                       ▼
                              LocaleUtilities.Localize(context, null, null)
                                       │
                                       ▼
                              CreateToastForBattle(enemyName, header)
                                       │
                                       ▼
                              Toast.TextParameters = object[1] { enemyName }
                              Toast.TextLocaleTextContext = victory/defeat/partial context
                              Toast.Data = IBattleResultHeader
                                       │
                                       ▼
                              BattleResultToastWidget.OnDidBindContext
                                       │
                                       ├── _statusText  ← TextLocaleTextContext + TextParameters[0]
                                       ├── _fleetName   ← _shipNameContext + TextParameters[0]
                                       └── _enemyName   ← EnemyUserProfile
```
## Mod Implementation: Enemy Name Localization

### Problem

The mod's battle notification parser (`battle_notify_parser.cc`) was showing generic hull classifications (e.g., "HOSTILE INTERCEPTOR") instead of the actual NPC ship name (e.g., "Rigellian Destroyer") for hostile NPC battles. The `UserProfile.Name` field is empty for NPC/marauder profiles, so the mod had no enemy name to display.

### Root Cause

The game's `DeploymentService.GetFleetUserProfile` constructs `UserProfile` objects for NPC battles with an empty `Name` field. The actual NPC name is stored as a numeric `LocaId` (int64) on the `UserProfile`, which must be localized through the game's `LocaleTextContext` system.

### Solution

The game's `PlayerProfileLocaleSettings` contains a `_marauderNameContext` field (offset 0x68) with:
- **Identifier pattern**: `marauder_name_only_{0}`
- **Category**: `navigation`

The `{0}` placeholder is substituted with the enemy `UserProfile.LocaId` (boxed as int64). This produces clean names like "Rigellian Destroyer" without extra placeholders.

There is also a related key `marauder_name_{0}` (without `_only_`) in the same category, but it produces "Rigellian Destroyer ({0})" with a leftover `{0}` placeholder — not suitable for display.

### Implementation Details

**File**: `mods/src/patches/battle_notify_parser.cc`

1. **`localize_enemy_name(int64_t locaId)`** — New function that:
   - Creates a `LocaleTextContext` with identifier `"marauder_name_only_{0}""` and category `"navigation"`
   - Boxes the `locaId` as an int64 and passes it via `ApplyIdentifierParameters`
   - Calls `LocaleUtilities.Localize(ltc, false, false)` to get the localized string
   - Returns the result (e.g., "Rigellian Destroyer")

2. **Enemy name extraction in `build_battle_data`** — When `UserProfile.Name` is empty:
   - Reads `UserProfile.LocaId` (int64 at offset 0x10)
   - Calls `localize_enemy_name(locaId)`
   - Filters out "Unknown" results (returned when locaId has no translation)
   - Sets `result.enemyName` to the localized name

3. **Ship name normalization** — `normalize_ship_name()` converts ALL CAPS hull names to Title Case:
   - "ORION CORVETTE" → "Orion Corvette"
   - "HOSTILE INTERCEPTOR" → "Hostile Interceptor"
   - Handles spaces, hyphens, and underscores as word boundaries

4. **Non-PVP enemy ship suppression** — `BattleSummaryData.isPvp` flag:
   - Set based on `BattleType` enum: `Fleet`, `Base`, `ArmadaBase`, `ArmadaAsb`, `ArmadaMta`, `PvpCuttingBeam`, `PvpChainShot` are PVP
   - All other types (PassiveMarauder, NpcInstantiated, ActiveMarauder, ArmadaMarauder, DockingPoint, Hazard, PveCuttingBeam, PveChainShot) are non-PVP
   - For non-PVP battles with a localized enemy name, `format_body()` drops the `(ShipClass)` suffix from the enemy side
   - PVP battles retain the full "Name (Ship) vs Name (Ship)" format

### Runtime Diagnostic Findings

From runtime logs (BattleType=2, PassiveMarauder, enemy = Rigellian Destroyer):

| Field | Value |
|-------|-------|
| `BattleType` | 2 (PassiveMarauder) |
| `EnemyUserProfile.LocaId` | 10005 |
| `EnemyUserProfile.Name` | "" (empty) |
| `EnemyUserProfile.IsNPC` | false |
| `EnemyUserProfile.IdRefs.locaId_` | 10005 |
| `EnemyUserProfile.IdRefs.locaStringId` | "" (empty) |
| `JournalFleetSummary.UserId` | "mar_1" |
| `Toast.TextParameters[0]` | Int64, value=0 (not the enemy name) |
| `Toast.TextLocaleTextContext` | identifier="battle_report_epicwin" category="battles" |
| `PPLRS._marauderNameContext` | identifier="marauder_name_only_{0}" category="navigation" |
| `PPLRS._npcNameContext` | identifier="title_{0}" category="entity" |

**Localization results** (from diagnostic brute-force attempts):

| Pattern | Category | Parameter | Result |
|---------|----------|-----------|--------|
| `marauder_name_only_{0}` | `navigation` | LocaId=10005 (int64) | **"Rigellian Destroyer"** |
| `marauder_name_{0}` | `navigation` | LocaId=10005 (int64) | "Rigellian Destroyer ({0})" (has leftover placeholder) |
| `title_{0}` | `entity` | LocaId=10005 (int64) | "Retrieving..." (not useful) |
| `marauder_name_only_{0}` | `navigation` | Toast.TextParameters[0]=0 | "Unknown" (wrong param) |
| `marauder_name_only_{0}` | `navigation` | UserId="mar_1" (string) | "" (empty) |

### Key Localization Keys (from game translation data)

```json
{ "key": "marauder_name_only_10005", "text": "Rigellian Destroyer" }
{ "key": "marauder_name_10005", "text": "Rigellian Destroyer ({0})" }
```

### ToastLocaleSettings Context Offsets (validated at runtime)

| Field | Offset | LTC Identifier | LTC Category |
|-------|--------|----------------|--------------|
| `_shipNameContext` | 0x68 | (varies) | (varies) |
| `_armadaOwnerNameContext` | 0x70 | (varies) | (varies) |
| `_marauderNameContext` | 0xC0 | (varies) | (varies) |
| `_npcNameContext` | 0xC8 | (varies) | (varies) |
| `_stationNameContext` | 0xD0 | (varies) | (varies) |

Note: At runtime, `FindObjectsOfTypeAll(ToastLocaleSettings)` returned 0 instances (not yet loaded or not serialized as ScriptableObjects). `PlayerProfileLocaleSettings` returned 8 instances, all with identical context values.

### PlayerProfileLocaleSettings Context Offsets (validated at runtime)

| Field | Offset | LTC Identifier | LTC Category |
|-------|--------|----------------|--------------|
| `_marauderNameContext` | 0x68 | `marauder_name_only_{0}` | `navigation` |
| `_npcNameContext` | 0x70 | `title_{0}` | `entity` |

### Example Output

**Before** (non-PVP marauder battle):
```
ChronoX725 (ORION CORVETTE) vs  (HOSTILE INTERCEPTOR)
```

**After**:
```
ChronoX725 (Orion Corvette) vs Rigellian Destroyer
```

**PVP battle** (unchanged format):
```
ChronoX725 (Orion Corvette) vs EnemyPlayer (Brel Class)
```

### BattleType Enum Reference

| Value | Name | PVP? | Description |
|-------|------|------|-------------|
| 0 | Fleet | Yes | Player vs player fleet |
| 1 | Base | Yes | Player vs player starbase |
| 2 | PassiveMarauder | No | Player vs passive hostile NPC |
| 3 | NpcInstantiated | No | Player vs instantiated NPC |
| 4 | DockingPoint | No | Player vs docking point |
| 5 | ActiveMarauder_MarauderInit | No | Player vs active marauder (marauder-initiated) |
| 6 | ActiveMarauder_PlayerInit | No | Player vs active marauder (player-initiated) |
| 7 | ArmadaBase | Yes | Armada vs player starbase |
| 8 | ArmadaMarauder | No | Armada vs marauder |
| 9 | PveDockingPoint | No | PvE docking point |
| 10 | ArmadaAsb | Yes | Armada vs alliance starbase |
| 11 | ArmadaMta | Yes | Armada vs main target array |
| 12 | Hazard | No | Hazard system battle |
| 13 | PveCuttingBeam | No | PvE cutting beam |
| 14 | PvpCuttingBeam | Yes | PvP cutting beam |
| 15 | PveChainShot | No | PvE chain shot |
| 16 | PvpChainShot | Yes | PvP chain shot |
