# Officer Inventory Ctrl+F Focus Investigation

## Objective
Implement a `Ctrl+F` hotkey that focuses the visible search box in the Officer Inventory view.

## Controller / UI Structure

The Officer Inventory is implemented through a small hierarchy of classes/components:

- `InventoryListViewController` (base)
  - Located in `Assembly-CSharp`, `Digit.Prime.Inventory` (inferred from usage).
  - Holds an `InputFieldWidget` at instance offset `0xD8`.
- `OfficerRosterViewController` : `InventoryListViewController`
  - The concrete controller for the Officer Inventory view.
  - Adds filter-related serialize fields and officer data.
- `InputFieldWidget` (`Digit.Client.UI`)
  - Wrapper around one or more `TMP_InputField` components.
  - Has at least `_input` (via `get_input`) and `_submitInput`.
- `TMP_InputField` (`TMPro` / `Unity.TextMeshPro`)
  - The actual TextMeshPro input component the user clicks and types into.
- Object tracking (`ObjectFinder<T>`) is used to enumerate live controller instances at runtime.

Relevant Unity APIs used during the investigation:

- `UnityEngine.Component.get_gameObject()` (managed method, not an icall).
- `UnityEngine.Component.get_transform()` (managed method, not an icall).
- `UnityEngine.Transform.get_parent()` (managed method).
- `UnityEngine.GameObject.GetComponentsInternal(Type, bool, bool, bool, bool, object)` (private, but resolvable).
- `UnityEngine.Behaviour.get_isActiveAndEnabled()` (icall).
- `UnityEngine.GameObject.get_activeInHierarchy()` (icall).
- `UnityEngine.UI.Selectable.Select()` (managed method).
- `TMPro.TMP_InputField.ActivateInputField()` (managed method).
- `Digit.Client.UI.InputFieldWidget.Focus()` (managed method).

## What Was Attempted

### 1. Direct programmatic focus on `InputFieldWidget._input`
- Call `InputFieldWidget.Focus()` and `Selectable.Select()` / `ActivateInputField()` on the widget's `_input` field.
- **Result:** The target pointer was not the same as the one receiving manual clicks. The search box did not visibly focus.

### 2. Diagnostic hooks on `TMP_InputField`
- Hooked `OnPointerClick`, `OnSelect`, and `ActivateInputField` to log every call with the `this` pointer.
- **Result:** Confirmed that the manually clicked `TMP_InputField` instance pointer differs from the one the code was targeting via `InputFieldWidget._input`.

### 3. Click recording per controller
- Walked up the Transform hierarchy from the clicked input field to find the owning `InventoryListViewController` or `OfficerRosterViewController`.
- Recorded the clicked `TMP_InputField` pointer per controller.
- In `FocusSearch`, used the recorded field first.
- **Result:** The hierarchy walk found a controller, but the recorded field was not the one the hotkey should focus later. After the user clicked elsewhere/unfocused, the recorded field's `GameObject` became inactive in the hierarchy, so the code fell back to a different field.

### 4. Enumerating active `TMP_InputField` children
- Used `GameObject.GetComponentsInternal` (recursive, non-inactive) to find all `TMP_InputField` components under the controller's `GameObject`.
- Picked the first active/visible one.
- **Result:** Only one active child was found under the controller, and it was still not the manually clicked field.

### 5. Picking the controller by active child field
- `FocusSearchBox` iterated all `OfficerRosterViewController` instances and chose the first one with an active input field child.
- **Result:** Multiple live controller instances exist (4 observed). The instance with an active child field was not the one the user clicked, so the wrong search box was targeted.

### 6. Section matching
- Compared each controller's `_targetSection` (offset `0x60`) with `SectionManager.CurrentSection`.
- **Result:** The current section ID did not match any controller's `_targetSection`. The officer inventory likely lives as a sub-view/popup without changing the top-level section, so `_targetSection` is not a reliable discriminator.

### 7. Global last-clicked controller tracking
- Stored the last controller that received a manual `TMP_InputField` click in a global variable.
- In `FocusSearchBox`, preferred that controller if it was still active in the hierarchy.
- **Result:** The last-clicked controller was found, but its `GameObject` was already inactive by the time `Ctrl+F` was pressed, so the code fell back again to the wrong controller.

### 8. Reorder `FocusSearch` to call `InputFieldWidget.Focus()` first
- Called `InputFieldWidget.Focus()` before searching for the active input field, so the widget could activate its search box first.
- **Result:** Still targeted the wrong controller because the controller selection was the root issue.

## Why It Did Not Work

The fundamental problem is that the visible Officer Inventory search UI and the tracked `OfficerRosterViewController` instances are not in a simple 1:1 relationship:

- There are multiple live `OfficerRosterViewController` instances at the same time.
- `MonoBehaviour.isActiveAndEnabled` is true on more than one of them.
- The `TMP_InputField` that receives the manual click is not a child of the controller that is marked active, nor is it the same instance the `InputFieldWidget` exposes via `get_input`.
- The manually clicked field's `GameObject` becomes inactive in the hierarchy between the click and the hotkey press, so any strategy that requires the clicked field to remain active fails.
- The game does not change the top-level `SectionManager.CurrentSection` to a value matching `_targetSection` while the Officer Inventory is open, so section-based selection is not viable.
- Because `InputFieldWidget.Focus()` triggers an `OnPointerClick` on whichever field it owns, it also pollutes the recorded click target, making click-recording strategies unreliable.

## Important Observations

- `UnityEngine.Component.get_gameObject()` and `get_transform()` are **managed methods**, not icalls. Resolving them with `il2cpp_resolve_icall` returns `nullptr`.
- `GameObject.GetComponentsInChildren(Type, bool)` has generic overloads with the same name/arity, so `IL2CppClassHelper::GetMethod(name, 2)` can fail or return `nullptr`. Use `GameObject.GetComponentsInternal(..., 6)` (unique signature) for reliable enumeration.
- `TMP_InputField` instances can be active/visible without being the search box the user expects. The visual "search box" may be a sibling or even a different GameObject subtree than the one the controller references directly.
- Multiple tracked controller instances, combined with the game reusing or keeping inactive views alive, makes simple "active instance" selection unreliable for this view.

## Conclusion

The feature was not completed because the runtime relationship between the tracked `OfficerRosterViewController` objects and the actual visible search input is not the straightforward hierarchy that the initial approach assumed. A reliable implementation would likely need a deeper understanding of how the view controller is bound to the search UI, or a different identification strategy (e.g. matching by sibling `InputFieldWidget` GameObject, observing the `EventSystem` current selected object, or using a different controller property that maps to the visible view).

All FocusSearch-related code and data types have been removed from the codebase per the request to return to the pre-implementation state.
