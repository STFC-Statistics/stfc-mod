# Ctrl+F Search Focus Shortcut — Implementation Guide

> Reference document for adding a keyboard shortcut to focus the search input field.

---

## Existing Infrastructure to Leverage

The game already has a complete, first-class keyboard shortcut system built on Unity's **new Input System**. No new patterns need to be invented — this is a plug-in to the existing architecture.

| Class | Location | Role |
|---|---|---|
| `ShortcutsManager` | `Digit.Prime.GameInput.ShortcutsManager` | Central registry. Holds `InputActionAsset _actions`, `Dictionary<Guid, InputAction> _supportedActions`, `Dictionary<KeyValuePair<InputAction, int>, SafeAction> _keybindOverrideEvents` |
| `InputBindingModificationPopupViewController` | `Digit.Prime.GameInput` | In-game rebinding UI — handles `RebindingOperation`, old/new keybind display, confirm/cancel |
| `ShortcutKeybindHint` | Used in `ActionElementWidget`, `FleetLocalViewController` | Widget that renders the bound key visually on-screen |
| `InputActionReference[]` | Used in `MissionManager`, `MissionContext`, `AllianceGamesSectionContext` | How individual views declare which shortcuts they respond to |
| `InputFieldWidget._input` | `Digit.Client.UI.InputFieldWidget` (field offset `0x80`) | The `TMP_InputField` that needs to receive `ActivateInputField()` |

---

## Implementation Steps

### Step 1 — Register the Action in `ShortcutsManager`

Add a new `InputAction` with the binding `<Keyboard>/ctrl+f` into the `InputActionAsset _actions`. In a runtime/mod context this is done programmatically:

```csharp
var searchAction = shortcutsManager._actions
    .AddActionMap("Search")
    .AddAction("FocusSearch", binding: "<Keyboard>/ctrl+f");

searchAction.Enable();
shortcutsManager._supportedActions[searchAction.id] = searchAction;
```

Then register a `SafeAction` callback entry in `_keybindOverrideEvents` for each view controller that hosts a search field.

---

### Step 2 — Subscribe Per View on Section Enter / Unsubscribe on Leave

Each search-capable view already follows a section lifecycle pattern (`OnEnterSection` / `OnLeaveSection`). Hook into this to subscribe and unsubscribe the shortcut:

```csharp
// On section enter
searchAction.performed += OnFocusSearchShortcut;

// On section leave
searchAction.performed -= OnFocusSearchShortcut;

// Handler
private void OnFocusSearchShortcut(InputAction.CallbackContext ctx)
{
    _inputField._input.ActivateInputField();
}
```

#### Views that need this subscription

| View Controller | Field to focus | Notes |
|---|---|---|
| `OfficerAssignmentViewController` | `_inputField` (offset `0x168`) | Most feature-complete search; coexists with filter panel |
| `InventoryListViewController` | `_inputField` (offset `0xD8`) | Covers all inventory tabs |
| `OfficerRosterViewController` | Inherited `_inputField` | Inherits from list base |
| `ShipConstructionInventoryListViewController` | Inherited `_inputField` | Inherits from list base |
| `ResearchManager` sections | Section-gated — use `IsSearchEnabledInSection(SectionID)` first | Already has `_searchEnabledSections[]` allowlist |

> **Note:** The Shop (`ShopSceneManager`) uses a pre-built `StoreSearchElement` cache index rather than a live `InputFieldWidget`, so Ctrl+F focus does not apply there in the same way.

---

### Step 3 — Display the Key Hint via `ShortcutKeybindHint`

Attach a `ShortcutKeybindHint` component to the `InputFieldWidget` GameObject and pass it an `InputActionReference` pointing to the new action. The existing infrastructure will render the `Ctrl+F` label on-screen automatically, consistent with all other shortcut hints in the game.

---

## Critical Guard: Chat Input Conflict

`ChatMessageListLocalViewController` has a `TMP_InputField _inputField` (offset `0x60`) that is always present on the HUD overlay. It already tracks `bool _inputFieldSelected` (offset `0xC0`).

**Before activating the shortcut, always check this flag:**

```csharp
private void OnFocusSearchShortcut(InputAction.CallbackContext ctx)
{
    if (chatMessageList._inputFieldSelected)
        return; // player is typing in chat — do not steal focus

    _inputField._input.ActivateInputField();
}
```

