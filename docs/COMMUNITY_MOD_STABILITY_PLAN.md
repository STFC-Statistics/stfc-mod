# STFC Community Mod — Stability, Diagnostics, and Validation Plan

Evidence-driven plan for improving Windows and macOS stability, diagnostics, runtime inspection, and patch drift detection. Work is divided into a no-change validation baseline followed by four implementation PRs.

## Scope and sequencing

- **Phase 0 — Validation baseline:** inventory hooks, signatures, platform guards, offsets, and existing runtime evidence before changing implementation routes.
- **PR 1 — Crash safety:** reduce mod-induced crashes without claiming C++ exceptions catch access violations.
- **PR 2 — Diagnostics and configuration:** crash artifacts, version checks, safe config reload, patch health, and runtime exception reporting.
- **PR 3 — Debug overlay:** ImGui config editing and diagnostics, only after a rendering/input spike proves a stable platform integration.
- **PR 4 — Dev-build validation and drift audit:** cross-check every patch against m93.1 and m93-dev, then run runtime smoke tests.

The implementation must preserve a disable/rollback path for every patch. No hook should be enabled on macOS solely because a matching decompiled method exists; detour size, signature, runtime behavior, and platform safety must also be established.

## Phase 0 — Validation baseline

1. Freeze the test matrix: production m93.1, available m93-dev, Windows configuration/architecture, and macOS arm64 with deployment target 13.5.
2. Generate an exact inventory from `SPUD_STATIC_DETOUR`, `il2cpp_get_class_helper`, `GetMethod*`, `GetField`, `GetProperty`, hardcoded offsets, and platform guards. Record source location, signature, return type, original-call fallback, thread, and platform.
3. Establish the macOS build baseline:

   ```bash
   xmake f -p macosx -a arm64 -m debug --target_minver=13.5 -y
   xmake -y mods
   ```

   Use `scripts/mac-build-test-debug.sh` for `run`, `debug`, and `crashlogs` when operating on macOS.
4. Run `scripts/validate_drift.ps1` against both decompiled trees and manually resolve every `manual-review` result.
5. Capture an unmodified baseline: startup log, patch-install summary, config/log paths, game version, crash-report locations, and reproducible smoke actions per patch.
6. Verify that `installGameVersionHook` is present in `Config` (`config.cc`) but **missing from the `patches[]` array** in `patches.cc`. Document whether this is intentional or a bug to fix in PR 2.
7. Keep optional macOS notifications and the Metal overlay from blocking crash handling, logging, symbol validation, or platform-skip documentation.

## PR 1 — Crash safety

### 1.1 IL2CPP helper validity

**File:** `mods/src/il2cpp/il2cpp_helper.h`

Make all `isValidHelper()` implementations check their relevant class/member pointer in every build mode. Preserve safe null behavior in `Get`, `SetRaw`, `GetRaw`, and field/property accessors.

### 1.2 Hook guards and null checks

Inventory all hooks before applying guards. C++ `try/catch` covers C++ exceptions only; it does not catch access violations or invalid-memory faults under the current `/EHsc`/`set_exceptions("cxx")` configuration. Guards must preserve the original-call fallback and return type for each hook. Do not use a blanket macro that silently skips the original method.

Add targeted `_this`, nested-field, method-pointer, and array checks to hot paths, especially pan, zoom, UI-scale, hotkey, sync, toast, and transition hooks. Record any deliberate behavior change in the validation report.

### 1.3 Object tracker safety

**Files:** `mods/src/il2cpp/il2cpp_helper.h`, `mods/src/patches/parts/object_tracker.cc`

Make tracker reads use `find()` rather than mutating `operator[]` in both `ObjectFinder::Get()` and `ObjectFinder::GetAll()`, return empty results for absent classes, and log empty `Get()` results in debug mode. Validate object/class assumptions only where the runtime representation makes that check safe.

Add `tracked_objects_mutex` protection to `ObjectFinder::Get()` and `ObjectFinder::GetAll()` read paths — currently only the mutation paths (ctor/destroy hooks) hold the mutex, leaving reads unprotected against concurrent tracker updates.

Also replace the bare `catch(...)` in `ApplyPatches()` (`patches.cc`) that silently swallows all exceptions from `SPUD_STATIC_DETOUR` with structured error reporting that logs the failure.

### 1.4 macOS entry-point and symbol hardening

**Files:** `macos-dylib/src/main.cc`, `mods/src/patches/patches.cc`, `mods/src/il2cpp/il2cpp-functions.cc`

