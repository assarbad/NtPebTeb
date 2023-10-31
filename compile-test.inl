// wchar_t* const wcsSystemRoot = (wchar_t*)(MM_SHARED_USER_DATA_VA + 0x30);
// unsigned short const* const wNativeProcessorArchitecture = (unsigned short*)(MM_SHARED_USER_DATA_VA + 0x026a);
// unsigned long const* const dwMajorVersion = (unsigned long*)(MM_SHARED_USER_DATA_VA + 0x026c);
// unsigned long const* const dwMinorVersion = (unsigned long*)(MM_SHARED_USER_DATA_VA + 0x0270);

#include <Windows.h>

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif
#include "ntnative.h"

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
