///////////////////////////////////////////////////////////////////////////////
///
/// Using the PEB and TEB to navigate loaded modules and such trickery
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
#ifndef __NTPEBLDR_H_VER__
#define __NTPEBLDR_H_VER__ 2021080900
#if (defined(_MSC_VER) && (_MSC_VER >= 1020)) || defined(__MCPP)
#pragma once
#endif // Check for "#pragma once" support
#include "./ntnative.h"

#ifndef __NAIVE_CRT_INLINES
#define __NAIVE_CRT_INLINES 1
#endif
#define STATIC_INLINE static inline
#if !__NAIVE_CRT_INLINES
#include <cstdio> // towupper/towlower et. al.
#endif
#include <cstddef> // offsetof etc
#ifdef _DEBUG
#include <tchar.h>
#endif // _DEBUG

namespace NT
{
// Must correspond to IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA from km/ntimage.h
// Kernel mode address is KI_USER_SHARED_DATA (on ARM64 this is relocatable!)
#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_AMD64))
#ifndef IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA
#define IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA ((unsigned char*)0x7ffe0000)
#endif
    WCHAR const* SystemRoot = ((WCHAR*)(IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA + 0x30));
    USHORT const& NativeProcessorArchitecture =
        *((USHORT*)(IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA + 0x026a)); // PROCESSOR_ARCHITECTURE_AMD64 etc.
    ULONG const& MajorVersion = *((ULONG*)(IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA + 0x026c));
    ULONG const& MinorVersion = *((ULONG*)(IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA + 0x0270));
#endif

    typedef struct _LDR_DATA_TABLE_ENTRY
    {
        LIST_ENTRY InLoadOrderModuleList;
        LIST_ENTRY InMemoryOrderModuleList;
        LIST_ENTRY InInitializationOrderModuleList;
        PVOID DllBase;
        PVOID EntryPoint;
        ULONG SizeOfImage;
        UNICODE_STRING FullDllName;
        UNICODE_STRING BaseDllName;
        ULONG Flags;
        USHORT ObsoleteLoadCount;
        USHORT TlsIndex;
        LIST_ENTRY HashLinks;
        ULONG TimeDateStamp;
        PVOID EntryPointActivationContext;
        PVOID Lock;
    } LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;
    static_assert(offsetof(LDR_DATA_TABLE_ENTRY, DllBase) == 3 * sizeof(LIST_ENTRY),
                  "DllBase offset has unexpected value");

    typedef struct
    {
        PVOID DllBase;
        PVOID EntryPoint;
        ULONG SizeOfImage;
        UNICODE_STRING FullDllName;
        UNICODE_STRING BaseDllName;
        ULONG Flags;
    } LDR_DATA_TABLE_ENTRY_CTX, *PLDR_DATA_TABLE_ENTRY_CTX;

    namespace Glue
    {
        typedef struct
        {
            LIST_ENTRY InLoadOrderModuleList;
            LIST_ENTRY InMemoryOrderModuleList;
            LIST_ENTRY InInitializationOrderModuleList;
            union
            {
                LDR_DATA_TABLE_ENTRY_CTX context;
                struct
                {
                    PVOID DllBase;
                    PVOID EntryPoint;
                    ULONG SizeOfImage;
                    UNICODE_STRING FullDllName;
                    UNICODE_STRING BaseDllName;
                    ULONG Flags;
                    USHORT ObsoleteLoadCount;
                    USHORT TlsIndex;
                    LIST_ENTRY HashLinks;
                    ULONG TimeDateStamp;
                    PVOID EntryPointActivationContext;
                    PVOID Lock;
                } real;
            } tail;
        } LDR_DATA_TABLE_ENTRY;
    } // namespace Glue
    static_assert(sizeof(Glue::LDR_DATA_TABLE_ENTRY) == sizeof(LDR_DATA_TABLE_ENTRY), "These two must match");
    static_assert(offsetof(LDR_DATA_TABLE_ENTRY, Flags) == offsetof(Glue::LDR_DATA_TABLE_ENTRY, tail.real.Flags),
                  "DllBase offset has unexpected value");
    static_assert(offsetof(Glue::LDR_DATA_TABLE_ENTRY, tail.context.Flags) ==
                      offsetof(Glue::LDR_DATA_TABLE_ENTRY, tail.real.Flags),
                  "DllBase offset has unexpected value");