- Catch C++ exceptions escaping the dylib constructor and report them through a pre-spdlog fallback; tolerate missing/empty constructor arguments.
- Replace early `printf` diagnostics that disappear through the loader's `execvp` path with a safe early logging path.
- After `dlsym`, validate only symbols classified as required for initialization, required by an enabled patch, or optional. Report missing symbols with their names.
- Keep the macOS `BuffService.ParseBinaryObject` ARM64 detour skip documented until an alternative target or safe trampoline is proven.
- Add a macOS quit path only if it is safe and intentionally equivalent to the existing Windows behavior.

### PR 1 gates

- Windows releasedbg build succeeds.
- macOS arm64 debug build succeeds with the AGENTS.md command.
- `git diff --check` succeeds.
- Windows and macOS logs show patch-install outcomes.
- A controlled C++ exception is logged without terminating the process.
- macOS early-init failures are visible through the chosen fallback and normal DiagnosticReports behavior remains intact.

## PR 2 — Diagnostics and configuration

### 2.1 Crash artifacts

**Files:** `mods/src/crash_handler.h/.cc` (new files to create), `mods/src/patches/patches.cc`

- Windows: install an unhandled-exception filter and write a minidump next to the log.
- macOS: use `sigaction(SA_SIGINFO)` for a minimal, async-signal-safe artifact. Use `open`/`write`/`close`; do not call spdlog, syslog, os_log, allocation, C++ strings, or file streams from the signal handler. Treat macOS DiagnosticReports as authoritative. A Mach exception handler is a separate future design, not a prerequisite.

### 2.2 Version, config, and exceptions

- Detect and log the actual game/Unity version, comparing against the supported target.
- Add config reload with synchronization for non-trivial strings/vectors; do not pretend already-installed hooks can be reconfigured without an explicit mechanism.
- Release patch toggles are already honored in all builds (`config.cc:622-624`); focus on improving disabled-patch logging clarity and ensuring the `installGameVersionHook` setting (present in `Config` but missing from the `patches[]` array) is either registered or documented as intentionally excluded.
- Add structured IL2CPP exception logging at safe `il2cpp_runtime_invoke` boundaries.

### 2.3 Hook health

Add a registry for install status, call count, error count, and last error. Aggregate missing helpers/methods and expose a clear startup summary. Keep registry access thread-safe and bounded. Build on the existing `ErrorMsg::MissingMethod`/`ErrorMsg::MissingHelper` pattern already used across patch files rather than designing a parallel reporting system.

### 2.4 macOS logging

Verify the actual `File::MakePath` output and directory creation behavior. Confirm that logs are available under the macOS preferences path and that early loader/dylib failures are still diagnosable before spdlog initialization.

Desktop notifications remain optional. Do not add Objective-C notification frameworks to the stability PR unless an isolated prototype proves host bundle, entitlement, authorization, queue, and failure behavior.

### PR 2 gates

- Windows minidump and macOS custom artifact/DiagnosticReports behavior are verified with controlled crashes.
- Config reload is tested for scalar and string/vector settings under concurrent readers.
- Missing hooks produce an aggregate warning and do not terminate the game.
- `git diff --check` and both platform builds succeed.

## PR 3 — Debug overlay with config editing

The overlay is a diagnostic feature, not a prerequisite for crash safety.

### Dependency and rendering spike

First verify that the configured xmake sources provide a vetted, compatible ImGui package/backend. If not, choose a pinned vendored revision or defer the overlay; do not assume an xmake `imgui` package exists.

- Windows: prototype D3D11 `IDXGISwapChain::Present` integration.
- macOS: prototype a real Unity Metal frame boundary. `CAMetalLayer::nextDrawable` only returns a drawable; it is not by itself a valid ImGui render hook. ImGui Metal rendering requires the active command buffer and compatible render pass/encoder. Do not ship a `nextDrawable`-only implementation.
- Recreate backend resources if Unity replaces the layer or drawable size.
- Integrate input interception only after rendering is stable: Win32 WndProc on Windows; the game's main NSView/NSEvent path on macOS.

### Panels

Provide read-only and editable views for:

1. hot-reloadable config, with Save and Reload actions;
2. hook installation/call/error health;
3. sync queue, request, response, latency, and error state;
4. game/mod version, FPS, tracked objects, and process memory;
5. patch status, including disabled, installed, failed, and manual-review states.

