#pragma once

#include <il2cpp/il2cpp_helper.h>

struct Tracker {
public:
  int get_Counter()
  {
    return *reinterpret_cast<int*>(reinterpret_cast<char*>(this) + 0x18);
  }

  int get_Limit()
  {
    return *reinterpret_cast<int*>(reinterpret_cast<char*>(this) + 0x1c);
  }
};