    typedef struct _PEB_LDR_DATA
    {
        ULONG Length;
        BOOLEAN Initialized;
        HANDLE SsHandle;
        LIST_ENTRY InLoadOrderModuleList;
        LIST_ENTRY InMemoryOrderModuleList;
        LIST_ENTRY InInitializationOrderModuleList;
        PVOID EntryInProgress;
        BOOLEAN ShutdownInProgress;
        HANDLE ShutdownThreadId;
    } PEB_LDR_DATA, *PPEB_LDR_DATA;

    inline namespace ntdll
    {

        STATIC_INLINE TEB* NtCurrentTeb()
        {
#if defined(_WIN64) && defined(_M_X64)
            return (TEB*)__readgsqword(0x30);
#elif defined(_WIN32) && defined(_M_IX86)
            return (TEB*)__readfsdword(0x18);
#else
#error This isn't currently implemented on the current platform, it seems. Review the code, implement it and retry.
#endif
        }

        STATIC_INLINE PEB* RtlGetCurrentPeb() // officially with winver>=5.1
        {
#if defined(_WIN64) && defined(_M_X64)
            return (PEB*)__readgsqword(0x60);
#elif defined(_WIN32) && defined(_M_IX86)
            return (PEB*)__readfsdword(0x30);
#else
            return NtCurrentTeb()->ProcessEnvironmentBlock;
#endif
        }

        // Reimplementation of a few functions we don't want to import
        namespace crt
        {
            STATIC_INLINE size_t strlen_(char const* str)
            {
                if (!str)
                    return 0;
                size_t idx;
                for (idx = 0; str[idx]; idx++)
                    ;
                return idx;
            }

            STATIC_INLINE size_t wcslen_(wchar_t const* str)
            {
                if (!str)
                    return 0;
                size_t idx;
                for (idx = 0; str[idx]; idx++)
                    ;
                return idx;
            }

#if __NAIVE_CRT_INLINES
            // Certainly naive and incomplete, but in all likelihood more than sufficient
            // for most purposes we're after here
            STATIC_INLINE WCHAR towlower_(WCHAR ch)
            {
                if ((L'A' <= ch) && (ch <= L'Z'))
                    ch += 0x20;
                return ch;
            }
            STATIC_INLINE WCHAR towupper_(WCHAR ch)
            {
                if ((L'a' <= ch) && (ch <= L'z'))
                    ch -= 0x20;
                return ch;
            }
            STATIC_INLINE CHAR tolower_(CHAR ch)
            {
                if (('A' <= ch) && (ch <= 'Z'))
                    ch += 0x20;
                return ch;
            }
            STATIC_INLINE CHAR toupper_(CHAR ch)
            {
                if (('a' <= ch) && (ch <= 'z'))
                    ch -= 0x20;
                return ch;
            }
#else
            // The right side is _intentionally_ without namespace, so a preprocessor
            // define could still be used to point these elsewhere ...
            wint_t (*towlower_)(wint_t) = towlower;
            wint_t (*towupper_)(wint_t) = towupper;
            int (*tolower_)(int) = tolower;
            int (*toupper_)(int) = toupper;
#endif
        } // namespace crt

        // This is not exactly optimized, but should be a halfway faithful implementation of the original functionality
        STATIC_INLINE LONG RtlCompareUnicodeString(PCUNICODE_STRING String1,
                                                   PCUNICODE_STRING String2,
                                                   BOOLEAN CaseInSensitive)
        {
            PCWSTR pwstr1 = String1->Buffer;
            PCWSTR pwstr2 = String2->Buffer;
            LONG const len1 = String1->Length >> 1; // number of WCHAR
            LONG const len2 = String2->Length >> 1; // number of WCHAR
            LONG const minlen = (len1 <= len2) ? len1 : len2;

            if (CaseInSensitive)
            {
                for (LONG idx = 0; idx < minlen; idx++)
                {
                    if (pwstr1[idx] != pwstr2[idx])
                    {
                        auto const c1 = ntdll::crt::towupper_(pwstr1[idx]);
                        auto const c2 = ntdll::crt::towupper_(pwstr2[idx]);
                        if (c1 != c2)
                        {
                            return (LONG)c1 - (LONG)c2;
                        }
                    }
                }
            }
            else
            {
                for (LONG idx = 0; idx < minlen; idx++)
                {
                    if (pwstr1[idx] != pwstr2[idx])
                    {
                        return (LONG)pwstr1[idx] - (LONG)pwstr2[idx];
                    }
                }
            }
            return len1 - len2;
        }

        // This is not exactly optimized, but should be a halfway faithful implementation of the original functionality
        STATIC_INLINE VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString, LPCWSTR SourceString)
        {
            DestinationString->Buffer = const_cast<LPWSTR>(SourceString);
            if (SourceString)
            {
                size_t const idx = ntdll::crt::wcslen_(SourceString);
                // TBD: should we check against 0x8000? Does the actual ntdll implementation do it?
                DestinationString->Length = (USHORT)idx * sizeof(SourceString[idx]);
                DestinationString->MaximumLength = DestinationString->Length + sizeof(SourceString[idx]);
            }
            else
            {
                DestinationString->Length = DestinationString->MaximumLength = 0;
            }
        }
    } // namespace ntdll

    STATIC_INLINE PEB_LDR_DATA* GetPebLdr()
    {
        PEB* peb = ntdll::RtlGetCurrentPeb();
        if (peb)
        {
            return (PEB_LDR_DATA*)peb->Ldr;
        }
        return nullptr;
    }

    enum class PebLdrOrder : unsigned char
    {
        load,
        memory,
        init,
    };

    STATIC_INLINE LIST_ENTRY const* GetPebLdrListHead(PEB_LDR_DATA const* ldrdata, PebLdrOrder order)
    {
        if (!ldrdata)
        {
            return nullptr;
        }
        LIST_ENTRY const* first = nullptr;
        switch (order)
        {
        case PebLdrOrder::load:
            return ldrdata->InLoadOrderModuleList.Flink;
        case PebLdrOrder::memory:
            return ldrdata->InMemoryOrderModuleList.Flink;
        case PebLdrOrder::init:
            return ldrdata->InInitializationOrderModuleList.Flink;
        }
        return nullptr;
    }

    STATIC_INLINE LIST_ENTRY const* GetPebLdrListHead(PebLdrOrder order)
    {
        PEB_LDR_DATA const* ldrdata = GetPebLdr();
        return GetPebLdrListHead(ldrdata, order);
    }

    STATIC_INLINE LDR_DATA_TABLE_ENTRY const* GetLdrDataTableEntry(LIST_ENTRY const* current, PebLdrOrder order)
    {
        switch (order)
        {
        case PebLdrOrder::load:
            return CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
        case PebLdrOrder::memory:
            return CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InMemoryOrderModuleList);
        case PebLdrOrder::init:
            return CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);
        }
        return nullptr;
    }

    STATIC_INLINE LDR_DATA_TABLE_ENTRY_CTX const* GetLdrDataTableEntryPredicateContext(LIST_ENTRY const* current,
                                                                                       PebLdrOrder order)
    {
        auto const* entry = GetLdrDataTableEntry(current, order);
        if (entry)
        {
            return &((Glue::LDR_DATA_TABLE_ENTRY*)entry)->tail.context;
        }
        return nullptr;
    }

    template <typename T, PebLdrOrder order_v = PebLdrOrder::load> struct callback
    {
        static PebLdrOrder const order = order_v;
        using data_t = T;
        using func_t = NTSTATUS(CALLBACK*)(NT::LDR_DATA_TABLE_ENTRY_CTX const&, typename T&);
    };

    template <typename T>
    STATIC_INLINE NTSTATUS IteratePebLdrDataTable(typename callback<T>::func_t predicate,
                                                  typename callback<T>::data_t& context)
    {
        constexpr PebLdrOrder const order = callback<T>::order;
        auto const* first = GetPebLdrListHead(order);

        if (!first)
        {
            return STATUS_INVALID_HANDLE;
        }
        auto const* current = first;

        do
        {
            auto const* tblentry = GetLdrDataTableEntryPredicateContext(current, order);
            NTSTATUS Status;
            if (STATUS_NOT_FOUND != (Status = predicate(*tblentry, context)))
            {
                return Status;
            }
            current = current->Flink;
        } while (current != first);
        return STATUS_NO_MORE_ENTRIES; // we've reached the list end
    }

