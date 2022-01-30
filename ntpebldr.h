///////////////////////////////////////////////////////////////////////////////
///
/// Using the PEB and TEB to navigate loaded modules and such trickery
///
///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2021, 2022 Oliver Schneider (assarbad.net)
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
#define __NTPEBLDR_H_VER__ 2022012523
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
#ifndef MM_SHARED_USER_DATA_VA
#define MM_SHARED_USER_DATA_VA                          ((unsigned char*)0x7ffe0000)
#define IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA MM_SHARED_USER_DATA_VA
#endif
    WCHAR const* SystemRoot = ((WCHAR*)(MM_SHARED_USER_DATA_VA + 0x30));
    USHORT const& NativeProcessorArchitecture =
        *((USHORT*)(MM_SHARED_USER_DATA_VA + 0x026a)); // PROCESSOR_ARCHITECTURE_AMD64 etc.
    ULONG const& MajorVersion = *((ULONG*)(MM_SHARED_USER_DATA_VA + 0x026c));
    ULONG const& MinorVersion = *((ULONG*)(MM_SHARED_USER_DATA_VA + 0x0270));
#endif

    using byte = unsigned char;

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

            STATIC_INLINE int strncmp_(char const* s1, char const* s2, size_t len)
            {
                if ((!s1 && !s2) || !len)
                {
                    return 0;
                }
                byte const* bs1 = (byte*)s1;
                byte const* bs2 = (byte*)s2;
                for (size_t idx = 0; idx <= len; idx++)
                {
                    auto const& b1 = bs1[idx];
                    auto const& b2 = bs2[idx];
                    if (b1 != b2)
                    {
                        return b1 - b2;
                    }
                    if (('\0' == b1) || ('\0' == b2))
                    {
                        return b1 - b2;
                    }
                }
                return 0;
            }

            STATIC_INLINE int wcsncmp_(wchar_t const* s1, wchar_t const* s2, size_t len)
            {
                if ((!s1 && !s2) || !len)
                {
                    return 0;
                }
                unsigned short const* bs1 = (unsigned short*)s1;
                unsigned short const* bs2 = (unsigned short*)s2;
                for (size_t idx = 0; idx <= len; idx++)
                {
                    auto const& b1 = bs1[idx];
                    auto const& b2 = bs2[idx];
                    if (b1 != b2)
                    {
                        return b1 - b2;
                    }
                    if ((L'\0' == b1) || (L'\0' == b2))
                    {
                        return b1 - b2;
                    }
                }
                return 0;
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
        using func_t = NTSTATUS(CALLBACK*)(NT::LDR_DATA_TABLE_ENTRY_CTX const&, T&);
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
                             &e->BaseDllName,
                             e->SizeOfImage,
                             e->Flags,
                             e->EntryPoint);
                    if (bShowFullName)
                    {
                        _tprintf(_T("         %wZ\n"), &e->FullDllName);
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
        namespace by_trait
        {
            typedef struct _MapByTrait
            {
                NTSTATUS Status;
                PVOID Address;
                PVOID DllBase;
                ULONG SizeOfImage;
            } MapByTrait;

            STATIC_INLINE NTSTATUS CALLBACK MapTraitPredicate(LDR_DATA_TABLE_ENTRY_CTX const& ldrctx, MapByTrait& data)
            {
                if (ldrctx.DllBase && ldrctx.SizeOfImage && (data.Address || data.DllBase))
                {
                    if ((data.DllBase) && (ldrctx.DllBase == data.DllBase))
                    {
                        if (!data.Address) // No address to look for given?
                        {
                            data.SizeOfImage = ldrctx.SizeOfImage;
                            return data.Status = STATUS_SUCCESS; // Found it!
                        }                                        // fall through into the other check
                    }
                    if (data.Address)
                    {
                        if (data.DllBase &&
                            (ldrctx.DllBase != data.DllBase)) // If we were passed a module, does it match?
                        {
                            return STATUS_NOT_FOUND; // Nope, so return failure early (will proceed to next ldr entry)
                        }
                        auto const* needle = (byte*)data.Address;
                        auto const* haystack_start = (byte*)ldrctx.DllBase;
                        auto const* haystack_end = haystack_start + ldrctx.SizeOfImage;
                        if ((needle >= haystack_start) && (needle <= haystack_end))
                        {
                            data.DllBase = ldrctx.DllBase;
                            data.SizeOfImage = ldrctx.SizeOfImage;
                            return data.Status = STATUS_SUCCESS;
                        }
                    }
                }
                return data.Status = STATUS_NOT_FOUND;
            }

            STATIC_INLINE HMODULE GetModHandleByAddress(PVOID Address)
            {
                MapByTrait context = {STATUS_UNSUCCESSFUL, Address, nullptr, 0};
                NTSTATUS Status = IteratePebLdrDataTable<MapByTrait>(MapTraitPredicate, context);
                if (NT_SUCCESS(Status))
                {
                    return (HMODULE)context.DllBase;
                }
                return nullptr;
            }

            STATIC_INLINE MapByTrait GetModTraits(HMODULE hMod)
            {
                MapByTrait context = {STATUS_UNSUCCESSFUL, nullptr, hMod, 0};
                (void)IteratePebLdrDataTable<MapByTrait>(MapTraitPredicate, context);
                return context;
            }
        } // namespace by_trait

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

            template <typename CTX, typename callback<CTX>::func_t Predicate>
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

        constexpr byte const* checked_cast(byte const* start, byte const* beyond, size_t size)
        {
            if ((beyond <= start) || (start + size >= beyond))
            {
                return nullptr;
            }
            return start;
        }

        template <typename T> constexpr T const* checked_cast(byte const* start, byte const* beyond)
        {
            if (!checked_cast(start, beyond, sizeof(T)))
            {
                return nullptr;
            }
            return reinterpret_cast<T const*>(start);
        }

        // Default to the 64-bit struct, as it is the bigger one
        // NB: takes a pre-populated MapByTrait struct
        STATIC_INLINE constexpr IMAGE_NT_HEADERS64 const* GetImageNtHeaders(by_trait::MapByTrait const& modtraits)
        {
            using byte = unsigned char;
            if (!NT_SUCCESS(modtraits.Status))
            {
                return nullptr;
            }
            byte const* mod = (byte*)modtraits.DllBase;
            byte const* const beyond = mod + modtraits.SizeOfImage;
            auto const* doshdr = checked_cast<IMAGE_DOS_HEADER>(mod, beyond);
            if (!doshdr || (IMAGE_DOS_SIGNATURE != doshdr->e_magic))
            {
                return nullptr;
            }
            auto const* nthdrs = checked_cast<IMAGE_NT_HEADERS64>(mod + doshdr->e_lfanew, beyond);
            if (!nthdrs || (IMAGE_NT_SIGNATURE != nthdrs->Signature))
            {
                return nullptr;
            }
            return nthdrs;
        }
    } // namespace predefined_helpers
    using predefined_helpers::by_string::MapByUnicodeString;
    using predefined_helpers::by_trait::MapByTrait;

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
        if (!DllName)
        {
            constexpr PebLdrOrder const order = PebLdrOrder::memory;
            auto const* head = GetPebLdrListHead(order);
            auto const* ldrentry = GetLdrDataTableEntry(head, order);
            return (HMODULE)ldrentry->DllBase;
        }
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

