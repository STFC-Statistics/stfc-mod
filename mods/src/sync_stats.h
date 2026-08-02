#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

namespace SyncStats
{
struct TargetStats
{
  std::string                         name;
  size_t                              queue_depth = 0;
  size_t                              total_sent = 0;
  size_t                              total_errors = 0;
  size_t                              total_bytes = 0;
  std::chrono::steady_clock::time_point last_send;
  int                                 last_status_code = 0;
  bool                                active = false;
};

void RecordSend(const std::string& target, int status_code, size_t bytes);
void RecordError(const std::string& target);
void RecordQueue(const std::string& target, size_t depth);
std::vector<TargetStats> GetStats();
size_t GetTotalQueueDepth();
size_t GetTotalSent();
size_t GetTotalErrors();
}
