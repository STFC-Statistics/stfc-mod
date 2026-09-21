#include "hook_health.h"

#include <spdlog/spdlog.h>

namespace HookHealth
{
static std::mutex             s_mutex;
static std::vector<Entry>     s_entries;
static thread_local const char* s_current_name = nullptr;

static Entry* Find(const char* name)
{
  for (auto& e : s_entries) {
    if (e.name == name) {
      return &e;
    }
  }
  return nullptr;
}

void Begin(const char* name)
{
  s_current_name = name;
}

void End()
{
  s_current_name = nullptr;
}

void MarkPartial(const char* error_msg)
{
  if (s_current_name == nullptr) {
    return;
  }

  std::scoped_lock lk{s_mutex};
  auto*            existing = Find(s_current_name);
  if (existing == nullptr) {
    Entry e;
    e.name      = s_current_name;
    e.status    = Status::Partial;
    e.error_msg = error_msg ? error_msg : "";
    s_entries.push_back(std::move(e));
    return;
  }

  if (existing->status != Status::Failed) {
    existing->status = Status::Partial;
  }
  if (error_msg) {
    existing->error_msg = error_msg;
  }
}

void Record(const char* name, Status status, const char* error_msg)
{
  std::scoped_lock lk{s_mutex};
  auto*            existing = Find(name);
  if (existing) {
    if (!(existing->status == Status::Partial && status == Status::Installed)) {
      existing->status = status;
    }
    if (error_msg) {
      existing->error_msg = error_msg;
    }
  } else {
    Entry e;
    e.name     = name;
    e.status   = status;
    if (error_msg) {
      e.error_msg = error_msg;
    }
    s_entries.push_back(std::move(e));
  }
}

void RecordError(const char* name, const char* error_msg)
{
  std::scoped_lock lk{s_mutex};
  auto*            existing = Find(name);
  if (existing) {
    existing->status = Status::Failed;
    if (error_msg) {
      existing->error_msg = error_msg;
    }
  } else {
    Entry e;
    e.name      = name;
    e.status    = Status::Failed;
    e.error_msg = error_msg ? error_msg : "";
    s_entries.push_back(std::move(e));
  }
}

void LogSummary()
{
  std::scoped_lock lk{s_mutex};

  auto installed = 0u, skipped = 0u, partial = 0u, failed = 0u, not_installed = 0u;
  for (const auto& e : s_entries) {
    switch (e.status) {
      case Status::Installed: installed++; break;
      case Status::Skipped: skipped++; break;
      case Status::Failed: failed++; break;
      case Status::Partial: partial++; break;
      default: not_installed++; break;
    }
  }

  spdlog::info("Hook health: {} installed, {} skipped, {} partial, {} failed, {} pending",
               installed, skipped, partial, failed, not_installed);

  if (partial > 0) {
    spdlog::warn("Partially installed hooks:");
    for (const auto& e : s_entries) {
      if (e.status == Status::Partial) {
        spdlog::warn("  {} : {}", e.name, e.error_msg.empty() ? "unknown error" : e.error_msg);
      }
    }
  }

  if (failed > 0) {
    spdlog::warn("Failed hooks:");
    for (const auto& e : s_entries) {
      if (e.status == Status::Failed) {
        spdlog::warn("  {} : {}", e.name, e.error_msg.empty() ? "unknown error" : e.error_msg);
      }
    }
  }
}

std::vector<Entry> GetEntries()
{
  std::scoped_lock lk{s_mutex};
  return s_entries;
}
}
