#include "sync_stats.h"

#include <spdlog/spdlog.h>

#include <mutex>
#include <unordered_map>

namespace SyncStats
{
struct AtomicTargetStats
{
  std::string                             name;
  std::atomic<size_t>                     queue_depth{0};
  std::atomic<size_t>                     total_sent{0};
  std::atomic<size_t>                     total_errors{0};
  std::atomic<size_t>                     total_bytes{0};
  std::chrono::steady_clock::time_point   last_send;
  std::atomic<int>                        last_status_code{0};
  std::atomic<bool>                       active{false};
};

static std::mutex                                                          s_mutex;
static std::unordered_map<std::string, std::shared_ptr<AtomicTargetStats>> s_targets;

static std::shared_ptr<AtomicTargetStats> GetOrCreate(const std::string& name)
{
  std::scoped_lock lk{s_mutex};
  auto&            entry = s_targets[name];
  if (!entry) {
    entry    = std::make_shared<AtomicTargetStats>();
    entry->name = name;
  }
  return entry;
}

void RecordSend(const std::string& target, int status_code, size_t bytes)
{
  auto t = GetOrCreate(target);
  t->total_sent.fetch_add(1, std::memory_order_relaxed);
  t->total_bytes.fetch_add(bytes, std::memory_order_relaxed);
  t->last_status_code.store(status_code, std::memory_order_relaxed);
  t->last_send = std::chrono::steady_clock::now();
  t->active.store(false, std::memory_order_relaxed);
}

void RecordError(const std::string& target)
{
  auto t = GetOrCreate(target);
  t->total_errors.fetch_add(1, std::memory_order_relaxed);
  t->active.store(false, std::memory_order_relaxed);
}

void RecordQueue(const std::string& target, size_t depth)
{
  auto t = GetOrCreate(target);
  t->queue_depth.store(depth, std::memory_order_relaxed);
  t->active.store(true, std::memory_order_relaxed);
}

std::vector<TargetStats> GetStats()
{
  std::scoped_lock lk{s_mutex};
  std::vector<TargetStats> result;
  result.reserve(s_targets.size());
  for (const auto& [name, ptr] : s_targets) {
    TargetStats copy;
    copy.name             = ptr->name;
    copy.queue_depth      = ptr->queue_depth.load(std::memory_order_relaxed);
    copy.total_sent       = ptr->total_sent.load(std::memory_order_relaxed);
    copy.total_errors     = ptr->total_errors.load(std::memory_order_relaxed);
    copy.total_bytes      = ptr->total_bytes.load(std::memory_order_relaxed);
    copy.last_send        = ptr->last_send;
    copy.last_status_code = ptr->last_status_code.load(std::memory_order_relaxed);
    copy.active           = ptr->active.load(std::memory_order_relaxed);
    result.push_back(std::move(copy));
  }
  return result;
}

size_t GetTotalQueueDepth()
{
  std::scoped_lock lk{s_mutex};
  size_t total = 0;
  for (const auto& [name, ptr] : s_targets) {
    total += ptr->queue_depth.load(std::memory_order_relaxed);
  }
  return total;
}

size_t GetTotalSent()
{
  std::scoped_lock lk{s_mutex};
  size_t total = 0;
  for (const auto& [name, ptr] : s_targets) {
    total += ptr->total_sent.load(std::memory_order_relaxed);
  }
  return total;
}

size_t GetTotalErrors()
{
  std::scoped_lock lk{s_mutex};
  size_t total = 0;
  for (const auto& [name, ptr] : s_targets) {
    total += ptr->total_errors.load(std::memory_order_relaxed);
  }
  return total;
}
}
