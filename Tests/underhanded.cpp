static_assert((_MSVC_LANG == 201402L) || (__cplusplus == 201402L), "Expecting exactly C++14 here");
#include <gtest/gtest.h>
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
#include "../underhanded-ntpebldr.hpp"
#ifdef NO_TEST_NAMESPACE
} // namespace
#endif // NO_TEST_NAMESPACE

class Underhanded : public ::testing::Test
{
  protected:
    void SetUp() override
    {
    }
};
