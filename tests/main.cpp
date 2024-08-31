
#include "common.h"

#include <iostream>

#if !defined(NDEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

static void terminateHandler()
{
    std::cerr << "std::terminate() called\n";

    std::abort();
}

int main(int argc, char** argv)
{
#if !defined(NDEBUG) && defined(_MSC_VER)
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif    

#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    std::set_terminate(terminateHandler);

    ::testing::InitGoogleTest(&argc, argv);

    auto r = RUN_ALL_TESTS();

    return r;
}
