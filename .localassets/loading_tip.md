# Loading Tip System

## Overview

The community mod overrides the game's loading/transition screen tips with custom messages. The system hooks `LoadingTipViewController` to inject tips at two distinct points in the game lifecycle.

## Source File

`mods/src/patches/parts/loading_tip.cc`

## Hooks

| Hook | Purpose |
|------|---------|
| `LoadingTipViewController.SetRandomTipLocalisedText` | Intercepts after the game sets its tip, replaces with custom text |
| `LoadingTipViewController.OnEnable` | Resets the tip counter at the start of each new transition cycle |

## Behavior

### Loading Screen (first OnEnable after init/reload)

Always shows the **welcome tip** — a fixed message containing the mod version string:

```
Welcome to Star Trek Fleet Command, Supported by the Community Mod v{version}! Please check our discord for the latest information!
```

After showing the welcome tip, `g_isLoadingScreen` flips to `false` so subsequent cycles use transition tips.

### Transitions (subsequent OnEnable calls)

- **50% chance**: Override with a custom tip (weighted random selection from the custom tip pool)
- **50% chance**: Pass-through — let the game's server tip stay as-is

The same custom tip is never shown twice in a row (tracked via `g_lastCustomTipIdx`).

## Custom Tips

Tips are hardcoded in `loading_tip.cc` as a weighted array (`kCustomTips[]`):

| Tip | Weight |
|-----|--------|
| Customize zoom presets, hotkeys, and UI scale in mod settings | 15 |
| Mod supports custom loading screens, transition backgrounds, borderless fullscreen | 10 |
| Press F11 to toggle borderless fullscreen and windowed mode | 15 |
| Visit https://stfc.pro for player rankings and stats | 50 |
| Join community Discord for bugs, features, and updates | 10 |

Higher weight = more likely to be selected.

## Config

| Option | Default | Description |
|--------|---------|-------------|
| `loader_tip_enabled` | `true` | Enable/disable custom tips. When disabled, the game's native tips are shown unmodified. |

## Reload Handling

`ResetLoadingTipState()` is called from `transition_screen.cc`'s `PrepareAllForReload` hook:

- Resets `g_tipCount` to 0
- Resets `g_isLoadingScreen` to `true` (so the welcome tip shows again after reload)
- Resets `g_lastCustomTipIdx` to `SIZE_MAX`

## Implementation Details

- The game's `SetRandomTipLocalisedText` is called first (picks a random server tip and sets it on `_textMeshPro`)
- The hook then overwrites the text by directly invoking `TMP_Text.set_text` on the `_textMeshPro` field (offset `0x20` on `LoadingTipViewController`)
- `OnEnable` resets the per-transition tip counter (`g_tipCount`) so the loading-screen-vs-transition distinction is maintained across transitions
- `g_isLoadingScreen` is managed in `SetRandomTipLocalisedText` — it stays `true` through the loading screen, then flips to `false` after the first tip is shown

## Game Class

`LoadingTipViewController` (`Assembly-CSharp`, namespace `Prime.LoadingScreen`)

| Field | Offset | Type |
|-------|--------|------|
| `_textMeshPro` | `0x20` | `TextMeshProUGUI` |

| Method | Purpose |
|--------|---------|
| `SetRandomTipLocalisedText()` | Picks a random tip and sets it on `_textMeshPro` |
| `OnEnable()` | Fires when the tip view controller is re-enabled (start of each transition) |
