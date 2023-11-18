#include "ntnative.h"

#ifdef __cplusplus
#    include <cstdio>
#else
#    include <stdio.h>
#endif

#if defined(__cplusplus) && defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L)
#    include "ntpebldr.h"
#endif // __cplusplus

int wmain()
{
#ifdef __cplusplus
    using namespace NT;
    wprintf(L"Windows: %u.%u (arch: %04X)\n", MajorVersion, MinorVersion, NativeProcessorArchitecture);
    wprintf(L"SystemRoot: %s\n", SystemRoot);
#else
    wprintf(L"Windows: %u.%u (arch: %04X)\n", *dwMajorVersion, *dwMinorVersion, *wNativeProcessorArchitecture);
    wprintf(L"SystemRoot: %s\n", wcsSystemRoot);
#endif
}
