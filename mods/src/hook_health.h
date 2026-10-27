#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace HookHealth
{
enum class Status : uint8_t
{
  NotInstalled = 0,
  Installed    = 1,
  Skipped      = 2,
  Failed       = 3,
  Partial      = 4,
};

struct Entry
{
  std::string name;
  Status      status{Status::NotInstalled};
  std::string error_msg;
};

void Begin(const char* name);
void End();
void MarkPartial(const char* error_msg = nullptr);
void Record(const char* name, Status status, const char* error_msg = nullptr);
void RecordError(const char* name, const char* error_msg);
void LogSummary();
std::vector<Entry> GetEntries();
}
