#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ActivityFeed
{
enum class Category : uint8_t
{
  Section, // screen/section transitions
  Canvas,  // canvas show/hide
  Toast,   // banner notifications
  Popup,   // interstitial modals
  Chat,    // chat messages
  Http,    // HTTP sync requests/responses
  Rtc,     // realtime (Centrifugo) payloads
  Proto,   // protobuf entity group parsing
};

struct Entry
{
  std::chrono::system_clock::time_point timestamp;
  Category                              category;
  std::string                           summary; // one-line display text
  std::string                           detail;  // optional multi-line detail (for drill-down)
};

static constexpr size_t kMaxEntries = 1000;

void                  Add(Category cat, std::string summary, std::string detail = "");
std::vector<Entry>    GetRecent(size_t count = kMaxEntries);
std::vector<Entry>    GetFiltered(const std::vector<Category>& filter, size_t count = kMaxEntries);
void                  Clear();
size_t                Count();
const char*           CategoryName(Category c);
const char*           CategoryTag(Category c); // short tag: "SECTION", "HTTP", etc.
const char*           ToastStateName(int state);
}
