// Logs host exceptions other than the ones the runtime expects (code, faulting address, native stack as
// exe-relative offsets) to crash.txt next to the exe. Symbolize with: llvm-symbolizer --obj=hexic.exe <offsets>.

#include <windows.h>

#include <cstdio>

namespace {

LONG WINAPI LogCrash(EXCEPTION_POINTERS* info) {
  DWORD code = info->ExceptionRecord->ExceptionCode;
  if (code == EXCEPTION_ACCESS_VIOLATION || code == 0x406D1388 || code == DBG_PRINTEXCEPTION_C ||
      code == DBG_PRINTEXCEPTION_WIDE_C || code == 0xE06D7363)  // MMIO, thread names, debug output, C++
    return EXCEPTION_CONTINUE_SEARCH;
  wchar_t path[MAX_PATH];
  DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
  while (n && path[n - 1] != L'\\') --n;
  wcscpy_s(path + n, MAX_PATH - n, L"crash.txt");
  FILE* f = _wfopen(path, L"a");
  if (!f) return EXCEPTION_CONTINUE_SEARCH;

  CONTEXT ctx = *info->ContextRecord;
  fprintf(f, "exception %08lX at %p\n", info->ExceptionRecord->ExceptionCode,
          info->ExceptionRecord->ExceptionAddress);
  for (int frame = 0; frame < 48 && ctx.Rip; ++frame) {
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(ctx.Rip), &module);
    char name[MAX_PATH] = "?";
    if (module) GetModuleFileNameA(module, name, MAX_PATH);
    fprintf(f, "  %s+0x%llX\n", strrchr(name, '\\') ? strrchr(name, '\\') + 1 : name,
            ctx.Rip - reinterpret_cast<DWORD64>(module));
    DWORD64 image_base;
    PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(ctx.Rip, &image_base, nullptr);
    if (!fn) {  // leaf function: return address is on top of the stack
      ctx.Rip = *reinterpret_cast<DWORD64*>(ctx.Rsp);
      ctx.Rsp += 8;
      continue;
    }
    void* handler_data;
    DWORD64 establisher;
    RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, ctx.Rip, fn, &ctx, &handler_data, &establisher, nullptr);
  }
  fclose(f);
  return EXCEPTION_CONTINUE_SEARCH;
}

const auto kInstalled = AddVectoredExceptionHandler(1, LogCrash);

}  // namespace
