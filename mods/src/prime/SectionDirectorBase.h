#pragma once

#include <il2cpp/il2cpp_helper.h>
#include "MonoSingleton.h"

struct SectionDirectorBase {
public:
  enum class State : int {
    NotLoaded = 0,
    PreLoaded = 1,
    Active = 2,
    Sleeping = 3,
    Idle = 4
  };

  int MySectionId()
  {
    static auto prop = get_class_helper().GetProperty("MySectionId");
    auto* raw = prop.GetRaw<Il2CppObject>(this);
    return raw ? *(int*)il2cpp_object_unbox(raw) : -1;
  }

  State MyState()
  {
    static auto prop = get_class_helper().GetProperty("MyState");
    auto* raw = prop.GetRaw<Il2CppObject>(this);
    return raw ? *(State*)il2cpp_object_unbox(raw) : State::NotLoaded;
  }

  static const char* StateToString(State s)
  {
    switch (s) {
      case State::NotLoaded:  return "NotLoaded";
      case State::PreLoaded:  return "PreLoaded";
      case State::Active:     return "Active";
      case State::Sleeping:   return "Sleeping";
      case State::Idle:       return "Idle";
      default:                return "Unknown";
    }
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.Sections", "SectionDirectorBase");
    return class_helper;
  }
};

struct SectionManager : MonoSingleton<SectionManager> {
public:
  int CurrentSection()
  {
    static auto prop = get_class_helper().GetProperty("CurrentSection");
    auto* raw = prop.GetRaw<Il2CppObject>(this);
    return raw ? *(int*)il2cpp_object_unbox(raw) : -1;
  }

  int PreviousSection()
  {
    static auto prop = get_class_helper().GetProperty("PreviousSection");
    auto* raw = prop.GetRaw<Il2CppObject>(this);
    return raw ? *(int*)il2cpp_object_unbox(raw) : -1;
  }

  bool IsSectionChangeInProgress()
  {
    static auto prop = get_class_helper().GetProperty("IsSectionChangeInProgress");
    auto* raw = prop.GetRaw<Il2CppObject>(this);
    return raw ? *(bool*)il2cpp_object_unbox(raw) : false;
  }

public:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.Sections", "SectionManager");
    return class_helper;
  }
};
