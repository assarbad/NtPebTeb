#pragma warning(push)
#pragma warning(disable : 4244 4389)
#include <gtest/gtest.h>
#pragma warning(pop)

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <Windows.h>
#include <cstring>
#include <tchar.h>
#include <tuple>

#ifdef NO_TEST_NAMESPACE
namespace
{
#endif // NO_TEST_NAMESPACE
#define NTPEBLDR_NAIVE_CRT_INLINES 1
#include "../ntpebldr.h"
#ifdef NO_TEST_NAMESPACE
} // namespace
#endif // NO_TEST_NAMESPACE

using NT::byte;
using std::wstring;
using strvec = std::vector<wstring>;

class KernelBase : public ::testing::Test
{
  protected:
    void SetUp() override
    {
    }
};

TEST_F(KernelBase, GetModuleHandleW)
{
    ASSERT_EQ(NT::GetModuleHandleW(nullptr), ::GetModuleHandleW(nullptr));

    strvec const names{L"ntdll.dll", L"kernelbase.dll", L"kernel32.dll"};
    wstring const SystemRoot(NT::SystemRoot);
    wstring SystemDir = SystemRoot + L"\\System32\\";

    for (auto const& name : names)
    {
        SCOPED_TRACE(name.c_str());
        ASSERT_TRUE(NT::GetModuleHandleW(name.c_str()) != nullptr);
        ASSERT_EQ(NT::GetModuleHandleW(name.c_str()), ::GetModuleHandleW(name.c_str()));

        wstring fullname = SystemDir + name;
        ASSERT_TRUE(NT::GetModuleHandleW(fullname.c_str()) != nullptr);
        ASSERT_EQ(NT::GetModuleHandleW(fullname.c_str()), ::GetModuleHandleW(fullname.c_str()));
    }
}

// module, image size, function name, observed ordinal, function pointer as retrieved by kernel32!GetProcAddress
using DllNameOrdinal = std::tuple<HMODULE, ULONG, std::string, USHORT, FARPROC>;
using NameOrdVec = std::vector<DllNameOrdinal>;
using DllMap = std::map<std::wstring, NameOrdVec>;

// Helper function to populate our DLL map
NameOrdVec RetrieveExportsByDllName(std::wstring dll)
{
    using namespace NT::predefined_helpers;
    SCOPED_TRACE(dll.c_str());
    HMODULE hMod = NT::GetModuleHandleW(dll.c_str());
    auto const traits = by_trait::GetModTraits(hMod);
    auto const* expdir = NT::GetExportDirectory(hMod);
    auto const* const mod = (byte*)hMod;
    auto const* const modBeyond = mod + traits.SizeOfImage;
    auto const* const rvaNames = (ULONG*)&mod[expdir->AddressOfNames];
    auto const* const rvaNameOrdinals = (USHORT*)&mod[expdir->AddressOfNameOrdinals];
    // auto const* const rvaFunctions = (ULONG*)&mod[expdir->AddressOfFunctions];
    NameOrdVec retval;
    // We have an upper estimate, so let's optimize
    retval.reserve(expdir->NumberOfFunctions);
    // auto const* const modname = (char*)&mod[expdir->Name];
    // _tprintf(_T("DLL name: %hs\n"), modname);
    for (size_t index = 0; index < expdir->NumberOfNames; index++)
    {
        // Retrieve RVA to the name of the function
        auto const nameRVA = rvaNames[index];
        // Compute VA from RVA
        auto const* const ExpFuncName = (char*)&mod[nameRVA];
        FARPROC funcptr = ::GetProcAddress(hMod, ExpFuncName);
        auto const Ordinal = (USHORT)rvaNameOrdinals[index] + (USHORT)expdir->Base;

        if (((byte*)funcptr > mod) && ((byte*)funcptr < modBeyond))
        {
            retval.emplace_back(hMod, traits.SizeOfImage, ExpFuncName, Ordinal, funcptr);
        }
    }
    return retval;
}

#define DLLMAP_ENTRY(s)                \
    {                                  \
        s, RetrieveExportsByDllName(s) \
    }

DllMap dllmap{
    DLLMAP_ENTRY(L"ntdll.dll"),
    DLLMAP_ENTRY(L"kernelbase.dll"),
    DLLMAP_ENTRY(L"kernel32.dll"),
};

class LdrFuncs : public testing::TestWithParam<DllNameOrdinal>
{
  public:
};

TEST_P(LdrFuncs, GetProcAddr)
{
    auto const& param = GetParam();
    HMODULE hMod = std::get<0>(param);
    auto const& FuncName = std::get<2>(param);
    auto const Ordinal = std::get<3>(param);
    auto const FuncPtrK32ByName = std::get<4>(param);
    // Our own GetProcAddress by name
    auto FuncPtrOurs = NT::GetProcAddress(hMod, FuncName.c_str());
    ASSERT_EQ(FuncPtrK32ByName, FuncPtrOurs);

    // kernel32!GetProcAddress with the ordinal we retrieved
    auto FuncPtrK32ByOrdinal = ::GetProcAddress(hMod, (LPCSTR)Ordinal);
    ASSERT_EQ(FuncPtrOurs, FuncPtrK32ByOrdinal);

    auto FuncPtrOursByOrdinal = NT::GetProcAddress(hMod, (LPCSTR)Ordinal);
    EXPECT_EQ(FuncPtrOurs, FuncPtrOursByOrdinal);
}

auto GetLdrFuncTestName = [](testing::TestParamInfo<DllNameOrdinal> const& info) { return std::get<2>(info.param); };

INSTANTIATE_TEST_SUITE_P(ntdll, LdrFuncs, testing::ValuesIn(dllmap[L"ntdll.dll"]), GetLdrFuncTestName);
INSTANTIATE_TEST_SUITE_P(kernelbase, LdrFuncs, testing::ValuesIn(dllmap[L"kernelbase.dll"]), GetLdrFuncTestName);
INSTANTIATE_TEST_SUITE_P(kernel32, LdrFuncs, testing::ValuesIn(dllmap[L"kernel32.dll"]), GetLdrFuncTestName);
