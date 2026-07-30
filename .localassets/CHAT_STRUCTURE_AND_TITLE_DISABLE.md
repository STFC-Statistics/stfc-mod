# Chat System Structure & Player Title Disable Guide

## 1. Chat System Architecture

The chat system lives under `Digit.Prime.Chat` and follows a **manager-provider-controller** pattern with the following layers:

```
┌─────────────────────────────────────────────────────────────────┐
│                        ChatManager                               │
│  (MonoSingleton — central hub for channels, messages, pips)       │
│  ├─ Manages: _globalChannels[], _allianceChannels[],             │
│  │           _privateChannels[], _newbiesChannels[], etc.        │
│  ├─ Send/Receive: SendMessage(), RegisterChannelMessages()      │
│  ├─ History: GetMessageHistory(), PullHistory()                   │
│  └─ Social: MuteUser(), UnmuteUser(), Block management            │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│                     ChatService (cached)                         │
│  (Network layer — server API for send/receive/history)          │
└────────────────────────────┬────────────────────────────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
┌───────▼───────┐   ┌───────▼────────┐   ┌───────▼──────────────┐
│ ChatPreview   │   │ FullScreenChat │   │ ChatMessageListLocal │
│ Controller    │   │ ViewController │   │ ViewController       │
│ (HUD strip)   │   │ (Main chat UI) │   │ (Message list +    │
│               │   │                │   │  input field)        │
└───────────────┘   └────────────────┘   └──────────────────────┘
```

### 1.1 Channel Categories

`ChatChannelCategory` enum defines 7 channel types:

| Value | Name | Description |
|-------|------|-------------|
| `-1` | `None` | Not set |
| `0` | `Newbie` | New player help chat |
| `1` | `Global` | Server-wide global chat |
| `2` | `Alliance` | Alliance-only chat |
| `3` | `Private` | Private message list |
| `4` | `Private_Message` | Active private conversation |
| `5` | `Block` | Blocked users |
| `6` | `Regional` | Regional/language-specific |

### 1.2 View Modes

`ChatViewMode` enum:
- `Fullscreen = 0` — Full-screen chat with tabs, channel list, messages
- `Side = 1` — Side-frame overlay on top of gameplay

### 1.3 Key Controller Classes

| Class | Role | Key Fields |
|-------|------|------------|
| `ChatManager` | Central hub | `_unreadMessageCounts`, `_channelCategories`, `_privateChannelsSerializable` |
| `ChatPreviewController` | HUD swipeable preview strip | `_channelsPreviewData`, `_swipeController`, `_panelIndicators`, `_pip` |
| `FullScreenChatViewController` | Main chat UI | `_categoriesTabBarViewController`, `_channelsList`, `_messageList`, `_emojisPanel`, `_animator` |
| `ChatMessageListLocalViewController` | Message scroll list + input | `_visualMessageList`, `_scrollRect`, `_inputField`, `_sendMessageButton` |
| `ChatMessageWidget` | Individual message bubble | `_userProfileWidget`, `_chatBubble`, `_stateAnimator`, `_tooltipTrigger` |
| `ChatEmojisViewController` | Emoji picker panel | `_emojiList`, `_emojiSmartScrollerGrid`, `_searchInputField` |

### 1.4 Section Directors

All inherit from `ChatBaseSectionDirector`:
- `ChatMainSectionDirector` — Primary chat section
- `ChatNewbieSectionDirector` — Newbie chat onboarding
- `ChatPrivateMessageSectionDirector` — Private conversation view
- `ChatPrivateListSectionDirector` — Private message list
- `ChatBlockSectionDirector` — Blocked users management

---

## 2. Message Bubble Structure

A rendered chat message (`ChatMessageWidget`) consists of the following UI elements:

### 2.1 Profile Area (`_userProfileWidget` — `UserProfileWidget`)

