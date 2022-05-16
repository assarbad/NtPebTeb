#pragma warning(push)
#pragma warning(disable : 4244 4389)
#include <gtest/gtest.h>
#pragma warning(pop)

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <Windows.h>
#include <cstring>
#include <tchar.h>

#ifdef NO_TEST_NAMESPACE
namespace
{
#endif // NO_TEST_NAMESPACE
#define NTPEBLDR_NAIVE_CRT_INLINES 1
#include "../ntpebldr.h"

    TEST(Crt, strlen)
    {
        ASSERT_EQ(NT::ntdll::crt::strlen_(nullptr), 0);
        ASSERT_EQ(NT::ntdll::crt::strlen_(""), 0);
        auto const* str1 = "Hello world!";
        ASSERT_EQ(NT::ntdll::crt::strlen_(str1), ::strlen(str1));
        auto const* str2 = L"Hello world!";
        ASSERT_EQ(NT::ntdll::crt::strlen_((char*)str2), ::strlen((char*)str2));
    }

    TEST(Crt, wcslen)
    {
        ASSERT_EQ(NT::ntdll::crt::wcslen_(nullptr), 0);
        ASSERT_EQ(NT::ntdll::crt::wcslen_(L""), 0);
        auto const* str1 = L"Hello world!";
        ASSERT_EQ(NT::ntdll::crt::wcslen_(str1), ::wcslen(str1));
        auto const* str2 = "Blablablablablablabla\0\0\0\0";
        ASSERT_EQ(NT::ntdll::crt::wcslen_((wchar_t*)str2), ::wcslen((wchar_t*)str2));
    }

    TEST(Crt, strncmp)
    {
        ASSERT_EQ(NT::ntdll::crt::strncmp_(nullptr, nullptr, 123), 0);
        ASSERT_EQ(NT::ntdll::crt::strncmp_("123", "234", 0), 0);

        ASSERT_LT(NT::ntdll::crt::strncmp_("123", "234", 3), 0);
        ASSERT_LT(NT::ntdll::crt::strncmp_("123", "abc", 3), 0);
        ASSERT_LT(NT::ntdll::crt::strncmp_("ABC", "abc", 3), 0);
        ASSERT_LT(NT::ntdll::crt::strncmp_("ABC\0", "abcd", 4), 0);
        ASSERT_LT(NT::ntdll::crt::strncmp_("ABCD", "abc\0", 4), 0);
    }

    TEST(Crt, wcsncmp)
    {
        ASSERT_EQ(NT::ntdll::crt::wcsncmp_(nullptr, nullptr, 123), 0);
        ASSERT_EQ(NT::ntdll::crt::wcsncmp_(L"123", L"234", 0), 0);

        ASSERT_LT(NT::ntdll::crt::wcsncmp_(L"123", L"234", 3), 0);
        ASSERT_LT(NT::ntdll::crt::wcsncmp_(L"123", L"abc", 3), 0);
        ASSERT_LT(NT::ntdll::crt::wcsncmp_(L"ABC", L"abc", 3), 0);
        ASSERT_LT(NT::ntdll::crt::wcsncmp_(L"ABC\0", L"abcd", 4), 0);
        ASSERT_LT(NT::ntdll::crt::wcsncmp_(L"ABCD", L"abc\0", 4), 0);
    }

    TEST(Crt, towlower)
    {
        for (size_t idx = 0; idx < 0x100; idx++)
        {
            SCOPED_TRACE("ASCII ordinal: " + std::to_string(idx));
            auto const chr = (WCHAR)(0xFFFF & idx);
            ASSERT_EQ(NT::ntdll::crt::towlower_(chr), ::towlower(chr));
        }
    }

    TEST(Crt, towupper)
    {
        for (size_t idx = 0; idx < 0x100; idx++)
        {
            SCOPED_TRACE("ASCII ordinal: " + std::to_string(idx));
            auto const chr = (WCHAR)(0xFFFF & idx);
            ASSERT_EQ(NT::ntdll::crt::towupper_(chr), ::towupper(chr));
        }
    }

    TEST(Crt, tolower)
    {
        for (size_t idx = 0; idx < 0x100; idx++)
        {
            SCOPED_TRACE("ASCII ordinal: " + std::to_string(idx));
            auto const chr = (CHAR)(0xFF & idx);
            ASSERT_EQ(NT::ntdll::crt::tolower_(chr), ::tolower(chr));
        }
    }

    TEST(Crt, toupper)
    {
        for (size_t idx = 0; idx < 0x100; idx++)
        {
            SCOPED_TRACE("ASCII ordinal: " + std::to_string(idx));
            auto const chr = (CHAR)(0xFF & idx);
            ASSERT_EQ(NT::ntdll::crt::toupper_(chr), ::toupper(chr));
        }
    }
#ifdef NO_TEST_NAMESPACE
} // namespace
#endif // NO_TEST_NAMESPACE