The existing `BackButtonSemaphore` pattern (used for back-button conflict resolution across overlapping screens) is the correct model here: each view registers and deregisters its shortcut claim when it enters and leaves focus, so only the topmost active view handles the key.

---

## Architecture Flow Summary

```
InputActionAsset  (ShortcutsManager._actions)
    └─ InputActionMap "Search"
            └─ InputAction "FocusSearch"  <Keyboard>/ctrl+f
                    │
                    ├─ [Rebindable via InputBindingModificationPopupViewController]
                    ├─ [Hint displayed via ShortcutKeybindHint on InputFieldWidget]
                    │
                    ├─ OfficerAssignmentViewController.OnEnterSection  → subscribe
                    │       performed → guard chat → _inputField._input.ActivateInputField()
                    │   OfficerAssignmentViewController.OnLeaveSection → unsubscribe
                    │
                    ├─ InventoryListViewController.OnEnterSection       → subscribe
                    │       performed → guard chat → _inputField._input.ActivateInputField()
                    │   InventoryListViewController.OnLeaveSection      → unsubscribe
                    │
                    └─ ResearchManager sections
                            performed → IsSearchEnabledInSection(currentSection) check
                                      → guard chat
                                      → _inputField._input.ActivateInputField()
```

---

## Key Field Offsets (for hooking / patching reference)

| Class | Field | Offset | Type |
|---|---|---|---|
| `ShortcutsManager` | `_actions` | `0xF0` | `InputActionAsset` |
| `ShortcutsManager` | `_supportedActions` | `0x108` | `Dictionary<Guid, InputAction>` |
| `ShortcutsManager` | `_keybindOverrideEvents` | `0x110` | `Dictionary<KeyValuePair<InputAction,int>, SafeAction>` |
| `InputFieldWidget` | `_input` | `0x80` | `TMP_InputField` |
| `InputFieldWidget` | `_inputValueChangedDelegates` | `0x78` | `Action<string>` |
| `OfficerAssignmentViewController` | `_inputField` | `0x168` | `InputFieldWidget` |
| `OfficerAssignmentViewController` | `_inputFieldContext` | `0x190` | `InputFieldContext` |
| `InventoryListViewController` | `_inputField` | `0xD8` | `InputFieldWidget` |
| `InventoryListViewController` | `_inputFieldContext` | `0xE0` | `InputFieldContext` |
| `ChatMessageListLocalViewController` | `_inputField` | `0x60` | `TMP_InputField` |
| `ChatMessageListLocalViewController` | `_inputFieldSelected` | `0xC0` | `bool` |

---

*Source: Decompiled from GameAssembly.dll using Il2CppInspectorRedux + PyGhidra pipeline*

---

## Findings from `stfc-client` Decompiled Analysis

### 1. Accessing `ShortcutsManager`

`ShortcutsManager` inherits from `MonoSingleton<ShortcutsManager>` (`public class ShortcutsManager : MonoSingleton<ShortcutsManager>`).

- `MonoSingleton<T>` has a static property `Instance`.
- Therefore, `ShortcutsManager` is reachable via `ShortcutsManager.Instance` (the `get_Instance` property on `MonoSingleton<ShortcutsManager>`).

### 2. Input System classes and extension methods

The Unity Input System classes are in `UnityEngine.InputSystem`:

- `InputActionAsset` (ScriptableObject): has `m_ActionMaps` array (field), `FindActionMap`, `AddActionMap`, `Enable`, `Disable`, `FindAction` methods.
- `InputActionMap` (sealed): has `AddAction` static extension method in `InputActionSetupExtensions`, plus `m_Actions`, `m_Bindings`, `FindAction`, `Enable`, `Disable` methods.
- `InputAction` (sealed): has `add_performed(Action<CallbackContext>)`, `remove_performed(Action<CallbackContext>)`, `Enable()`, `Disable()` methods.
- `InputActionSetupExtensions` static class has the extension methods `AddAction(InputActionMap, string, InputActionType, string binding, ...)` and `AddActionMap(InputActionAsset, string)`.

**Important:** `InputActionSetupExtensions` is a static class. In IL2CPP C++, these methods are invoked as normal static methods with the `InputActionMap`/`InputActionAsset` as the first argument, not as instance methods.

### 3. Section lifecycle events

The `SectionManager` class has public events:

- `public event EnterSectionDelegateSignature OnEnterSection`
- `public event LeaveSectionDelegateSignature OnLeaveSection`

Delegate signatures:

```csharp
deleter SectionManager.CheckFeedback EnterSectionDelegateSignature(SectionStatus status, SectionEnterPhases phase, SectionStorage storage);
deleter SectionManager.CheckFeedback LeaveSectionDelegateSignature(SectionStatus status, SectionLeavePhases phase, SectionStorage storage);
```

This is the actual mechanism for "OnEnterSection / OnLeaveSection" subscriptions. There are **no** `OnEnterSection`/`OnLeaveSection` methods directly on `ViewController` or `InventoryListViewController`; the lifecycle comes through `SectionManager` events.

`SectionManager` is accessible via `Hub::get_SectionManager()` (already known in the codebase).

### 4. ViewController lifecycle methods

`ViewController<T>` has `protected virtual`:

- `AboutToShow()`
- `AboutToHide()`
- `OnDidBindCanvasContext()`
- `OnAboutToReleaseCanvasContext()`

These are the per-view hooks, not section-based. `OfficerAssignmentViewController` and `InventoryListViewController` override some of them.

### 5. InputFieldWidget field offsets

From `InputFieldWidget.cs`:

- `private TMP_InputField _input` (field)
- `public TMP_InputField input` (property)
- `private Action<string> _inputValueChangedDelegates` (field)
- `public void Focus()`
- `public void Clear()`
- `public void Enable()`
- `public void Disable()`

The document's `0x80` offset for `_input` is the field offset, not the property. In IL2CPP, the property `get_input` is a method; the field can be read directly at offset `0x80` if the layout is stable.

### 6. Chat conflict guard

`ChatMessageListLocalViewController` (confirmed):

- `private TMP_InputField _inputField` (offset `0x60`)
- `private bool _inputFieldSelected` (offset `0xC0`)

The `_inputFieldSelected` flag is set/cleared in `OnSelect`/`OnDeselect` callbacks of the chat input.

### 7. Key controllers / search fields

- `OfficerAssignmentViewController` (namespace `Digit.Prime.OfficerAssignment`)
  - `_inputField` at offset `0x168` (confirmed as `InputFieldWidget`)
  - This is the officer assignment/selection screen (not the same as the roster inventory).

- `InventoryListViewController` (namespace `Digit.Prime.Inventories`)
  - `_inputField` at offset `0xD8` (confirmed as `InputFieldWidget`)
  - `OfficerRosterViewController` inherits from it and reuses the same field.

### 8. `TMP_InputField` methods

`TMP_InputField` inherits `Selectable`, so:

- `public void Select()` (from `UnityEngine.UI.Selectable`)
- `public void ActivateInputField()` (from `TMP_InputField`)

Both are non-virtual instance methods taking no arguments and returning `void`.

### 9. Practical implementation path

Based on the decompiled code, the correct mod-level approach is:

1. **Register the action once** after `ShortcutsManager` is initialized:
   - Get `ShortcutsManager.Instance`.
   - Read `_actions` (InputActionAsset).
   - Call `InputActionSetupExtensions.AddActionMap(_actions, "Search")` to get an `InputActionMap`.
   - Call `InputActionSetupExtensions.AddAction(searchMap, "FocusSearch", binding: "<Keyboard>/ctrl+f")` to get an `InputAction`.
   - Call `searchAction.Enable()`.
   - Store the `InputAction` pointer globally.

2. **Subscribe to the action globally** with a callback that checks the current context and focuses the appropriate input.

3. **Do not rely on per-view OnEnterSection/OnLeaveSection methods** because they do not exist on `ViewController`. Instead, use `SectionManager.OnEnterSection` / `SectionManager.OnLeaveSection` events to know which section is active, or use `SectionManager.CurrentSection` in the global callback to decide which input field to focus.

4. **In the callback**:
   - Guard `ChatMessageListLocalViewController._inputFieldSelected`.
   - Determine the current active section (e.g. officer inventory, officer assignment, research).
   - Use `ObjectFinder<OfficerAssignmentViewController>` / `ObjectFinder<InventoryListViewController>` / `ObjectFinder<OfficerRosterViewController>` to find the controller instance.
   - Read `_inputField` from the controller.
   - Call `_inputField._input.ActivateInputField()` (or `get_input().ActivateInputField()`).

