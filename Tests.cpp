#include "CppUnitTest.h"
#include <Windows.h>
#define __NAIVE_CRT_INLINES 1
#include "ntpebldr.h"

namespace Microsoft
{
    namespace VisualStudio
    {
        namespace CppUnitTestFramework
        {
            template <> inline std::wstring ToString<unsigned short>(unsigned short const& t)
            {
                RETURN_WIDE_STRING(t);
            }

            template <> inline std::wstring ToString<UNICODE_STRING>(UNICODE_STRING const& t)
            {
                RETURN_WIDE_STRING(t.Buffer);
            }
        } // namespace CppUnitTestFramework
    }     // namespace VisualStudio
} // namespace Microsoft

using AssertX = Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

template <>
static void AssertX::AreEqual<UNICODE_STRING>(UNICODE_STRING const& expected,
                                              UNICODE_STRING const& actual,
                                              const wchar_t* message,
                                              const __LineInfo* pLineInfo)
{
    FailOnCondition(expected.Length == actual.Length, EQUALS_MESSAGE(expected, actual, message), pLineInfo);
    FailOnCondition(CppUnitStrCmpW(reinterpret_cast<const unsigned short*>(expected.Buffer),
                                   reinterpret_cast<const unsigned short*>(actual.Buffer),
                                   TRUE),
                    EQUALS_MESSAGE(expected, actual, message),
                    pLineInfo);
}

namespace Tests
{
    typedef PVOID(NTAPI* RtlGetCurrentPeb_t)();
    typedef VOID(NTAPI* RtlInitUnicodeString_t)(PUNICODE_STRING, LPCWSTR);
    typedef LONG(NTAPI* RtlCompareUnicodeString_t)(PCUNICODE_STRING, PCUNICODE_STRING, BOOLEAN);
    HMODULE hNtDll = nullptr;
    NTNATIVE_FUNC(RtlGetCurrentPeb) = nullptr;
    NTNATIVE_FUNC(RtlInitUnicodeString) = nullptr;
    NTNATIVE_FUNC(RtlCompareUnicodeString) = nullptr;
    using PebLdrOrder = NT::PebLdrOrder;
    using PEB_LDR_DATA = NT::PEB_LDR_DATA;

    TEST_MODULE_INITIALIZE (setUp)
    {
        hNtDll = ::GetModuleHandleW(L"ntdll.dll");
        Assert::IsNotNull(hNtDll, L"Expecting ntdll.dll to be loaded into the current process");

        RtlGetCurrentPeb = NTNATIVE_GETFUNC(hNtDll, RtlGetCurrentPeb);
        AssertX::IsNotNull(RtlGetCurrentPeb, L"Expecting ntdll!RtlGetCurrentPeb to be available");

        RtlInitUnicodeString = NTNATIVE_GETFUNC(hNtDll, RtlInitUnicodeString);
        AssertX::IsNotNull(RtlInitUnicodeString, L"Expecting ntdll!RtlInitUnicodeString to be available");

        RtlCompareUnicodeString = NTNATIVE_GETFUNC(hNtDll, RtlCompareUnicodeString);
        AssertX::IsNotNull(RtlCompareUnicodeString, L"Expecting ntdll!RtlCompareUnicodeString to be available");
    }

