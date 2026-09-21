#pragma once

#include "str_utils.h"
#include "hook_health.h"

#include <il2cpp/il2cpp-functions.h>

#include <spdlog/spdlog.h>

#if _WIN32
#include <winrt/Windows.Foundation.h>
#endif

namespace ErrorMsg
{
static auto MissingMethod(const char* classname, const char* methodname)
{
  HookHealth::MarkPartial("missing method");
  spdlog::error("Unable to find method '{}->{}'", classname, methodname);
}

static void MissingStaticMethod(const char* classname, const char* methodname)
{
  HookHealth::MarkPartial("missing static method");
  spdlog::error("Unable to find method '{}::{}'", classname, methodname);
}

static void MissingHelper(const char* namespacename, const char* classname)
{
  HookHealth::MarkPartial("missing class helper");
  spdlog::error("Unable to find helper '{}.{}'", namespacename, classname);
}

static void SyncMsg(const char* section, const std::string& msg)
{
  spdlog::error("Failed to send {} sync data: {}", section, msg);
}

static void SyncMsg(const char* section, const std::wstring& msg)
{
  spdlog::error("Failed to send {} sync data: {}", section, to_string(msg));
}

static void SyncRuntime(const char* section, const std::runtime_error& e)
{
  spdlog::error("Runtime error sending {} sync data: {}", section, e.what());
}

static void SyncException(const char* section, const std::exception& e)
{
  spdlog::error("Exception sending {} sync data: {}", section, e.what());
}

static void Il2CppException(const char* context, Il2CppException* exc)
{
  if (!exc) {
    return;
  }
  char msg[1024] = {};
  il2cpp_format_exception(exc, msg, sizeof(msg));
  spdlog::error("IL2CPP exception in {}: {}", context, msg);
}

#if _WIN32
static void SyncWinRT(const char* section, winrt::hresult_error const& ex)
{
  spdlog::error("WINRT Error sending {} sync data: {}", section, winrt::to_string(ex.message()));
}
#endif
};
