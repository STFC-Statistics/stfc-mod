#include "activity_feed.h"

#include <algorithm>
#include <mutex>

namespace ActivityFeed
{
static std::mutex         s_mutex;
static std::vector<Entry> s_entries;

void Add(Category cat, std::string summary, std::string detail)
{
  std::scoped_lock lk{s_mutex};
  s_entries.push_back({std::chrono::system_clock::now(), cat, std::move(summary), std::move(detail)});
  if (s_entries.size() > kMaxEntries) {
    s_entries.erase(s_entries.begin());
  }
}

std::vector<Entry> GetRecent(size_t count)
{
  std::scoped_lock lk{s_mutex};
  if (s_entries.size() <= count) {
    return s_entries;
  }
  return std::vector<Entry>(s_entries.end() - count, s_entries.end());
}

std::vector<Entry> GetFiltered(const std::vector<Category>& filter, size_t count)
{
  std::scoped_lock lk{s_mutex};

  std::vector<Entry> result;
  result.reserve(std::min(count, s_entries.size()));

  // Iterate from most recent backwards so we get the latest `count` matching entries.
  for (auto it = s_entries.rbegin(); it != s_entries.rend() && result.size() < count; ++it) {
    if (std::find(filter.begin(), filter.end(), it->category) != filter.end()) {
      result.push_back(*it);
    }
  }

  // Reverse to chronological order (oldest first) for display.
  std::reverse(result.begin(), result.end());
  return result;
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

const char* CategoryName(Category c)
{
  switch (c) {
    case Category::Section: return "Section";
    case Category::Canvas:  return "Canvas";
    case Category::Toast:   return "Toast";
    case Category::Popup:   return "Popup";
    case Category::Chat:    return "Chat";
    case Category::Http:    return "HTTP";
    case Category::Rtc:     return "RTC";
    case Category::Proto:   return "Protobuf";
    default:                return "Unknown";
  }
}

const char* CategoryTag(Category c)
{
  switch (c) {
    case Category::Section: return "SECTION";
    case Category::Canvas:  return "CANVAS";
    case Category::Toast:   return "TOAST";
    case Category::Popup:   return "POPUP";
    case Category::Chat:    return "CHAT";
    case Category::Http:    return "HTTP";
    case Category::Rtc:     return "RTC";
    case Category::Proto:   return "PROTO";
    default:                return "?";
  }
}

const char* ToastStateName(int state)
{
  // Mirrors the ToastState enum in prime/Toast.h and the titles in notification_service.cc.
  switch (state) {
    case 0:  return "Standard";
    case 1:  return "FactionWarning";
    case 2:  return "FactionLevelUp";
    case 3:  return "FactionLevelDown";
    case 4:  return "FactionDiscovered";
    case 5:  return "IncomingAttack";
    case 6:  return "IncomingAttackFaction";
    case 7:  return "FleetBattle";
    case 8:  return "StationBattle";
    case 9:  return "StationVictory";
    case 10: return "Victory";
    case 11: return "Defeat";
    case 12: return "StationDefeat";
    case 14: return "Tournament";
    case 15: return "ArmadaCreated";
    case 16: return "ArmadaCanceled";
    case 17: return "ArmadaIncomingAttack";
    case 18: return "ArmadaBattleWon";
    case 19: return "ArmadaBattleLost";
    case 20: return "DiplomacyUpdated";
    case 21: return "JoinedTakeover";
    case 22: return "CompetitorJoinedTakeover";
    case 23: return "AbandonedTerritory";
    case 24: return "TakeoverVictory";
    case 25: return "TakeoverDefeat";
    case 26: return "TreasuryProgress";
    case 27: return "TreasuryFull";
    case 28: return "Achievement";
    case 29: return "AssaultVictory";
    case 30: return "AssaultDefeat";
    case 31: return "ChallengeComplete";
    case 32: return "ChallengeFailed";
    case 33: return "StrikeHit";
    case 34: return "StrikeDefeat";
    case 35: return "WarchestProgress";
    case 36: return "WarchestFull";
    case 37: return "PartialVictory";
    case 38: return "ArenaTimeLeft";
    case 39: return "ChainedEventScored";
    case 40: return "FleetPresetApplied";
    case 41: return "SurgeWarmUpEnded";
    case 42: return "SurgeHostileGroupDefeated";
    case 43: return "SurgeTimeLeft";
    case 44: return "QueueForLeaseActivated";
    case 45: return "QueueForLeaseExpired";
    case 46: return "PermanentQueuePurchased";
    case 47: return "OutpostStartedOrEnded";
    case 48: return "CrossAllianceArmadaVictory";
    case 49: return "CrossAllianceArmadaDefeat";
    case 50: return "CrossAllianceArmadaPartialVictory";
    case 51: return "FactionWeeklyEventsProgress";
    case 52: return "FactionWeeklyEventsComplete";
    case 53: return "ArmadaPlayerBlocked";
    case 54: return "ArmadaPlayerUnblocked";
    case 55: return "DynamicCrisisUpdate";
    case 56: return "DynamicCrisisFailed";
    case 57: return "DynamicCrisisCompleted";
    case 58: return "GalacticAnomalySystemEntered";
    case 59: return "ChapterCompleted";
    default: return "Unknown";
  }
}
}