#if defined(PRINT_FUNCS) || defined(_DEBUG)
    inline namespace print_helpers
    {
        STATIC_INLINE void print_ldr_entry_ctx(LDR_DATA_TABLE_ENTRY_CTX const& ldrctx)
        {
            _tprintf(_T("  PVOID DllBase = @%p;\n"), ldrctx.DllBase);
            _tprintf(_T("  PVOID EntryPoint = @%p;\n"), ldrctx.EntryPoint);
            _tprintf(_T("  ULONG SizeOfImage = %u (0x%08X);\n"), ldrctx.SizeOfImage, ldrctx.SizeOfImage);

            _tprintf(_T("  UNICODE_STRING FullDllName = \"%wZ\";\n"), &ldrctx.FullDllName);
            _tprintf(_T("  UNICODE_STRING BaseDllName = \"%wZ\";\n"), &ldrctx.BaseDllName);

            _tprintf(_T("  ULONG Flags = 0x%08X);\n"), ldrctx.Flags);
        }

        STATIC_INLINE void print_ldr_entry(LDR_DATA_TABLE_ENTRY const& ldrentry, bool nolinks = false)
        {
            _tprintf(_T("((LDR_DATA_TABLE_ENTRY*)@%p)\n"), &ldrentry);

            if (!nolinks)
            {
                _tprintf(_T("  LIST_ENTRY InLoadOrderModuleList = {Flink = @%p, Blink = @%p};\n"),
                         ldrentry.InLoadOrderModuleList.Flink,
                         ldrentry.InLoadOrderModuleList.Blink);
                _tprintf(_T("  LIST_ENTRY InMemoryOrderModuleList = {Flink = @%p, Blink = @%p};\n"),
                         ldrentry.InMemoryOrderModuleList.Flink,
                         ldrentry.InMemoryOrderModuleList.Blink);
                _tprintf(_T("  LIST_ENTRY InInitializationOrderModuleList = {Flink = @%p, Blink = @%p};\n"),
                         ldrentry.InInitializationOrderModuleList.Flink,
                         ldrentry.InInitializationOrderModuleList.Blink);
            }

            auto const& ldrctx = ((Glue::LDR_DATA_TABLE_ENTRY*)&ldrentry)->tail.context;
            print_ldr_entry_ctx(ldrctx);

            _tprintf(_T("  ULONG TimeDateStamp = 0x%08X;\n"), ldrentry.TimeDateStamp);
        }

        STATIC_INLINE void print_linked_list(LIST_ENTRY const* first, PebLdrOrder order, BOOLEAN bShowFullName = FALSE)
        {
            // GetLdrDataTableEntry(current);
            auto const* current = first;
            ULONG idx = 0;
            do
            {
                auto const* e = GetLdrDataTableEntryPredicateContext(current, order);
                if (e && e->DllBase && e->SizeOfImage && e->BaseDllName.Buffer && e->FullDllName.Buffer)
                {
                    _tprintf(_T("    [% 2u] @%p, %wZ: s = %u, f = 0x%08X, ep = @%p\n"),
                             idx,
                             e->DllBase,
                             e->BaseDllName,
                             e->SizeOfImage,
                             e->Flags,
                             e->EntryPoint);
                    if (bShowFullName)
                    {
                        _tprintf(_T("         %wZ\n"), e->FullDllName);
                    }
                }
                current = current->Flink;
                idx++;
            } while (current != first);
        }

        STATIC_INLINE void print_ldr_data(PEB_LDR_DATA const& ldrdata, bool nolinks = false)
        {
            _tprintf(_T("((PEB_LDR_DATA*)@%p)\n"), &ldrdata);

            _tprintf(_T("  ULONG Length = %u (0x%08X);\n"), ldrdata.Length, ldrdata.Length);
            _tprintf(_T("  BOOLEAN Initialized = %u;\n"), ldrdata.Initialized);
            _tprintf(_T("  HANDLE SsHandle = @%p;\n"), ldrdata.SsHandle);

            if (!nolinks)
            {
                _tprintf(_T("  LIST_ENTRY InLoadOrderModuleList = {Flink = @%p, Blink = @%p};\n"),
                         ldrdata.InLoadOrderModuleList.Flink,
                         ldrdata.InLoadOrderModuleList.Blink);
                print_linked_list(ldrdata.InLoadOrderModuleList.Flink, PebLdrOrder::load);
                _tprintf(_T("  LIST_ENTRY InMemoryOrderModuleList = {Flink = @%p, Blink = @%p};\n"),
                         ldrdata.InMemoryOrderModuleList.Flink,
                         ldrdata.InMemoryOrderModuleList.Blink);
                print_linked_list(ldrdata.InMemoryOrderModuleList.Flink, PebLdrOrder::memory);
                _tprintf(_T("  LIST_ENTRY InInitializationOrderModuleList = {Flink = @%p, Blink = @%p};\n"),
                         ldrdata.InInitializationOrderModuleList.Flink,
                         ldrdata.InInitializationOrderModuleList.Blink);
                print_linked_list(ldrdata.InInitializationOrderModuleList.Flink, PebLdrOrder::init);
            }
        }
    } // namespace print_helpers
