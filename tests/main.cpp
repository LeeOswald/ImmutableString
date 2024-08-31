
#include "common.h"

#if !defined(NDEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif


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

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
