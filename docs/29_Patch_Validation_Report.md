# Patch Validation Report

Generated: 2026-08-02
Mod version: 1.1.5 (Patch 1)
Game versions: m93.1, m93-dev

## Summary

| Metric | Count |
|---|---:|
| Patch groups | 21 |
| Class lookups (unique) | 222 |
| Classes present in both builds | 164 |
| Classes missing in both builds | 58 |
| Manual-review results | 451 |

The 58 "missing" classes are Unity engine types (UnityEngine.*, System.*, Google.Protobuf.*), compiler-generated nested types (e.g. `FleetsManager.<Tow>d__192`, `StateContainer\`1`), and generic types — all expected to not have standalone .cs files in the decompiled source. See `docs/drift_report.md` for the full table.

## Confirmed Findings

### F1: ToastState enum missing ChapterCompleted = 59

- **Status**: `static-confirmed` — present in both m93.1 and m93-dev decompiled source
- **Mod file**: `mods/src/prime/Toast.h:5-65`
- **m93.1**: `Assembly-CSharp/Digit.Prime.HUD/ToastState.cs:63` — `ChapterCompleted = 59`
- **m93-dev**: `Assembly-CSharp/Digit.Prime.HUD/ToastState.cs:63` — `ChapterCompleted = 59`
- **Impact**: Low — the mod's enum ends at `GalacticAnomalySystemEntered = 58`. Missing `ChapterCompleted` means toast banners for chapter completions won't be recognized by enum-based filtering. The `disable_banners` patch uses `ToastState` for filtering, so chapter completion toasts would pass through unfiltered.
- **Action**: Add `ChapterCompleted = 59` to `mods/src/prime/Toast.h`

### F2: OfficerSortGenerators — below-deck ability fields

- **Status**: `static-confirmed` in m93-dev
- **m93-dev**: `Assembly-CSharp/Digit.Prime.Officers/OfficerSortGenerators.cs:29` — `private string _belowDeckAbilitySortId;` and `:35` — `private string _assignBelowDeckAbilityId;`
- **m93.1**: Same fields present (confirmed via file search)
- **Mod file**: `mods/src/patches/parts/officer_sort.cc`
- **Impact**: The mod restores the Below Deck Ability sort option. The presence of these fields in both decompiled trees supports the restoration hypothesis. The mod injects sort generators at runtime via `SortingPredicates` and `SortComparer` — both classes are `present-both` in the drift report.
- **Evidence classification**: `static-confirmed` (fields exist) + `runtime-confirmed` (mod log shows "Officer sort: restored Below Deck Ability sort option")
- **Action**: No code change needed. Document as confirmed.

### F3: BuffService.IsBuffConditionMet vs AreAllBuffConditionsMet

- **Status**: `manual-review` — requires runtime validation
- **Mod file**: `mods/src/patches/parts/buff_fixes.cc:30` — hooks `BuffService` class
- **Finding**: `IsBuffConditionMet` is private in dev source while `AreAllBuffConditionsMet` is public. The mod hooks `BuffService` for out-of-dock power calculation.
- **Action**: Compare both routes and test behavior before selecting one. The mod currently uses the `BuffService` class helper and resolves methods at runtime — the specific method used should be verified against the running client.

### F4: #if 0 disabled block in sync.cc — PlatformModelRegistry.ProcessResultInternal

- **Status**: `manual-review` — incomplete/abandoned functionality
- **Mod file**: `mods/src/patches/parts/sync.cc:2110-2122`
- **Content**: The disabled block attempts to hook `PlatformModelRegistry.ProcessResultInternal` for sync data interception. It was disabled (via `#if 0`) and never completed.
- **Evidence**: `PlatformModelRegistry` is in `Digit.PrimePlatform.Core` namespace. The class is not in the drift report's `present-both` list — it appears as a `manual-review` item.
- **Action**: Remove the `#if 0` block to reduce dead code, or document why it was abandoned. The `GameServerModelRegistry.ParseBinaryObjectsHelper` hook (line 2107) covers the same sync data flow.

### F5: Toast.h hardcoded offsets (0x18, 0x20, 0x38)