#endif

    namespace predefined_helpers
    {
        namespace by_string
        {
            typedef struct _MapByUnicodeString
            {
                HMODULE hMod = nullptr;
                UNICODE_STRING const& usMod; // could be name or path
            } MapByUnicodeString;

            STATIC_INLINE NTSTATUS CALLBACK MapByBaseDllNamePredicate(LDR_DATA_TABLE_ENTRY_CTX const& ldrctx,
                                                                      MapByUnicodeString& data)
            {
                auto const& DllName = ldrctx.BaseDllName;
                if (ldrctx.DllBase && ldrctx.SizeOfImage && DllName.Buffer && DllName.Length)
                {
                    if (0 == ntdll::RtlCompareUnicodeString(&data.usMod, &DllName, TRUE))
                    {
                        data.hMod = (HMODULE)ldrctx.DllBase;
                        return STATUS_SUCCESS;
                    }
                }
                return STATUS_NOT_FOUND;
            }

            STATIC_INLINE NTSTATUS CALLBACK MapByFullDllNamePredicate(LDR_DATA_TABLE_ENTRY_CTX const& ldrctx,
                                                                      MapByUnicodeString& data)
            {

                auto const& DllName = ldrctx.FullDllName;
                if (ldrctx.DllBase && ldrctx.SizeOfImage && DllName.Buffer && DllName.Length)
                {
                    if (0 == ntdll::RtlCompareUnicodeString(&data.usMod, &DllName, TRUE))
                    {
                        data.hMod = (HMODULE)ldrctx.DllBase;
                        return STATUS_SUCCESS;
                    }
                }
                return STATUS_NOT_FOUND;
            }

            template <typename CTX, typename callback<typename CTX>::func_t Predicate>
            STATIC_INLINE HMODULE GetModHandle(UNICODE_STRING const& DllName)
            {
                CTX context = {nullptr, DllName};
                NTSTATUS Status = IteratePebLdrDataTable<CTX>(Predicate, context);
                if (NT_SUCCESS(Status))
                {
                    return context.hMod;
                }
                return nullptr;
            }
        } // namespace by_string
    }     // namespace predefined_helpers

    STATIC_INLINE HMODULE GetModHandleByBaseName(UNICODE_STRING const& DllName)
    {
        using namespace predefined_helpers::by_string;
        return GetModHandle<MapByUnicodeString, MapByBaseDllNamePredicate>(DllName);
    }

    STATIC_INLINE HMODULE GetModHandleByFullName(UNICODE_STRING const& DllName)
    {
        using namespace predefined_helpers::by_string;
        return GetModHandle<MapByUnicodeString, MapByFullDllNamePredicate>(DllName);
    }

    STATIC_INLINE HMODULE GetModuleHandleW(LPCWSTR DllName)
    {
        UNICODE_STRING usMod{0, 0, 0};
        ntdll::RtlInitUnicodeString(&usMod, DllName);
        HMODULE hMod = GetModHandleByBaseName(usMod);
        if (!hMod)
        {
            hMod = GetModHandleByFullName(usMod);
            // TBD: further logic to deal with missing .dll suffix needed?
        }
        return hMod;
    }