// unreliable in managed processes (mscoree takes its place when run from VSUnit), needs to be reviewed thoroughly
// (FIXME/TODO)
#if 0
    STATIC_INLINE HMODULE GetKernel32()
    {
        constexpr PebLdrOrder const order = PebLdrOrder::memory;
        auto const* head = GetPebLdrListHead(order);
        auto const* ldrentry = GetLdrDataTableEntry(head->Flink->Flink, order);
        return (HMODULE)ldrentry->DllBase;
    }
#endif // 0

    STATIC_INLINE HMODULE GetNtDll()
    {
        constexpr PebLdrOrder const order = PebLdrOrder::memory;
        auto const* head = GetPebLdrListHead(order);
        auto const* ldrentry = GetLdrDataTableEntry(head->Flink, order);
        return (HMODULE)ldrentry->DllBase;
    }

    STATIC_INLINE IMAGE_DATA_DIRECTORY const GetDataDirectory(IMAGE_NT_HEADERS64 const* nthdrs, DWORD datadir_index)
    {
        if (!nthdrs)
        {
            return {};
        }
        switch (nthdrs->FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
            {
                auto const* nthdrs32 = (IMAGE_NT_HEADERS32*)nthdrs;
                if (datadir_index < nthdrs32->OptionalHeader.NumberOfRvaAndSizes)
                {
                    return nthdrs32->OptionalHeader.DataDirectory[datadir_index];
                }
            }
        case IMAGE_FILE_MACHINE_AMD64:
            if (datadir_index < nthdrs->OptionalHeader.NumberOfRvaAndSizes)
            {
                return nthdrs->OptionalHeader.DataDirectory[datadir_index];
            }
        }
        return {};
    }

    STATIC_INLINE IMAGE_DATA_DIRECTORY const GetDataDirectory(HMODULE hMod, DWORD datadir_index)
    {
        using namespace predefined_helpers;
        auto const traits = by_trait::GetModTraits(hMod);
        if (!NT_SUCCESS(traits.Status))
        {
            return {};
        }
        auto const* nthdrs = GetImageNtHeaders(traits);
        return GetDataDirectory(nthdrs, datadir_index);
    }

    STATIC_INLINE IMAGE_EXPORT_DIRECTORY const* GetExportDirectory(HMODULE hMod)
    {
        using namespace predefined_helpers;
        auto const traits = by_trait::GetModTraits(hMod);
        if (!NT_SUCCESS(traits.Status))
        {
            return nullptr;
        }
        auto const* nthdrs = GetImageNtHeaders(traits);
        IMAGE_DATA_DIRECTORY const expdatadir = GetDataDirectory(nthdrs, IMAGE_DIRECTORY_ENTRY_EXPORT);

        if (expdatadir.Size && expdatadir.VirtualAddress)
        {
            auto const* const mod = (byte*)traits.DllBase;
            auto const* const modend = mod + traits.SizeOfImage;
            auto const* expdir = checked_cast<IMAGE_EXPORT_DIRECTORY>(&mod[expdatadir.VirtualAddress], modend);
            if (expdir && checked_cast((byte*)expdir, modend, expdatadir.Size))
            {
                return expdir;
            }
        }
        return nullptr;
    }

    STATIC_INLINE FARPROC GetProcAddress(HMODULE hMod, LPCSTR FuncName)
    {
        auto const* expdir = GetExportDirectory(hMod);

        if (!expdir || !FuncName)
        {
            return nullptr;
        }

        auto const* const mod = (byte*)hMod;
        // RVAs to the names of named exports
        auto const* const rvaNames = (ULONG*)&mod[expdir->AddressOfNames];
        // Parallel to the above, contains the index into AddressOfFunctions
        auto const* const rvaNameOrdinals = (USHORT*)&mod[expdir->AddressOfNameOrdinals];
        auto const* const rvaFunctions = (ULONG*)&mod[expdir->AddressOfFunctions];
        #if 0
        char const* const modName = (char*)&mod[expdir->Name];
        _tprintf(_T("[%hs] Base: 0x%08X; version: %u.%u\n"),
                 modName,
                 expdir->Base,
                 expdir->MajorVersion,
                 expdir->MinorVersion);
        #endif

        if (IS_INTRESOURCE(FuncName))
        {
            USHORT const idx = USHORT((ULONG_PTR)FuncName - expdir->Base);
            if (idx < expdir->NumberOfFunctions)
            {
                char const* const ExpFuncName = (char*)&mod[rvaNames[idx]];
#if 0
                    _tprintf(_T("Found ordinal idx %zu: %hs -> RVA:0x%08X\n"),
                             idx,
                             ExpFuncName,
                             rvaFunctions[idx);
#endif
                return (FARPROC)(rvaFunctions[idx] + mod);
            }
        }
        else
        {
            size_t const FuncNameLen = ntdll::crt::strlen_(FuncName);
            for (size_t idx = 0; idx < expdir->NumberOfNames; idx++)
            {
                char const* const ExpFuncName = (char*)mod + rvaNames[idx];
                if (0 == ntdll::crt::strncmp_(FuncName, ExpFuncName, FuncNameLen))
                {
                    auto const addridx = rvaNameOrdinals[idx];
                    #if 0
                    _tprintf(_T("Found name idx    %zu: %hs -> RVA:0x%08X\n"),
                             idx,
                             ExpFuncName,
                             rvaFunctions[addridx]);
                    #endif
                    return (FARPROC)(rvaFunctions[addridx] + mod);
                }
            }
        }

        return nullptr;
    }
} // namespace NT

#endif // __NTPEBLDR_H_VER__