- **Status**: `manual-review` — requires metadata/runtime layout verification
- **Mod file**: `mods/src/prime/Toast.h:71,76,81`
- **Offsets**:
  - `0x18` — `get_TextParameters()` returns `Il2CppArray*`
  - `0x20` — `get_TextLocaleTextContext()` returns `void*`
  - `0x38` — `get_Data()` returns `Il2CppObject*`
- **Risk**: Hardcoded offsets break if the game adds/removes fields in the `Toast` class. The `State` property (line 86) correctly uses the IL2CPP property helper instead.
- **Action**: Compare against metadata/runtime layout. If field offsets match, mark as `runtime-confirmed`. Consider migrating to field helpers (`GetField()`) for future-proofing.

## Patch Group Entries

### 1. UiScaleHooks

- **Install function**: `InstallUiScaleHooks` (`mods/src/patches/parts/ui_scale.cc`)
- **Config toggle**: `cfg.installUiScaleHooks`
- **Detour target**: `ScreenManager.UpdateCanvasRootScaleFactor` (`Assembly-CSharp/Digit.Client.UI/ScreenManager`)
- **Drift status**: `static-confirmed` — `ScreenManager` present in both m93.1 and m93-dev
- **Platform**: All
- **Evidence**: Class lookup confirmed in drift report. Method resolved at runtime via `GetMethod("UpdateCanvasRootScaleFactor")`.
- **Regression test**: Launch game, verify UI scales correctly at different resolutions. Check log for "UI Scale" patch install.

### 2. ZoomHooks

- **Install function**: `InstallZoomHooks` (`mods/src/patches/parts/zoom.cc`)
- **Config toggle**: `cfg.installZoomHooks`
- **Detour targets**: `NavigationZoom.Update`, `PlanetViewUtils.CameraZoomedEventHandler`, `NavigationZoom.SetDepth` (Windows only)
- **Drift status**: `static-confirmed` — `NavigationZoom`, `PlanetViewUtils` present in both trees
- **Platform**: `SetDepth` hook is Windows-only (see macOS skip audit)
- **Evidence**: Class lookups confirmed. Null checks added in PR 1 for `_this` pointers.
- **Regression test**: Zoom in/out on galaxy map and planet view. Verify zoom limits work.

### 3. BuffFixHooks

- **Install function**: `InstallBuffFixHooks` (`mods/src/patches/parts/buff_fixes.cc`)
- **Config toggle**: `cfg.installBuffFixHooks`
- **Detour target**: `BuffService` (`Digit.Client.PrimeLib.Runtime/Digit.PrimeServer.Services/BuffService`)
- **Drift status**: `static-confirmed` — `BuffService` present in both trees
- **Platform**: All
- **Evidence**: See F3 above regarding `IsBuffConditionMet` vs `AreAllBuffConditionsMet`.
- **Regression test**: Dock/undock a ship and verify out-of-dock power calculation is correct.

### 4. ToastBannerHooks

- **Install function**: `InstallToastBannerHooks` (`mods/src/patches/parts/disable_banners.cc`)
- **Config toggle**: `cfg.installToastBannerHooks`
- **Detour targets**: `ToastObserver.EnqueueToast`, `ToastObserver.EnqueueOrCombineToast` (`Assembly-CSharp/Digit.Prime.HUD/ToastObserver`)
- **Drift status**: `static-confirmed` — `ToastObserver` present in both trees
- **Platform**: All
- **Evidence**: See F1 above regarding `ToastState` enum gap. Null checks added in PR 1.
- **Regression test**: Trigger various toast events (battle results, achievements). Verify disabled banner types are suppressed.

### 5. PanHooks

- **Install function**: `InstallPanHooks` (`mods/src/patches/parts/fix_pan.cc`)
- **Config toggle**: `cfg.installPanHooks`
- **Detour targets**: `TKTouch.populateWithPosition` (`TouchKit`), `NavigationPan.LateUpdate` (`Assembly-CSharp/Digit.Prime.Navigation/NavigationPan`)
- **Drift status**: `static-confirmed` — both classes present in both trees
- **Platform**: All
- **Evidence**: Null checks added in PR 1 for `TKTouch` and `NavigationPan._this`.
- **Regression test**: Pan around galaxy map with touch/mouse. Verify smooth panning without crashes.

### 6. HotkeyHooks

