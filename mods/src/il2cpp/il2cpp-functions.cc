#include "il2cpp-functions.h"

#include <cstdio>

#if !_WIN32
#include <dlfcn.h>
#include <libgen.h>
#include <mach-o/dyld.h>

#define PATH_MAX 1024
#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus
#define DO_API(r, n, p) n##_t n;
#define DO_API_NO_RETURN(r, n, p) DO_API(r, n, p)
#include "il2cpp-api-functions.h"
#undef DO_API
#undef DO_API_NORETURN
#if defined(__cplusplus)
}
#endif // __cplusplus
#endif

bool init_il2cpp_pointers()
{
#if !_WIN32
  char     buf[PATH_MAX];
  uint32_t bufsize = PATH_MAX;
  if (_NSGetExecutablePath(buf, &bufsize) != 0) {
    return false;
  }

  char assembly_path[PATH_MAX];
  snprintf(assembly_path, sizeof(assembly_path), "%s/%s", dirname(buf), "../Frameworks/GameAssembly.dylib");
  auto assembly = dlopen(assembly_path, RTLD_LAZY | RTLD_GLOBAL);
  if (assembly == nullptr) {
    return false;
  }
#define DO_API(r, n, p) n = (n##_t)dlsym(assembly, #n);
#define DO_API_NO_RETURN(r, n, p) DO_API(r, n, p)
#include "il2cpp-api-functions.h"
#undef DO_API
#undef DO_API_NORETURN

  return il2cpp_init != nullptr && il2cpp_domain_get != nullptr && il2cpp_class_from_name != nullptr;
#else
  return true;
#endif
}
