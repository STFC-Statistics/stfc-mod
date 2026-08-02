#pragma once

#include <il2cpp/il2cpp_helper.h>
#include <str_utils.h>

struct DebugInfo {
public:
  static std::string UnityVersion()
  {
    static auto prop = get_class_helper().GetProperty("UnityVersion");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string AppVersion()
  {
    static auto prop = get_class_helper().GetProperty("AppVersion");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string ServerVersion()
  {
    static auto prop = get_class_helper().GetProperty("ServerVersion");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string StackName()
  {
    static auto prop = get_class_helper().GetProperty("StackName");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string CurrentServerInstanceName()
  {
    static auto prop = get_class_helper().GetProperty("CurrentServerInstanceName");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string HomeServerInstanceName()
  {
    static auto prop = get_class_helper().GetProperty("HomeServerInstanceName");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string UserId()
  {
    static auto prop = get_class_helper().GetProperty("UserId");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string QualitySettings()
  {
    static auto prop = get_class_helper().GetProperty("QualitySettings");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string Platform()
  {
    static auto prop = get_class_helper().GetProperty("Platform");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

  static std::string MemoryMode()
  {
    static auto prop = get_class_helper().GetProperty("MemoryMode");
    auto* str = prop.GetRaw<Il2CppString>(nullptr);
    return str ? to_string(str) : "";
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.Core", "DebugInfo");
    return class_helper;
  }
};