- **Install function**: `InstallHotkeyHooks` (`mods/src/patches/parts/hotkeys.cc`)
- **Config toggle**: `cfg.installHotkeyHooks`
- **Detour targets**: `ShortcutsManager`, `ScreenManager.Update`, `RewardsButtonWidget`, `PreScanTargetWidget`
- **Drift status**: `static-confirmed` — all classes present in both trees
- **Platform**: All
- **Evidence**: Existing null checks in `OnDidBindContext_Hook` and `ShowWithFleet_Hook`.
- **Regression test**: Press configured hotkeys. Verify fleet shortcuts and screen navigation work.

### 7. GiftsBulkClaimHooks

- **Install function**: `InstallGiftsBulkClaimHooks` (`mods/src/patches/parts/open_bulk_claim_gifts.cc`)
- **Config toggle**: `cfg.installGiftsBulkClaimHooks`
- **Detour target**: `SectionManager` (`Assembly-CSharp/Digit.Client.Sections/SectionManager`)
- **Drift status**: `static-confirmed` — `SectionManager` present in both trees
- **Platform**: All
- **Regression test**: Open gifts section, verify bulk claim functionality works.

### 8. FreeResizeHooks (Windows only)

- **Install function**: `InstallFreeResizeHooks` (`mods/src/patches/parts/free_resize.cc`)
- **Config toggle**: `cfg.installFreeResizeHooks`
- **Detour target**: `AspectRatioConstraintHandler` (`Assembly-CSharp/Digit.Client.Utils/AspectRatioConstraintHandler`)
- **Drift status**: `static-confirmed` — present in both trees
- **Platform**: Windows only (`#if _WIN32` guard in patches array)
- **Evidence**: Uses Win32 WndProc for resize behavior. See macOS skip audit.
- **Regression test**: Resize game window on Windows. Verify free resize works without aspect ratio lock.

### 9. TempCrashFixes

- **Install function**: `InstallTempCrashFixes` (`mods/src/patches/parts/misc.cc`)
- **Config toggle**: `cfg.installTempCrashFixes`
- **Drift status**: `manual-review` — requires runtime verification of specific crash fix targets
- **Platform**: All
- **Regression test**: Verify game stability during common crash scenarios (rapid navigation, loading transitions).

### 10. TestPatches

- **Install function**: `InstallTestPatches` (`mods/src/patches/parts/testing.cc`)
- **Config toggle**: `cfg.installTestPatches`
- **Detour targets**: `AppConfig`, `Model`, `Cursor` — all `static-confirmed` in both trees
- **Platform**: All
- **Evidence**: Test/debug patches. Classes confirmed present.
- **Regression test**: Verify test patches don't interfere with normal gameplay.

### 11. MiscPatches

- **Install function**: `InstallMiscPatches` (`mods/src/patches/parts/misc.cc`)
- **Config toggle**: `cfg.installMiscPatches`
- **Drift status**: `manual-review` — miscellaneous patches, targets vary
- **Platform**: All
- **Regression test**: Verify miscellaneous QoL features work as expected.

### 12. ChatPatches

- **Install function**: `InstallChatPatches` (`mods/src/patches/parts/chat.cc`)
- **Config toggle**: `cfg.installChatPatches`
- **Detour targets**: `FullScreenChatViewController`, `ChatPreviewController` (`Assembly-CSharp/Digit.Prime.Chat`)
- **Drift status**: `static-confirmed` — both classes present in both trees
- **Platform**: All
- **Regression test**: Open chat, verify fullscreen mode and chat preview work correctly.

### 13. ResolutionListFix

- **Install function**: `InstallResolutionListFix` (`mods/src/patches/parts/misc.cc`)
- **Config toggle**: `cfg.installResolutionListFix`
- **Drift status**: `manual-review` — typically skipped (disabled by default)
- **Platform**: All
- **Regression test**: Check available resolutions in game settings.

### 14. SyncPatches

