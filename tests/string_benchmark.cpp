#include "common.h"

#include <immutable_string/string.hxx>

#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

static size_t generateWord(std::ostringstream& ss)
{
    static const char ValidChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static const size_t ValidCharsCount = sizeof(ValidChars) - 1;
    static const int MaxWordLength = 25;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    static std::uniform_int_distribution<> wordLenDistrib(1, MaxWordLength);
    static std::uniform_int_distribution<> charDistrib(0, ValidCharsCount - 1);

    size_t size = 0;
    auto wordLen = wordLenDistrib(gen);
    while (wordLen--)
    {
        auto ch = ValidChars[charDistrib(gen)];
        ss << ch;
        ++size;
    }

    ss << ' ';
    ++size;

    return size;
}

static std::pair<std::string, size_t> generateLargeString(size_t size)
{
    std::ostringstream ss;
    size_t done = 0;
    size_t words = 0;
    while (done < size)
    {
        done += generateWord(ss);
        ++words;
    }

    return std::pair<std::string, size_t>(ss.str(), words);
}

template <class StringT, class ContainerT>
void split2(const StringT& source, ContainerT& receiver)
{
    size_t start = 0;

    while (start < source.size())
    {
        const auto delim = source.find(' ', start);
        const auto end = (delim == StringT::npos) ? source.size() : delim;

        if (start != end)
        {
            receiver.push_back(source.substr(start, end - start));
        }
        
        if (delim == StringT::npos)
            break;

        start = delim + 1;
    }
}

static void std_string_bench(const std::string& source, size_t words)
{
    std::cout << "std::string: split " << words << " words\n";

    std::vector<std::string> v;
    v.reserve(words);

    auto start = std::chrono::high_resolution_clock::now();
    split2(source, v);
    auto end = std::chrono::high_resolution_clock::now();

    auto dura = end - start;
    auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();

    std::cout << msecs << " ms\n";
}

static void immutable_string_bench(const immutable_string& source, size_t words)
{
    std::cout << "immutable_string: split " << words << " words\n";

    std::vector<immutable_string> v;
    v.reserve(words);


    auto start = std::chrono::high_resolution_clock::now();
    split2(source, v);
    auto end = std::chrono::high_resolution_clock::now();

    auto dura = end - start;
    auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();

    std::cout << msecs << " ms\n";
}

void run_benchmark(size_t size)
{
    try
    {
        std::cout << "Generating test data...\n";
        auto source = generateLargeString(size);
        immutable_string source_immutable(source.first);
             
        std_string_bench(source.first, source.second);
        immutable_string_bench(source_immutable, source.second);
    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
    }
}