#pragma once

#include <functional>

// Thread-safe queue for deferring IL2CPP/Unity API calls from the overlay's
// render-thread (D3D Present hook) callbacks to the Unity main thread.
//
// The overlay UI runs inside the D3D11 Present hook, which executes on the
// render thread. Many game/Unity APIs (UI instantiation, coroutines, etc.)
// are not safe to call off the main thread and will crash. Actions enqueued
// here are drained once per frame from a hook on ScreenManager::LateUpdate,
// which always runs on the Unity main thread.
namespace MainThreadQueue
{
// Installs the main-thread drain hook. Call once during Overlay::Install().
void Install();

// Enqueue an action to run on the next Unity main-thread tick.
// Safe to call from any thread.
void Enqueue(std::function<void()> action);
}
