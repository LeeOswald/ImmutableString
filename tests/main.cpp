
#include "common.h"

#include <csignal>
#include <cstring>
#include <iostream>

#if !defined(NDEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif


static void terminateHandler()
{
    std::cerr << "std::terminate() called\n";

    std::abort();
}

static void signalHandler(int signal)
{
    if (signal == SIGABRT)
        std::cerr << "SIGABRT received\n";
    else
        std::cerr << "Unexpected signal " << signal << " received\n";
    std::_Exit(EXIT_FAILURE);
}

int generate_benchmark(const std::string& file, unsigned long long words);
int run_benchmark(const std::string& file, unsigned long long words, unsigned runs);

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
    std::signal(SIGABRT, signalHandler);

    bool bench = false;
    unsigned long long bench_size_words = 1 * 1000 * 1000; 
    std::string data_file;
    bool generate = false;
    unsigned runs = 5;

    for (int i = 0; i < argc; ++i)
    {
        if (!std::strcmp(argv[i], "--benchmark"))
        {
            bench = true;
        }
        else if (!std::strcmp(argv[i], "--size"))
        {
            if (i + 1 < argc)
            {
                bench_size_words = std::strtoull(argv[i + 1], nullptr, 10);
                ++i;
            }
        }
        else if (!std::strcmp(argv[i], "--runs"))
        {
            if (i + 1 < argc)
            {
                runs = (unsigned)std::strtoull(argv[i + 1], nullptr, 10);
                ++i;
            }
        }
        else if (!std::strcmp(argv[i], "--generate"))
        {
            generate = true;
            if (i + 1 < argc)
            {
                data_file = argv[i + 1];
                ++i;
            }
        }
        else if (!std::strcmp(argv[i], "--load"))
        {
            if (i + 1 < argc)
            {
                data_file = argv[i + 1];
                ++i;
            }
        }
    }

    if (generate)
    {
        return generate_benchmark(data_file, bench_size_words);
    }

    if (bench)
    {
        return run_benchmark(data_file, bench_size_words, runs);
    }

    ::testing::InitGoogleTest(&argc, argv);
    auto r = RUN_ALL_TESTS();
    return r;
}