- **Install function**: `InstallSyncPatches` (`mods/src/patches/parts/sync.cc`)
- **Config toggle**: `cfg.installSyncPatches`
- **Detour targets**: `GameServer.Initialise`, `GameServer.SetInstanceIdHeader`, `GameServerModelRegistry.ParseBinaryObjectsHelper`, `BuffDataContainer.ParseBinaryObject`, `BuffService.ParseBinaryObject` (Windows only), various data containers
- **Drift status**: `static-confirmed` — `GameServer`, `GameServerModelRegistry`, `BuffDataContainer`, `BuffService` all present in both trees
- **Platform**: `BuffService.ParseBinaryObject` skipped on macOS (see skip audit)
- **Evidence**: See F4 above for `#if 0` disabled `PlatformModelRegistry` block. `installGameVersionHook` is handled within this patch group (not a separate top-level patch).
- **Regression test**: Verify sync data flows correctly. Check log for sync headers and battle data upload.

### 15. ObjectTracker

- **Install function**: `InstallObjectTrackers` (`mods/src/patches/parts/object_tracker.cc`)
- **Config toggle**: `cfg.installObjectTracker`
- **Drift status**: `static-confirmed` — tracked object classes present in both trees
- **Platform**: All
- **Evidence**: Mutex protection added in PR 1 for `tracked_objects` map. `ObjectFinder::Get()` and `GetAll()` fixed to use `find()` instead of `operator[]`.
- **Regression test**: Verify tracked objects are correctly enumerated. Check for race conditions during rapid scene transitions.

### 16. LoadingScreen

- **Install function**: `InstallLoadingScreenHooks` (`mods/src/patches/parts/loading_screen.cc`)
- **Config toggle**: `cfg.installLoadingScreenHooks`
- **Detour targets**: `LoginSequence.Awake` (`Assembly-CSharp/Digit.Prime.Login/LoginSequence`), Unity `Component.get_transform`, `Transform`, `GameObject`, `Image`
- **Drift status**: `static-confirmed` — `LoginSequence` present in both trees. Unity engine types are `manual-review` (expected — no standalone .cs files).
- **Platform**: All
- **Regression test**: Verify custom loading screen background and logos appear during login.

### 17. TransitionScreen

- **Install function**: `InstallTransitionScreenHooks` (`mods/src/patches/parts/transition_screen.cc`)
- **Config toggle**: `cfg.installTransitionScreenHooks`
- **Detour targets**: Unity `MonoBehaviour`, `GameObject`, `Component`, `Behaviour` — all `manual-review` (Unity engine types)
- **Drift status**: `manual-review` — Unity engine types not in decompiled source as standalone files
- **Platform**: All
- **Regression test**: Verify screen transitions work smoothly without visual artifacts.

### 18. LoadingTip

- **Install function**: `InstallLoadingTipHooks` (`mods/src/patches/parts/loading_tip.cc`)
- **Config toggle**: `cfg.loader_tip_enabled`
- **Drift status**: `manual-review` — requires runtime verification
- **Platform**: All
- **Regression test**: Verify loading tips display correctly during loading screens.

### 19. FocusSearch

- **Install function**: `InstallFocusSearchHooks` (`mods/src/patches/parts/focus_search.cc`)
- **Config toggle**: `cfg.installFocusSearchHooks`
- **Drift status**: `manual-review` — requires runtime verification
- **Platform**: All
- **Evidence**: Log confirms "[FocusSearch] installed" at runtime.
- **Regression test**: Focus search input field, verify auto-focus behavior works.

### 20. CargoFormat

- **Install function**: `InstallCargoFormatHooks` (`mods/src/patches/parts/cargo_format.cc`)
- **Config toggle**: `cfg.installCargoFormatHooks`
- **Detour targets**: `TextLocalizer`, `ColourTextLocalizer` (`Assembly-CSharp/Digit.Client.UI`)
- **Drift status**: `static-confirmed` — both classes present in both trees
- **Platform**: All
- **Evidence**: Log confirms "Cargo format: significant decimals = 2" at runtime.
- **Regression test**: View cargo amounts in UI. Verify significant decimals formatting is correct.

### 21. OfficerSortHooks

- **Install function**: `InstallOfficerSortHooks` (`mods/src/patches/parts/officer_sort.cc`)
- **Config toggle**: `cfg.installOfficerSortHooks`
- **Detour targets**: `SortingPredicates`, `SortComparer` (`Assembly-CSharp/Digit.Client.Sorting`)
- **Drift status**: `static-confirmed` — both classes present in both trees
- **Platform**: All
- **Evidence**: See F2 above. Log confirms "Officer sort: restored Below Deck Ability sort option".
- **Regression test**: Open officer roster, verify Below Deck Ability sort option is available and works correctly.

