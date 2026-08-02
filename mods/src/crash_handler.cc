#include "crash_handler.h"
#include "file.h"

#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>

#if _WIN32
#include <Windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace CrashHandler
{
#if _WIN32

static char s_dump_path[1024] = {};
static LPTOP_LEVEL_EXCEPTION_FILTER s_previous_filter = nullptr;

static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* ep)
{
  static std::atomic<bool> handling{false};
  if (handling.exchange(true)) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  if (s_dump_path[0] != '\0') {
    HANDLE hFile = CreateFileA(s_dump_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
      MINIDUMP_EXCEPTION_INFORMATION mei{};
      mei.ThreadId          = GetCurrentThreadId();
      mei.ExceptionPointers = ep;
      mei.ClientPointers    = FALSE;

      MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                        MiniDumpWithDataSegs, ep != nullptr ? &mei : nullptr, nullptr, nullptr);
      CloseHandle(hFile);
    }
  }

  if (s_previous_filter != nullptr && s_previous_filter != UnhandledExceptionFilter) {
    return s_previous_filter(ep);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

void Install()
{
  const auto path = std::filesystem::path(File::Log()).replace_extension(".dmp").string();
  std::snprintf(s_dump_path, sizeof(s_dump_path), "%s", path.c_str());
  s_previous_filter = SetUnhandledExceptionFilter(UnhandledExceptionFilter);
  spdlog::info("Crash handler installed (Windows minidump): {}", s_dump_path);
}

#else

static char s_crash_path[1024] = {};

static size_t AppendUnsigned(char* buffer, size_t offset, size_t capacity, unsigned value)
{
  char digits[3];
  size_t count = 0;
  do {
    digits[count++] = static_cast<char>('0' + (value % 10));
    value /= 10;
  } while (value != 0 && count < sizeof(digits));

  while (count > 0 && offset < capacity) {
    buffer[offset++] = digits[--count];
  }
  return offset;
}

static void SignalHandler(int sig, siginfo_t*, void*)
{
  static volatile sig_atomic_t handling = 0;
  if (handling != 0) {
    _exit(128 + sig);
  }
  handling = 1;

  char   buf[128];
  size_t len = 0;
  constexpr char prefix[] = "[STFC Community Mod] Fatal signal ";
  constexpr char suffix[] = "\n";
  constexpr size_t prefix_len = sizeof(prefix) - 1;
  constexpr size_t suffix_len = sizeof(suffix) - 1;

  if (prefix_len < sizeof(buf)) {
    for (size_t i = 0; i < prefix_len; ++i) {
      buf[i] = prefix[i];
    }
    len = AppendUnsigned(buf, prefix_len, sizeof(buf) - suffix_len, static_cast<unsigned>(sig));
    if (len + suffix_len <= sizeof(buf)) {
      for (size_t i = 0; i < suffix_len; ++i) {
        buf[len + i] = suffix[i];
      }
      len += suffix_len;
    }
  }

  if (s_crash_path[0] != '\0') {
    int fd = open(s_crash_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
      if (len > 0) {
        write(fd, buf, len);
      }
      close(fd);
    }
  }

  if (len > 0) {
    write(STDERR_FILENO, buf, len);
  }

  struct sigaction sa{};
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sigaction(sig, &sa, nullptr);
  raise(sig);
  _exit(128 + sig);
}

void Install()
{
  const auto path = std::filesystem::path(File::Log()).replace_extension(".crash").string();
  std::snprintf(s_crash_path, sizeof(s_crash_path), "%s", path.c_str());

  struct sigaction sa{};
  sa.sa_sigaction = SignalHandler;
  sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGABRT, &sa, nullptr);
  sigaction(SIGBUS, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);
  sigaction(SIGFPE, &sa, nullptr);

  spdlog::info("Crash handler installed (macOS signal artifact): {}", s_crash_path);
}

#endif
}
