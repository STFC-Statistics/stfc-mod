#pragma once

#include <spdlog/sinks/base_sink.h>

#include <mutex>
#include <string>
#include <vector>

namespace OverlayLog
{
struct LogEntry
{
  spdlog::level::level_enum level;
  std::string               text;
};

static constexpr size_t kMaxEntries = 500;

void                    Add(spdlog::level::level_enum level, const std::string& text);
std::vector<LogEntry>   GetRecent(size_t count = kMaxEntries);
void                    Clear();
size_t                  Count();
}

class OverlayLogSink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    spdlog::memory_buf_t formatted;
    base_sink<std::mutex>::formatter_->format(msg, formatted);
    OverlayLog::Add(msg.level, std::string(formatted.data(), formatted.size()));
  }

  void flush_() override {}
};