| Element | Type | Description |
|---------|------|-------------|
| **Avatar Image** | `ImageSelector` | Player's portrait picture |
| **Avatar Frame** | `FrameAndAvatarWidget` | Decorative frame around portrait |
| **Player Name** | `TextLocalizer` | Sender's display name (dynamic style) |
| **Player Level** | `TextLocalizer` | Numeric level (e.g., "42") |
| **Server Name** | `TextLocalizer` | Home server identifier |
| **Player Title** | `PlayerTitleWidget` | Optional title/badge above/below name |
| **Server Rival Widget** | `ServerRivalWidget` | Rival server marker |
| **Diplomacy Symbol** | `LocaleTextContext` | Alliance diplomacy icon |
| **Animator** | `Animator` | Loading/state transitions |

### 2.2 Bubble Area (`_chatBubble` — `ChatBubble` inner class)

| Element | Type | Description |
|---------|------|-------------|
| **Sender Label Container** | `GameObject` | Wrapper for sender info row |
| **Player Name** | `TextLocalizer` | Name in bubble header |
| **Name Color** | `ColourSelector` | Color based on faction/diplomacy |
| **Alliance Name** | `TextMeshProUGUI` | Alliance tag (e.g., "[STFC]") |
| **Message Body** | `TextMeshProUGUI` | Actual chat text content |
| **Timestamp** | `ChatTimeWidget` | Message time (updates every 60s) |
| **Hit Area / Button** | `PositionButton` | Tap target for profile view |
| **RectTransform** | `RectTransform` | Layout positioning |

### 2.3 Widget-Level State

| Element | Type | Description |
|---------|------|-------------|
| **Bubble Type Animator** | `Animator` | System, Sent, Received, Error, ReceivedMentor |
| **Mentor Tag** | `Animator` parameter | Special styling for mentor messages |
| **Side Chat Flag** | `Animator` parameter | Compact layout for side mode |
| **Admin Avatar** | `FrameAndAvatarWidget` | System/admin avatar (replaces player avatar) |
| **Tooltip Trigger** | `TooltipActionTrigger` | Long-press menu (copy, block, report, bookmark) |
| **Action Settings** | `ActionLocaleSettingsList` | Context menus for normal/blocked/bookmark states |

### 2.4 Bubble Types (`AnimatorBubbleType`)

| Type | Description |
|------|-------------|
| `System = 0` | System messages (no player avatar) |
| `Sent = 1` | Your own messages (right-aligned) |
| `Received = 2` | Other player messages (left-aligned) |
| `Error = 3` | Failed to send (retry UI) |
| `ReceivedMentor = 4` | Mentor/tagged player messages |

### 2.5 Preview Strip (`ChatPreviewMessageWidget`)

The HUD preview strip is a **simplified** version showing only:
- `_text` — combined name + truncated message
- `_nameTextColor` — sender name color
- `_messageTextColor` — message body color

No avatar, no timestamp, no alliance name, no title in the preview.

---

## 3. How to Disable the Player Title in Chat

The **player title** is rendered by `PlayerTitleWidget` inside `UserProfileWidget`. `ChatMessageWidget` references a `_userProfileWidget` for the avatar/profile area of each message bubble.

### 3.1 Option A: Surgical Code Patch (Recommended)

Target: `ChatMessageWidget.SetWidgetData()` or `SetStateAnimation()`

Since `UserProfileWidget` exposes `ClearPlayerTitleIdentifier()` and `ClearPlayerTitleWidget()`, the cleanest approach is to clear the title immediately after the profile widget binds its data in the chat context.

**Patch location:** `Digit/Prime/Chat/ChatMessageWidget.cs`

Add to `SetWidgetData()` or `OnDidBindContext()`:

```csharp
if (_userProfileWidget != null)
{
    _userProfileWidget.ClearPlayerTitleIdentifier();
    // Or more aggressively:
    // _userProfileWidget.ClearPlayerTitleWidget();
}
```

**Pros:**
- Only affects chat messages; titles remain visible in all other UI (player profile popups, leaderboards, etc.)
- No prefab changes needed
- Safe and reversible

