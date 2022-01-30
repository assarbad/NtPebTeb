///////////////////////////////////////////////////////////////////////////////
///
/// Small program to learn about the PEB and TEB on various Windows versions
///
///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2021 Oliver Schneider (assarbad.net)
///
/// Permission is hereby granted, free of charge, to any person obtaining a
/// copy of this software and associated documentation files (the "Software"),
/// to deal in the Software without restriction, including without limitation
/// the rights to use, copy, modify, merge, publish, distribute, sublicense,
/// and/or sell copies of the Software, and to permit persons to whom the
/// Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
/// DEALINGS IN THE SOFTWARE.
///
///////////////////////////////////////////////////////////////////////////////

#include <Windows.h>
#include <cstdio>
#include <tchar.h>
#ifdef _DEBUG
#include <crtdbg.h>
#else
#define _ASSERTE(...)    \
    do                   \
    {                    \
        if (__VA_ARGS__) \
        {                \
        }                \
    } while (0)
#endif // _DEBUG

#include "ntpebldr.h"

int _tmain(int argc, _TCHAR** argv)
{
    _tprintf(_T("SystemRoot = %ws\n"), NT::SystemRoot);
    _tprintf(_T("NT = %u.%u (arch: %u)\n"), NT::MajorVersion, NT::MinorVersion, NT::NativeProcessorArchitecture);
    // _ASSERTE(NT::GetKernel32() == ::GetModuleHandleW(L"kernel32.dll"));
    _ASSERTE(NT::GetNtDll() == ::GetModuleHandleW(L"ntdll.dll"));
    // _tprintf(_T("Module #1 ntdll   : 0x%08p\n"), NT::GetKernel32());
    _tprintf(_T("Module #2 kernel32: 0x%08p\n"), NT::GetNtDll());
    auto* ldrdata = NT::GetPebLdr();
    if (ldrdata)
    {
#ifdef _DEBUG
        print_ldr_data(*ldrdata);
#endif // _DEBUG

        auto const* name = L"kernelbase.dll";
        HMODULE hMod = NT::GetModuleHandleW(name);
        _ASSERTE(hMod == ::GetModuleHandleW(name));

        _tprintf(_T("Module handle retrieved for %ws: 0x%08p\n"), name, hMod);

        FARPROC pFunc1Native = NT::GetProcAddress(hMod, "GetProcAddress");
        FARPROC pFunc2Native = NT::GetProcAddress(hMod, (LPCSTR)682U);
        _tprintf(_T("via name   : 0x%08p (PEB)\n"), pFunc1Native);
        _tprintf(_T("via ordinal: 0x%08p (PEB)\n"), pFunc2Native);

        FARPROC pFunc1Win32 = ::GetProcAddress(hMod, "GetProcAddress");
        FARPROC pFunc2Win32 = ::GetProcAddress(hMod, (LPCSTR)682U);
        _tprintf(_T("via name   : 0x%08p (Win32)\n"), pFunc1Win32);
        _tprintf(_T("via ordinal: 0x%08p (Win32)\n"), pFunc2Win32);
    }
    else
    {
        _ftprintf(stderr, _T("Failed to retrieve loader data from PEB!\n"));
    }
    return 0;
}