    TEST_CLASS (TestPEBTEB)
    {
      public:
        TEST_METHOD (TestNtCurrentTeb)
        {
            AssertX::AreEqual((PVOID)::NtCurrentTeb(),
                              (PVOID)NT::ntdll::NtCurrentTeb(),
                              L"We should get the same TEB value as the official implementation");
        }

        TEST_METHOD (TestRtlGetCurrentPeb)
        {
            AssertX::AreEqual((PVOID)RtlGetCurrentPeb(),
                              (PVOID)NT::ntdll::RtlGetCurrentPeb(),
                              L"We should get the same PEB value as the official implementation");
        }

        TEST_METHOD (Test_strlen)
        {
            AssertX::AreEqual((size_t)0, NT::ntdll::crt::strlen_(nullptr));
            AssertX::AreEqual((size_t)0, NT::ntdll::crt::strlen_(""));
            AssertX::AreEqual((size_t)3, NT::ntdll::crt::strlen_("123"));
            AssertX::AreEqual((size_t)10, NT::ntdll::crt::strlen_("foobarbaz\n"));
        }

        TEST_METHOD (Test_wcslen)
        {
            AssertX::AreEqual((size_t)0, NT::ntdll::crt::wcslen_(nullptr));
            AssertX::AreEqual((size_t)0, NT::ntdll::crt::wcslen_(L""));
            AssertX::AreEqual((size_t)3, NT::ntdll::crt::wcslen_(L"123"));
            AssertX::AreEqual((size_t)10, NT::ntdll::crt::wcslen_(L"foobarbaz\n"));
        }

        TEST_METHOD (Test_toupperlower)
        {
            for (size_t idx = 0; idx < 0x80; idx++)
            {
                CHAR const ch = (CHAR)(idx & 0x7F);
                AssertX::AreEqual((int)::toupper(ch),
                                  (int)NT::ntdll::crt::toupper_(ch),
                                  L"[toupper] We should yield the same for ASCII proper");
                AssertX::AreEqual((int)::tolower(ch),
                                  (int)NT::ntdll::crt::tolower_(ch),
                                  L"[tolower] We should yield the same for ASCII proper");
            }
        }

        TEST_METHOD (Test_towupperlower)
        {
            for (size_t idx = 0; idx < 0x80; idx++)
            {
                WCHAR const ch = (WCHAR)(idx & 0x7F);
                AssertX::AreEqual((int)::towupper(ch),
                                  (int)NT::ntdll::crt::towupper_(ch),
                                  L"[towupper] We should yield the same for ASCII proper");
                AssertX::AreEqual((int)::towlower(ch),
                                  (int)NT::ntdll::crt::towlower_(ch),
                                  L"[towlower] We should yield the same for ASCII proper");
            }
        }

        TEST_METHOD (TestRtlInitUnicodeString)
        {
            UNICODE_STRING ntSystemRootNtDll{};
            UNICODE_STRING ntSystemRootOurs{};

            RtlInitUnicodeString(&ntSystemRootNtDll, NT::SystemRoot);
            RtlInitUnicodeString(&ntSystemRootOurs, NT::SystemRoot);

            AssertX::AreEqual(ntSystemRootNtDll, ntSystemRootOurs, L"The results should match");

            WCHAR const a1[] = L"BLAFOOBARBAZ";
            UNICODE_STRING const s1 = RTL_CONSTANT_STRING(L"FOOBARBAZ");
            UNICODE_STRING s2{};
            RtlInitUnicodeString(&s2, &a1[3]);
            AssertX::AreEqual(s1, s2, L"The results should match");
        }

        TEST_METHOD (TestRtlCompareUnicodeString)
        {
            auto testComparison = [](PCUNICODE_STRING s1, PCUNICODE_STRING s2, BOOLEAN caseInSensitive, auto predicate)
            {
                LONG theirs = RtlCompareUnicodeString(s1, s2, caseInSensitive);
                LONG ours = NT::ntdll::RtlCompareUnicodeString(s1, s2, caseInSensitive);
                AssertX::IsTrue(predicate(ours), L"Our implementation glitched");
                AssertX::IsTrue(predicate(theirs), L"Their implementation glitched [Huh?!]");
                AssertX::AreEqual(ours, theirs, L"Expected our and their implementation to match!");
            };

            UNICODE_STRING const foo = RTL_CONSTANT_STRING(L"foo");
            UNICODE_STRING const foobar = RTL_CONSTANT_STRING(L"foobar");
            UNICODE_STRING const bar = RTL_CONSTANT_STRING(L"bar");

            testComparison(&foo, &foo, FALSE, [](auto x) { return x == 0; });
            testComparison(&foo, &foobar, FALSE, [](auto x) { return x < 0; });
            testComparison(&foo, &bar, FALSE, [](auto x) { return x > 0; });

            UNICODE_STRING const FOO = RTL_CONSTANT_STRING(L"FOO");
            UNICODE_STRING const FOOBAR = RTL_CONSTANT_STRING(L"FOOBAR");
            UNICODE_STRING const BAR = RTL_CONSTANT_STRING(L"BAR");

            testComparison(&foo, &FOO, TRUE, [](auto x) { return x == 0; });
            testComparison(&foo, &FOOBAR, TRUE, [](auto x) { return x < 0; });
            testComparison(&foo, &BAR, TRUE, [](auto x) { return x > 0; });
        }

        TEST_METHOD (TestGetPebLdr)
        {
            PEB* peb = (PEB*)RtlGetCurrentPeb();
            PEB_LDR_DATA* pebldr_ntdll = (PEB_LDR_DATA*)peb->Ldr;
            AssertX::AreEqual(
                (PVOID)pebldr_ntdll, (PVOID)NT::GetPebLdr(), L"Expecting to get the same address for PEB::Ldr pointer");
        }

        TEST_METHOD (TestGetPebLdrListHead)
        {
            PEB_LDR_DATA* pebldr = NT::GetPebLdr();
            LIST_ENTRY const* init = NT::GetPebLdrListHead(pebldr, PebLdrOrder::init);
            LIST_ENTRY const* load = NT::GetPebLdrListHead(pebldr, PebLdrOrder::load);
            LIST_ENTRY const* memr = NT::GetPebLdrListHead(pebldr, PebLdrOrder::memory);

            AssertX::AreEqual(
                (PVOID)pebldr->InInitializationOrderModuleList.Flink, (PVOID)init, L"Init order pointer must match");
            AssertX::AreEqual(
                (PVOID)pebldr->InLoadOrderModuleList.Flink, (PVOID)load, L"Load order pointer must match");
            AssertX::AreEqual(
                (PVOID)pebldr->InMemoryOrderModuleList.Flink, (PVOID)memr, L"In-memory order pointer must match");
        }

#if 0
        TEST_METHOD (TestGetKernel32)
        {
            HMODULE theirs = ::GetModuleHandleW(L"kernel32.dll");
            HMODULE ours = NT::GetKernel32();
            AssertX::AreEqual((PVOID)theirs, (PVOID)ours, L"The module handles for kernel32 should match");
        }
#endif // 0

        TEST_METHOD (TestGetNtDll)
        {
            HMODULE theirs = ::GetModuleHandleW(L"ntdll.dll");
            HMODULE ours = NT::GetNtDll();
            AssertX::AreEqual((PVOID)theirs, (PVOID)ours, L"The module handles for ntdll should match");
        }

        TEST_METHOD (TestGetModuleHandleW)
        {
            HMODULE theirs = ::GetModuleHandleW(L"kernelbase.dll");
            HMODULE ours = NT::GetModuleHandleW(L"kernelbase.dll");
            AssertX::AreEqual((PVOID)theirs, (PVOID)ours, L"The module handles for kernelbase should match");
        }
    };
} // namespace Tests
