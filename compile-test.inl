#include "ntnative.h"

#ifdef __cplusplus
#    include <cstdio>
#else
#    include <stdio.h>
#endif

#if (__cplusplus >= 201402L) || (_MSVC_LANG >= 201402L)
#    include "ntpebldr.hpp"
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
