#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <exception>

#include "patches/patches.h"

__attribute__((constructor))
void myconstructor(int argc, const char **argv)
{
  const char* proc_name = (argc > 0 && argv != nullptr && argv[0] != nullptr) ? argv[0] : "(unknown)";
  syslog(LOG_ERR, "[+] STFC Community Mod dylib injected in %s", proc_name);
  try {
    ApplyPatches();
  } catch (const std::exception& e) {
    syslog(LOG_ERR, "[!] STFC Community Mod: failed to apply patches: %s", e.what());
  } catch (...) {
    syslog(LOG_ERR, "[!] STFC Community Mod: failed to apply patches: unknown exception");
  }
}

void* operator new[](size_t size, const char* /*name*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/,
                     int /*line*/)
{
  return malloc(size);
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*name*/, int /*flags*/,
                     unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
  return malloc(size);
}
