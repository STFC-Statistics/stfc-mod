#pragma once

#include <il2cpp/il2cpp_helper.h>

#include "MonoSingleton.h"

class PlayerProfileManager : public MonoSingleton<PlayerProfileManager>
{
public:
  void FetchPlayerKillCounter(void* callbacks)
  {
    static auto method = get_class_helper().GetMethod<void(PlayerProfileManager*, void*)>("FetchPlayerKillCounter", 1);
    if (method) {
      method(this, callbacks);
    }
  }

private:
  friend struct MonoSingleton<PlayerProfileManager>;
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.PlayerProfile", "PlayerProfileManager");
    return class_helper;
  }
};
