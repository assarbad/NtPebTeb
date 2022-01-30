#include <gtest/gtest.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstring>
#include <tchar.h>

namespace
{
#define __NAIVE_CRT_INLINES 1
#include "../ntpebldr.h"
} // namespace

class Ntdll : public ::testing::Test
{
  protected:
    HMODULE hNtdll = nullptr;
    LONG(NTAPI* RtlCompareUnicodeString)(PCUNICODE_STRING, PCUNICODE_STRING, BOOLEAN) = nullptr;
    VOID(NTAPI* RtlInitUnicodeString)(PUNICODE_STRING, LPCWSTR) = nullptr;
    PEB*(NTAPI* RtlGetCurrentPeb)() = nullptr;

    void SetUp() override
    {
        if (!hNtdll)
        {
            hNtdll = ::GetModuleHandle(_T("ntdll.dll"));
        }
        EXPECT_TRUE(hNtdll != nullptr);
        RtlCompareUnicodeString = (decltype(RtlCompareUnicodeString))GetProcAddress(hNtdll, "RtlCompareUnicodeString");
        RtlInitUnicodeString = (decltype(RtlInitUnicodeString))GetProcAddress(hNtdll, "RtlInitUnicodeString");
        RtlGetCurrentPeb = (decltype(RtlGetCurrentPeb))GetProcAddress(hNtdll, "RtlGetCurrentPeb");

        ASSERT_TRUE(RtlCompareUnicodeString != nullptr);
        ASSERT_TRUE(RtlInitUnicodeString != nullptr);
        ASSERT_TRUE(RtlGetCurrentPeb != nullptr);
    }
};

TEST_F(Ntdll, RtlCompareUnicodeString)
{
    auto testComparison =
        [=](PCUNICODE_STRING s1, PCUNICODE_STRING s2, BOOLEAN caseInSensitive, auto predicate)
    {
        LONG theirs = RtlCompareUnicodeString(s1, s2, caseInSensitive);
        LONG ours = NT::ntdll::RtlCompareUnicodeString(s1, s2, caseInSensitive);
        ASSERT_TRUE(predicate(ours)) << "Our implementation glitched";
        ASSERT_TRUE(predicate(theirs)) << "Their implementation glitched [Huh?!]";
        ASSERT_EQ(ours, theirs) << "Expected our and their implementation to match!";
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

TEST_F(Ntdll, RtlInitUnicodeString)
{
    UNICODE_STRING ntSystemRootNtDll{};
    UNICODE_STRING ntSystemRootOurs{};

    RtlInitUnicodeString(&ntSystemRootNtDll, NT::SystemRoot);
    NT::ntdll::RtlInitUnicodeString(&ntSystemRootOurs, NT::SystemRoot);

    ASSERT_EQ(memcmp(&ntSystemRootNtDll, &ntSystemRootOurs, sizeof(UNICODE_STRING)), 0);
}

TEST_F(Ntdll, NtCurrentTeb)
{
    ASSERT_EQ((PVOID)::NtCurrentTeb(), (PVOID)NT::ntdll::NtCurrentTeb());
}

TEST_F(Ntdll, RtlGetCurrentPeb)
{
    ASSERT_EQ(NT::ntdll::RtlGetCurrentPeb(), this->RtlGetCurrentPeb());
}

TEST_F(Ntdll, GetNtDll)
{
    ASSERT_TRUE(NT::GetNtDll() != nullptr);
    ASSERT_EQ(NT::GetNtDll(), ::GetModuleHandleW(L"ntdll.dll"));
}

TEST_F(Ntdll, GetPebLdr)
{
    ASSERT_TRUE(NT::GetPebLdr() != nullptr);
    ASSERT_EQ((void*)NT::GetPebLdr(), (void*)RtlGetCurrentPeb()->Ldr);
}

using NT::PebLdrOrder;

TEST_F(Ntdll, GetPebLdrListHead)
{
    NT::PEB_LDR_DATA* pebldr = NT::GetPebLdr();
    LIST_ENTRY const* init = NT::GetPebLdrListHead(pebldr, PebLdrOrder::init);
    LIST_ENTRY const* load = NT::GetPebLdrListHead(pebldr, PebLdrOrder::load);
    LIST_ENTRY const* memr = NT::GetPebLdrListHead(pebldr, PebLdrOrder::memory);

    ASSERT_EQ((void*)pebldr->InInitializationOrderModuleList.Flink, (void*)init);
    ASSERT_EQ((void*)pebldr->InLoadOrderModuleList.Flink, (void*)load);
    ASSERT_EQ((void*)pebldr->InMemoryOrderModuleList.Flink, (void*)memr);
}