#if 0 // unreliable in managed processes, needs to be reviewed thoroughly (FIXME/TODO)
    STATIC_INLINE HMODULE GetKernel32()
    {
        constexpr PebLdrOrder const order = PebLdrOrder::memory;
        auto const* head = GetPebLdrListHead(order);
        auto const* ldrentry = GetLdrDataTableEntry(head->Flink->Flink, order);
        return (HMODULE)ldrentry->DllBase;
    }
#endif // 0 // unreliable in managed processes, needs to be reviewed thoroughly (FIXME/TODO)

    STATIC_INLINE HMODULE GetNtDll()
    {
        constexpr PebLdrOrder const order = PebLdrOrder::memory;
        auto const* head = GetPebLdrListHead(order);
        auto const* ldrentry = GetLdrDataTableEntry(head->Flink, order);
        return (HMODULE)ldrentry->DllBase;
    }

    STATIC_INLINE FARPROC GetProcAddress(HMODULE hMod, LPCSTR FuncName)
    {
        if (IS_INTRESOURCE(FuncName))
        {
            // By ordinal
        }
        else
        {
            // By name
        }
        // RtlAllocateHeap
        // RtlProcessHeap
        // RtlCopyMemory
        // RtlFreeHeap
        return nullptr;
    }
} // namespace NT

#endif // __NTPEBLDR_H_VER__
