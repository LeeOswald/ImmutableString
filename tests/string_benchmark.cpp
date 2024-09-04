#include "common.h"

#include <immutable_string/string.hxx>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

class allocator_base
{
public:
    void* allocate(size_t size)
    {
        auto p = std::malloc(size);
        if (!p)
            throw std::bad_alloc();

        ++_allocations;
        _current_bytes += size;
        if (_current_bytes > _peak_bytes)
            _peak_bytes = _current_bytes;

        return p;
    }

    void deallocate(void* p, size_t size)
    {
        std::free(p);
        _current_bytes -= size;
        ++_frees;
    }

    static int64_t _peak_bytes;
    static int64_t _current_bytes;
    static int64_t _allocations;
    static int64_t _frees;
};

int64_t allocator_base::_peak_bytes = 0;
int64_t allocator_base::_current_bytes = 0;
int64_t allocator_base::_allocations = 0;
int64_t allocator_base::_frees = 0;

template <class _Ty>
class Allocator
    : public allocator_base
{
public:
    using value_type = _Ty;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    constexpr Allocator() noexcept {}

    constexpr Allocator(const Allocator&) noexcept = default;
    
    template <class _Other>
    constexpr Allocator(const Allocator<_Other>&) noexcept {}
    
    constexpr ~Allocator() = default;
    constexpr Allocator& operator=(const Allocator&) = default;

    constexpr void deallocate(_Ty* const _Ptr, const size_t _Count) 
    {
        allocator_base::deallocate(_Ptr, sizeof(_Ty) * _Count);
    }

    constexpr _Ty* allocate(const size_t _Count) 
    {
        return static_cast<_Ty*>(allocator_base::allocate(sizeof(_Ty) * _Count));
    }

};

using StdString = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
using RString = basic_immutable_string<char, std::char_traits<char>, Allocator<char>>;


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

static std::pair<StdString, size_t> generateLargeString(size_t size)
{
    std::ostringstream ss;
    size_t done = 0;
    size_t words = 0;
    while (done < size)
    {
        done += generateWord(ss);
        ++words;
    }

    return std::pair<StdString, size_t>(ss.str(), words);
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

static void std_string_bench(const StdString& source, size_t words)
{
    std::cout << "-------------- std::string -----------------------\n";
    std::cout << "Words: " << words << "\n";

    std::vector<StdString> v;
    v.reserve(words);
    unsigned const runs = 3;

    int64_t timeSplit = 0;
    int64_t memSplit = 0;
    int64_t allocsSplit = 0;

    int64_t timeMerge = 0;
    int64_t memMerge = 0;
    int64_t allocsMerge = 0;


    for (unsigned r = 0; r < runs; r++)
    {
        // split
        auto mem0 = allocator_base::_current_bytes;
        auto allocs0 = allocator_base::_allocations;

        auto start = std::chrono::high_resolution_clock::now();
        split2(source, v);
        auto end = std::chrono::high_resolution_clock::now();

        auto mem1 = allocator_base::_current_bytes;
        memSplit += mem1 - mem0;
        auto allocs1 = allocator_base::_allocations;
        allocsSplit += allocs1 - allocs0;

        auto dura = end - start;
        auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();
        timeSplit += msecs;

        // merge
        mem0 = allocator_base::_current_bytes;
        allocs0 = allocator_base::_allocations;

        start = std::chrono::high_resolution_clock::now();
        
        StdString result;
        for (auto& s : v)
        {
            result.append(std::move(s));
            result.append(" ");
        }
                
        if (result != source)
            std::cerr << "Split/merge of std::string went wrong\n";


        end = std::chrono::high_resolution_clock::now();

        mem1 = allocator_base::_current_bytes;
        memMerge += mem1 - mem0;
        allocs1 = allocator_base::_allocations;
        allocsMerge += allocs1 - allocs0;

        dura = end - start;
        msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();
        timeMerge += msecs;

        v.clear();
    }

    timeSplit /= runs;
    memSplit /= runs;
    allocsSplit /= runs;

    std::cout << "Time: s=" << timeSplit << " m=" << timeMerge << " t=" << timeSplit + timeMerge << " ms\n";
    std::cout << "Memory: s=" << memSplit << " m=" << memMerge << " t=" << memSplit + memMerge << " bytes\n";
    std::cout << "Allocations: s=" << allocsSplit << " m=" << allocsMerge << " t=" << allocsSplit + allocsMerge << "\n";
}

static void immutable_string_bench(const RString& source, size_t words)
{
    std::cout << "-------------- immutable_string -----------------------\n";
    std::cout << "Words: " << words << "\n";

    std::vector<RString> v;
    v.reserve(words);

    unsigned const runs = 3;

    int64_t timeSplit = 0;
    int64_t memSplit = 0;
    int64_t allocsSplit = 0;

    int64_t timeMerge = 0;
    int64_t memMerge = 0;
    int64_t allocsMerge = 0;

    for (unsigned r = 0; r < runs; r++)
    {
        // split
        auto mem0 = allocator_base::_current_bytes;
        auto allocs0 = allocator_base::_allocations;

        auto start = std::chrono::high_resolution_clock::now();
        split2(source, v);
        auto end = std::chrono::high_resolution_clock::now();

        auto mem1 = allocator_base::_current_bytes;
        memSplit += mem1 - mem0;
        auto allocs1 = allocator_base::_allocations;
        allocsSplit += allocs1 - allocs0;

        auto dura = end - start;
        auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();
        timeSplit += msecs;

        // merge
        mem0 = allocator_base::_current_bytes;
        allocs0 = allocator_base::_allocations;

        start = std::chrono::high_resolution_clock::now();

        RString::builder bld;
        for (auto& s : v)
        {
            bld.append(std::move(s));
            bld.append(" ");
        }

        auto result = bld.str();
        if (std::memcmp(result.data(), source.data(), std::min(result.size(), source.size())) != 0)
            std::cerr << "Split/merge of RString went wrong\n";


        end = std::chrono::high_resolution_clock::now();

        mem1 = allocator_base::_current_bytes;
        memMerge += mem1 - mem0;
        allocs1 = allocator_base::_allocations;
        allocsMerge += allocs1 - allocs0;

        dura = end - start;
        msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();
        timeMerge += msecs;

        v.clear();
    }

    timeSplit /= runs;
    memSplit /= runs;
    allocsSplit /= runs;

    std::cout << "Time: s=" << timeSplit << " m=" << timeMerge << " t=" << timeSplit + timeMerge << " ms\n";
    std::cout << "Memory: s=" << memSplit << " m=" << memMerge << " t=" << memSplit + memMerge << " bytes\n";
    std::cout << "Allocations: s=" << allocsSplit << " m=" << allocsMerge << " t=" << allocsSplit + allocsMerge << "\n";
}

void run_benchmark(size_t size)
{
    try
    {
        std::cout << "Generating test data...\n";
        auto source = generateLargeString(size);

#if 0
        std::ofstream of("data.txt");
        of << source.first;
        of.close();
#endif
        RString source_immutable(source.first);
        
        std_string_bench(source.first, source.second);
        immutable_string_bench(source_immutable, source.second);

    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
    }
}