All config editing must validate ranges, serialize safely, and distinguish runtime-only changes from changes requiring restart/reinstallation.

### PR 3 gates

- The selected/pinned ImGui backend builds on each supported platform.
- A platform rendering spike succeeds before full overlay work continues.
- Overlay visibility, input capture, config edit, save, reload, and shutdown are tested on each supported platform.
- If macOS Metal integration is not stable, leave it disabled and document the fallback rather than shipping an unvalidated swizzle.

## PR 4 — Dev-build validation and drift audit

Create `docs/29_Patch_Validation_Report.md` or its workspace equivalent with one entry for each of the 21 patch groups currently listed in `patches.cc`.

Each entry must include:

1. mod helper and detour target with source line;
2. m93-dev and m93.1 class/member references;
3. signature evidence from dump/metadata/runtime resolution, without inferring hidden IL2CPP parameters solely from decompiler output;
4. drift status;
5. field-offset evidence from metadata/runtime layout, not C# field order alone;
6. enum comparison;
7. evidence classification: `static-confirmed`, `runtime-confirmed`, `inferred`, or `unknown`;
8. implementation-route assessment;
9. reproducible in-client regression action and expected observation.

### Confirmed findings to track

- m93-dev `ToastState.cs` includes `ChapterCompleted = 59`; the current `mods/src/prime/Toast.h` enum ends at `GalacticAnomalySystemEntered = 58`.
- m93-dev `OfficerSortGenerators.cs` contains `_belowDeckAbilitySortId` and `_assignBelowDeckAbilityId`; m93.1 lacks those fields. This supports the below-deck restoration hypothesis but does not alone prove the injected sort route is correct.
- `BuffService.IsBuffConditionMet` is private in the dev source while `AreAllBuffConditionsMet` is public. Compare both routes and test behavior before selecting one.
- `DataInterceptorHandler` demonstrates path/entity-group interception concepts, but its presence does not prove it is a drop-in replacement for the mod's sync hooks. Trace initialization and callback flow first.
- `Toast.h` uses hardcoded offsets `0x18`, `0x20`, and `0x38`; compare against metadata/runtime layout before considering field-helper migration.
- Audit the `#if 0` disabled block in `sync.cc` (around the `GameServerModelRegistry` section) that contains incomplete or abandoned functionality; document whether it should be removed or completed.

### Runtime evidence

- The available `SRDebuggerWebUI.cs` confirms a `/cheats` path and `UnityHttpServer`, but does not establish port 4649. Use the running client's actual `GetURL()` result or configuration.
- `DevConsoleInput.cs` confirms reflection-oriented parsing patterns, but decompiled method bodies are stubs. Validate command grammar interactively before documenting commands.
- Record client build, scene, object path, command, captured value, and patch/config state for every runtime observation.

### Drift tool limitations

The validator should extract literal class/member lookups from source, check both decompiled trees, preserve file/line references, and emit `manual-review` for dynamic names, overload ambiguity, generated names, unresolved paths, and anything it cannot prove. It is a release preflight aid, not proof of detour safety or behavior.

### macOS platform-skip audit

Explicitly review and document:

- `sync.cc`'s `BuffService.ParseBinaryObject` skip due to short ARM64 function body;
- `zoom.cc`'s Windows-only `SetDepth` hook;
- Windows-only free-resize/WndProc behavior;
- Windows-only donation slider settings;
- the intentional macOS notification no-op.

For each, record reason, runtime/static evidence, user impact, safe alternative, and rollback behavior.

## Required verification commands

```powershell
# Workspace/static checks
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/validate_drift.ps1 `
  -M931Root "<path-to-m93.1-il2cpp-source>" `
  -M93DevRoot "<path-to-m93-dev-il2cpp-source>" `
  -OutputPath docs/drift_report.md

# Windows
xmake f -p windows -m releasedbg -y
xmake -y

# macOS (run on macOS)
xmake f -p macosx -a arm64 -m debug --target_minver=13.5 -y
xmake -y mods

git diff --check
```

## Acceptance criteria

- Every enabled hook has a verified target, signature source, original-call fallback, platform status, and rollback/disable path.
- Static drift reports contain no unexplained missing members; remaining unknowns are explicitly marked `manual-review`.
- macOS has equivalent diagnostics for early init, symbol resolution, hook installation, crashes, and logs.
- No optional overlay/notification feature blocks the stability path.
- Both platform build gates and targeted runtime smoke tests pass before a PR is considered complete.
