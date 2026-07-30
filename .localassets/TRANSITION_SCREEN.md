# Transition Screen / Loading Screen System

## Overview

The STFC Community Mod replaces the game's default loading / transition screen with:

1. A **custom background image** and logo overlays during transitions.
2. A **custom tip message** shown as the first tip on every transition, followed by the game's normal random tip rotation.

The implementation spans three source files:

| File | Role |
|------|------|
| `mods/src/patches/parts/loading_screen.cc` | Custom background + logos on the login/loading screen (hooks `LoginSequence.Awake`) |
| `mods/src/patches/parts/transition_screen.cc` | Custom background (or black) + logos on transition screens (hooks `TransitionViewController` lifecycle) |
| `mods/src/patches/parts/loading_tip.cc` | Custom tip messages on loading/transition screens (hooks `LoadingTipViewController`) |
| `mods/src/patches/parts/loading_screen_common.h` | Shared utilities: texture loading, sprite creation, image overlay creation, logo placement |

---

## Game Classes Involved

From `Assembly-CSharp`, namespace `Digit.Prime.LoadingScreen`:

| Class | Role |
|-------|------|
| `TransitionManager` | Singleton state-machine. Key field: `_currentState` (offset 0x58, `int32_t` enum). Methods: `SetLoadingScreen()`, `Hide()` |
| `TransitionViewController` | MonoBehaviour on the transition UI prefab. Methods: `Awake`, `AboutToShow`, `AboutToHide` |
| `LoadingTipViewController` | Handles tip rotation. Method: `SetRandomTipLocalisedText()` |
| `SlideShowViewer` | Background slideshow during some transitions |

---

## Hooks Installed

### In `loading_screen.cc`

| Hook | Purpose |
|------|---------|
| `LoginSequence.Awake` | Replaces the login screen background image with custom texture, adds logo overlays |

### In `transition_screen.cc`

| Hook | Purpose |
|------|---------|
| `TransitionViewController.Awake` | Resets state, applies customization as fallback (AboutToShow may not fire after reload) |
| `TransitionViewController.AboutToShow` | Primary hook for applying custom background + logos |
| `TransitionViewController.AboutToHide` | Re-enables canvas animator so hide animation plays |
| `SlideShowViewer.ShowCurrentSlide` | Hides slideshow image so custom BG shows through (skipped in black mode) |
| `MonoSingleton.PrepareAllForReload` | Cleans up overlay GameObjects and nulls stale pointers before reload |

---

## Custom Tip Implementation

See `docs/loading_tip.md` for full details on the tip system.

### Summary
- **Loading screen** (first `OnEnable` after init/reload): always shows the welcome tip with mod version.
- **Transitions** (subsequent `OnEnable` calls): 50% chance to override with a custom tip, 50% pass-through to game's server tip.
- Custom tips are weighted and the same tip is never shown twice in a row.
- Tip texts and weights are hardcoded in `loading_tip.cc`.

---

## Black Mode (`loader_transition_black`)

When `loader_transition_black = true`, the mod takes a minimal-interference approach:

- The game's **default black background** is left fully intact (no hide, no RectTransform reset, no overlay)
- The game's **slideshow** is left visible
- The game's **native logo** stays at its default position
- The game's **canvas animator** is not disabled
- **Only** the mod logo and CC logo overlays are added

This mode is automatically enabled when `loader_transition = false`. When `loader_transition = true` (default), `loader_transition_black` defaults to `false` and the full custom background is applied.

### Custom Texture Mode (`loader_transition = true`, `loader_transition_black = false`)

Full customization is applied:
- Game's BG image is hidden (alpha=0)
- BG RectTransform is reset to stretch-fill (game oversizes it for parallax bleed)
- Custom BG overlay is created with the loaded texture
- Native `LogoContainer` is repositioned to top-right
- Native `LoadingTipsContainer` is repositioned to lower-center
- Canvas animator is disabled (to prevent overriding child RT values at ShowComplete keyframes)
- Slideshow image is hidden
- Mod logo and CC logo overlays are added

---

## Config Options

| Option | Default | Description |
|--------|---------|-------------|
| `loader_enabled` | `true` | Replace login/loading screen background with custom texture + add logos |
| `loader_transition` | `true` | Enable transition screen customization. When `false`, `loader_transition_black` is forced to `true` |
| `loader_transition_black` | `false` | Use game's default black background. Only logos are added. Auto-forced to `true` when `loader_transition = false` |
| `loader_image` | `""` | Path to custom loading/transition image. Empty = use embedded fallback |
| `loader_logo_scale` | `1.0` | Scale multiplier for mod logo and CC logo |
| `loader_tip_enabled` | `true` | Show custom tips on loading/transition screens |

## Reload Handling

The mod hooks `MonoSingleton.PrepareAllForReload` to clean up before reload:
- Nulls all overlay GameObject pointers (Unity destroys them during reload)
- Resets texture pointers (textures are reloaded on next access)
- Resets loading screen, transition screen, and tip state

After reload, `TVC.Awake` fires for the new `TransitionViewController` and re-applies customization.

## Key Implementation Details

- Custom overlays are created as **siblings** under the root Canvas, not children of the TVC
- The canvas animator is disabled after show to prevent it from overriding child RectTransform positions at "ShowComplete" keyframes, and re-enabled before hide
- The BG RectTransform is reset to stretch-fill because the game oversizes it for parallax bleed
- Logo positioning scales proportionally with screen width (reference: 1920px)

## Game Classes Involved

From `Assembly-CSharp`, namespace `Digit.Prime.LoadingScreen`:

| Class | Role |
|-------|------|
| `TransitionManager` | Singleton state-machine. Key field: `_currentState` (offset 0x58, `int32_t` enum). Methods: `SetLoadingScreen()`, `Hide()` |
| `TransitionViewController` | MonoBehaviour on the transition UI prefab. Methods: `Awake`, `AboutToShow`, `AboutToHide` |
| `LoadingTipViewController` | Handles tip rotation. Method: `SetRandomTipLocalisedText()` |
| `SlideShowViewer` | Background slideshow during some transitions |

From `Assembly-CSharp`, namespace `Digit.Prime.Login`:

| Class | Role |
|-------|------|
| `LoginSequence` | Login flow controller. Field: `_mainCanvas`. Method: `Awake` |