**Cons:**
- Requires patching the DLL / runtime

### 3.2 Option B: Element Mask Manipulation

`UserProfileWidget` uses an internal element bitmask (`_elementMask`) to control visible sub-elements:

```csharp
// From UserProfileWidget.cs Elements class
public const int Name = 1;
public const int Level = 2;
public const int Avatar = 4;
public const int Animator = 8;
public const int Server = 16;
public const int PlayerTitle = 32;   // <-- target
public const int Everything = 63;   // 1+2+4+8+16+32 = 63
```

If you can access the `_elementMask` field on the `_userProfileWidget` instance inside `ChatMessageWidget`, set it to exclude `PlayerTitle`:

```csharp
// In ChatMessageWidget after profile binding:
_userProfileWidget.SetElementMask(Elements.Everything & ~Elements.PlayerTitle);
// or if no setter exists, set the field directly via reflection/runtime patch
```

**Note:** The decompiled code does not show a public `SetElementMask()` method, so this would require either:
- Adding a public setter
- Using reflection at runtime
- Patching the `UpdatePlayerTitle()` method in `UserProfileWidget` to early-exit when the caller is chat-related

### 3.3 Option C: Prefab Modification

The `ChatMessageWidget` is instantiated from a prefab. If you have access to the Unity project or asset bundles:

1. Locate the `ChatMessageWidget` prefab
2. Find the `PlayerTitleWidget` GameObject under the `UserProfileWidget` child
3. **Disable** the `PlayerTitleWidget` GameObject (set `active = false`)

**Pros:**
- No code changes
- Immediate visual effect

**Cons:**
- Requires prefab/asset bundle access
- May be overwritten by game updates
- Cannot be toggled dynamically

### 3.4 Option D: Patch `UserProfileWidget.UpdatePlayerTitle()`

Target: `Digit/Prime/PlayerProfile/UserProfileWidget.cs`

Patch `UpdatePlayerTitle()` to return immediately:

```csharp
private void UpdatePlayerTitle()
{
    return; // No-op: titles never render anywhere
}
```

**Pros:**
- Single point of control
- Covers chat, profiles, leaderboards, etc.

**Cons:**
- **Global effect** — titles disappear from ALL UI, not just chat
- Only use if you want titles disabled game-wide

### 3.5 Option E: Data-Driven (Server-Side or Client Override)

If the player's title is sourced from a `UserProfile` or `PlayerContext` data object, you can:
- Set the title field to `null` or empty string before the chat widget binds
- Override the title data context in `ChatMessageWidget.SetWidgetData()`

This is the least invasive but depends on the data binding order.

---

## 4. Recommended Approach

For **disabling titles only in chat** while preserving them everywhere else:

> **Patch `ChatMessageWidget.SetWidgetData()` to call `_userProfileWidget.ClearPlayerTitleIdentifier()` immediately after the profile context binds.**

This is the most surgical, lowest-risk approach.

If you also need to hide titles from the **HUD preview strip**, note that `ChatPreviewMessageWidget` does **not** render titles at all — it only shows combined `name + truncated message` text, so no additional change is needed there.

---

## 5. Files Involved

| File | Path | Role |
|------|------|------|
| `ChatMessageWidget.cs` | `Digit/Prime/Chat/` | Message bubble renderer — **primary patch target** |
| `UserProfileWidget.cs` | `Digit/Prime/PlayerProfile/` | Profile widget with title element — **secondary target** |
| `PlayerTitleWidget.cs` | `Digit/Prime/PlayerAvatars/` | Title widget itself — **alternative target** |
| `ChatManager.cs` | `Digit/Prime/Chat/` | Central manager |
| `ChatPreviewController.cs` | `Digit/Prime/Chat/` | HUD preview strip |
| `FullScreenChatViewController.cs` | `Digit/Prime/Chat/` | Main chat UI |
| `ChatMessageListLocalViewController.cs` | `Digit/Prime/Chat/` | Message list + input |
