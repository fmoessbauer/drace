#include <dr_api.h>
#include <ipc/DrLock.h>
#include <util/DrNativeLoader.h>

namespace drace {
namespace util {

DrNativeLoader::DrNativeLoader() { _initialize(); }

DrNativeLoader::DrNativeLoader(const char* filename) : DrNativeLoader() {
  _load(filename);
}

DrNativeLoader::~DrNativeLoader() {
  if (loaded()) {
    _unload();
  }
  dr_free_module_data(_kernelModule);
  _kernelModule = nullptr;
}

::util::ProcedurePtr DrNativeLoader::operator[](const char* proc_name) const {
  typedef void* (*getProcAddress_t)(HMODULE, LPCSTR);
  return ::util::ProcedurePtr(
      getProcAddress_t(_getProcAddressFunc)(HMODULE(_lib), proc_name));
}

void DrNativeLoader::_initialize() {
  _kernelModule = dr_lookup_module_by_name("kernel32.dll");

  _loadLibraryFunc = dr_get_proc_address(_kernelModule->handle, "LoadLibraryA");
  _freeLibraryFunc = dr_get_proc_address(_kernelModule->handle, "FreeLibrary");
  _getProcAddressFunc =
      dr_get_proc_address(_kernelModule->handle, "GetProcAddress");
}

bool DrNativeLoader::_load(const char* filename) {
  typedef HMODULE (*loadLibrary_t)(LPCSTR);
  _lib = loadLibrary_t(_loadLibraryFunc)(filename);
  return loaded();
}

bool DrNativeLoader::_unload() {
  if (loaded()) {
    typedef BOOL (*freeLibrary_t)(HMODULE);
    return freeLibrary_t(_freeLibraryFunc)(HMODULE(_lib)) != FALSE;
  }
  return false;
}

}  // namespace util
}  // namespace drace
