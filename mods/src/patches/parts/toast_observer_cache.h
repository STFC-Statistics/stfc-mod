#pragma once

// Global cache of a valid ToastObserver instance, populated by the
// ToastObserver_EnqueueToast_Hook in disable_banners.cc whenever the
// game enqueues its own toast.  Other patches (e.g. kill_tracker) can
// read this pointer to enqueue custom toasts without having to search
// the scene themselves.
extern void* g_cached_toast_observer;
