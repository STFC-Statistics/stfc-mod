#include "overlay_log.h"

namespace OverlayLog
{
static std::mutex             s_mutex;
static std::vector<LogEntry>  s_entries;

void Add(spdlog::level::level_enum level, const std::string& text)
{
  std::scoped_lock lk{s_mutex};
  s_entries.push_back({level, text});
  if (s_entries.size() > kMaxEntries) {
    s_entries.erase(s_entries.begin());
  }
}

std::vector<LogEntry> GetRecent(size_t count)
{
  std::scoped_lock lk{s_mutex};
  if (s_entries.size() <= count) {
    return s_entries;
  }
  return std::vector<LogEntry>(s_entries.end() - count, s_entries.end());
}

void Clear()
{
  std::scoped_lock lk{s_mutex};
  s_entries.clear();
}

size_t Count()
{
  std::scoped_lock lk{s_mutex};
  return s_entries.size();
}
}