5. **Important caveat**: The `OnEnterSection`/`OnLeaveSection` events are `SectionManager` events, not per-view. Subscribing/unsubscribing to them requires adding/removing IL2CPP delegates. The callback signatures include `SectionStatus`, `SectionEnterPhases`, `SectionStorage` and return `CheckFeedback`.

### 10. Open implementation questions

- `InputActionSetupExtensions` static methods are extension methods; the exact C++ IL2CPP call convention is static with the target as the first argument.
- `InputAction.add_performed` takes an `Action<CallbackContext>`. Creating an `Action<CallbackContext>` delegate in native code requires constructing a `MulticastDelegate` (or `Action`) object and passing it to `add_performed`.
- `InputAction` is a `sealed class` and `InputActionMap` is `sealed`; the `AddAction` extension method returns `InputAction` and adds it to the map's `m_Actions` array.
- The `InputActionAsset` asset is loaded from `ShortcutsManager._actions` at runtime. It may be `null` before `ShortcutsManager.Initialize` completes.
- Persistent storage: `ShortcutsManager` already persists and loads bindings via `SaveBindings`/`LoadBindings`. Adding a new action to the asset may or may not persist across reloads depending on whether the asset is re-created. If the asset is a ScriptableObject loaded from the game bundle, adding actions at runtime should persist for the session but may be lost on game restart.

## Final Implementation (implemented in this repo)

The native `ShortcutsManager`/`InputAction` route was avoided because the mod already has a working global hotkey system tied to `ScreenManager.Update` via `MapKey::IsDown`. The implemented solution reuses that infrastructure.

### Files changed

- `mods/src/patches/parts/focus_search.cc` — new patch part containing `FocusSearchBox()`.
- `mods/src/patches/parts/focus_search.h` — declaration.
- `mods/src/patches/parts/hotkeys.cc` — calls `FocusSearchBox()` from `ScreenManager_Update_Hook` when `GameFunction::FocusSearch` is pressed.
- `mods/src/patches/patches.cc` — registers `InstallFocusSearchHooks`.
- `mods/src/patches/gamefunctions.h` — adds `FocusSearch` enum value.
- `mods/src/config.h`, `mods/src/config.cc`, `mods/src/defaultconfig.h` — adds `installFocusSearchHooks` toggle and `focus_search = "CTRL-F"` shortcut.
- `example_community_patch_settings.toml` — example entries under `[patches]` and `[shortcuts]`.
- `mods/src/prime/CanvasController.h` — adds `GetCanvasControllerFromComponent()` helper.
- `mods/src/prime/InputFieldWidget.h`, `AssignShipsWidget.h`, `InventoryListViewController.h`, `OfficerAssignmentViewController.h` — bindings for the input widgets and controllers.
- `mods/src/patches/parts/object_tracker.cc` — tracks the new controller/widget classes so `ObjectFinder<>` can find them.

### How it works

1. **Hotkey wiring:** `ScreenManager_Update_Hook` checks `MapKey::IsDown(GameFunction::FocusSearch)` each frame. The binding is parsed from `focus_search` in the config (default `CTRL-F`) and is only active when the chat input is not focused.
2. **Patch entry:** `FocusSearchBox()` first checks `Config::Get().installFocusSearchHooks`. If false it returns immediately.
3. **Chat guard:** `ObjectFinder<ChatMessageListLocalViewController>` is used to read `_inputFieldSelected`. If the chat text box is selected, the search focus is skipped.
4. **Widget discovery:** The function scans, in order:
   - `InventoryListViewController` (covers `OfficerRosterViewController` and other inventory subclasses)
   - `OfficerAssignmentViewController`
   - `AssignShipsWidget`
5. **Visibility/activity checks:** For each candidate it checks `canvasController->Visible()` and `isActiveAndEnabled` on both the controller/widget and its `_inputField`.
6. **Focus:** The first visible, active input has `_inputField->Focus()` called. `InputFieldWidget.Focus()` calls the managed `Focus()` method and, as a fallback, `TMP_InputField.ActivateInputField()`.

### Production cleanup

- All `[FocusSearch]` `spdlog::info` notifications are wrapped in `#ifdef _MODDBG` so they do not appear in release builds.
- Candidate-scan `spdlog::debug` logs are filtered by the default release log level (`info`).
- Runtime exceptions in `GetCanvasControllerFromComponent()` are caught and returned as `nullptr`.