## macOS Platform-Skip Audit

### S1: BuffService.ParseBinaryObject skip

- **Location**: `mods/src/patches/parts/sync.cc:2136-2139`
- **Reason**: `BuffService.ParseBinaryObject` has a 0x18-byte function body immediately before `HandleResponseData` on ARM64. Spud's ARM64 absolute jump is larger than 0x18 bytes, so detouring it overwrites the next function entry.
- **Evidence**: `static-confirmed` — function body size verified via binary inspection (documented in code comment)
- **User impact**: Buff sync data parsing is not intercepted on macOS. The `BuffDataContainer.ParseBinaryObject` hook (which is installed on macOS) covers similar data flow.
- **Safe alternative**: `BuffDataContainer.ParseBinaryObject` hook remains active on macOS
- **Rollback behavior**: N/A — the skip is compile-time (`#if __APPLE__`)

### S2: NavigationZoom.SetDepth Windows-only

- **Location**: `mods/src/patches/parts/zoom.cc`
- **Reason**: Windows-only `SetDepth` hook — platform guard in patch installation
- **Evidence**: `inferred` — likely related to ARM64 detour size constraints similar to S1
- **User impact**: Depth-based zoom adjustment not available on macOS
- **Safe alternative**: Other zoom hooks (`NavigationZoom.Update`, `PlanetViewUtils.CameraZoomedEventHandler`) remain active on macOS
- **Rollback behavior**: N/A — compile-time guard

### S3: FreeResizeHooks Windows-only

- **Location**: `mods/src/patches/patches.cc:123-125` — `#if _WIN32` guard in patches array
- **Reason**: Uses Win32 `WndProc` for resize behavior, which is Windows-specific
- **Evidence**: `static-confirmed` — `AspectRatioConstraintHandler` class exists on both platforms, but the WndProc-based implementation is Windows-only
- **User impact**: Free resize not available on macOS
- **Safe alternative**: macOS uses native window resizing
- **Rollback behavior**: N/A — compile-time guard

### S4: Windows-only donation slider settings

- **Location**: Config settings for donation slider are Windows-specific
- **Reason**: UI implementation uses Win32 APIs
- **Evidence**: `inferred`
- **User impact**: Donation slider not shown on macOS
- **Safe alternative**: N/A
- **Rollback behavior**: N/A — config defaults handle platform difference

### S5: macOS notification no-op

- **Location**: `mods/src/patches/notification_service.cc`
- **Reason**: Windows notification service uses WinRT; macOS intentionally has a no-op notification implementation
- **Evidence**: `static-confirmed` — log shows "[Notify] Windows notification service initialized" only on Windows
- **User impact**: No desktop notifications on macOS
- **Safe alternative**: Game-internal toast system remains functional
- **Rollback behavior**: N/A — runtime platform check

## Evidence Classification Summary

| Classification | Count | Description |
|---|---:|---|
| `static-confirmed` | 164 | Class found in both m93.1 and m93-dev decompiled source |
| `runtime-confirmed` | 3 | Verified at runtime via mod logs (FocusSearch, OfficerSort, CargoFormat) |
| `inferred` | 0 | Class found in only one decompiled tree |
| `unknown` | 58 | Unity engine types, generics, compiler-generated — not in decompiled source as standalone files |
| `manual-review` | 451 | Member lookups (scope-ambiguous), dynamic lookups, and missing classes |

## Required Actions

1. **F1**: Add `ChapterCompleted = 59` to `mods/src/prime/Toast.h`
2. **F4**: Remove or document the `#if 0` block in `sync.cc:2110-2122`
3. **F5**: Verify Toast.h hardcoded offsets against metadata/runtime layout
4. **F3**: Runtime-test `IsBuffConditionMet` vs `AreAllBuffConditionsMet` behavior

## Drift Tool Limitations

The `validate_drift.ps1` script is a static preflight aid. It:
- Extracts literal class/member lookups from mod source
- Checks both decompiled trees for class file presence
- Cannot resolve scope-ambiguous member lookups (which class helper owns which `GetMethod` call)
- Cannot prove detour safety, signatures, offsets, or runtime behavior
- Emits `manual-review` for dynamic names, overload ambiguity, generated names, and unresolved paths
