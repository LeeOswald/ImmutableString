#include "common.h"

#include <immutable_string/string.hxx>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

using namespace ims;

const char SEPARATOR = '\n';
const char* const SSEPARATOR = "\n";

class allocator_base
{
public:
    void* allocate(size_t size)
    {
        auto p = std::malloc(size);
        if (!p)
            throw std::bad_alloc();

        ++_allocations;
        _allocated_bytes += size;

        if (_verbose)
            std::cout << "a " << size << "\n";

        return p;
    }

    void deallocate(void* p, size_t size)
    {
        std::free(p);

        if (_verbose)
            std::cout << "r " << size << "\n";
    }

    static uint64_t _allocations;
    static uint64_t _allocated_bytes;
    static bool _verbose;
};

uint64_t allocator_base::_allocations = 0;
uint64_t allocator_base::_allocated_bytes = 0;
bool allocator_base::_verbose = false;

template <class _Ty>
class BenchAllocator
    : public allocator_base
{
public:
    using value_type = _Ty;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    constexpr BenchAllocator() noexcept {}

    constexpr BenchAllocator(const BenchAllocator&) noexcept = default;
    
    template <class _Other>
    constexpr BenchAllocator(const BenchAllocator<_Other>&) noexcept {}
    
    constexpr ~BenchAllocator() = default;
    constexpr BenchAllocator& operator=(const BenchAllocator&) = default;

    constexpr void deallocate(_Ty* const _Ptr, const size_t _Count) 
    {
        allocator_base::deallocate(_Ptr, sizeof(_Ty) * _Count);
    }

    constexpr _Ty* allocate(const size_t _Count) 
    {
        return static_cast<_Ty*>(allocator_base::allocate(sizeof(_Ty) * _Count));
    }

    friend constexpr bool operator==(const BenchAllocator& a, const BenchAllocator& b) noexcept
    {
        return true;
    }
};

using StdString = std::basic_string<char, std::char_traits<char>, BenchAllocator<char>>;
using RString = basic_immutable_string<char, std::char_traits<char>, BenchAllocator<char>>;
using OStringStream = std::basic_ostringstream<char, std::char_traits<char>, BenchAllocator<char>>;

static void generateWord(OStringStream& ss)
{
    static const char ValidChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static const size_t ValidCharsCount = sizeof(ValidChars) - 1;
    static const int MaxWordLength = 64;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    static std::uniform_int_distribution<> wordLenDistrib(1, MaxWordLength);
    static std::uniform_int_distribution<> charDistrib(0, ValidCharsCount - 1);

    auto wordLen = wordLenDistrib(gen);
    while (wordLen--)
    {
        auto ch = ValidChars[charDistrib(gen)];
        ss << ch;
    }
}

static StdString generate_data_set(unsigned long long words)
{
    OStringStream ss;
    
    while (words)
    {
        generateWord(ss);
        --words;
        if (words > 0)
            ss << SEPARATOR;
    }

    return ss.str();
}

template <class StringT, class ContainerT>
void split2(const StringT& source, ContainerT& receiver)
{
    size_t start = 0;

    while (start < source.size())
    {
        const auto delim = source.find(SEPARATOR, start);
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

static std::string format_memsize(uint64_t bytes)
{
    std::ostringstream stream;
    if (bytes < 1024 * 10)
    {
        stream << bytes << " bytes";
    }
    else
    {
        stream << std::fixed << std::setprecision(3);
        double v = bytes / 1024.0;
        if (v < 1024 * 10)
        {
            stream << v << " Kb";
        }
        else
        {
            v /= 1024.0;
            if (v < 1024 * 10)
            {
                stream << v << " Mb";
            }
            else
            {
                v /= 1024.0;
                stream << v << " Gb";
            }
        }
    }

    return stream.str();
}

template <typename StringT, typename SplitT, typename MergeT>
void run_benchmark_split_merge(const StringT& source, uint64_t wc, SplitT splitter, MergeT merger, unsigned runs, bool silent)
{
    uint64_t timeSplit = 0;
    uint64_t memSplit = 0;
    uint64_t allocsSplit = 0;

    uint64_t timeMerge = 0;
    uint64_t memMerge = 0;
    uint64_t allocsMerge = 0;

    std::vector<StringT, BenchAllocator<StringT>> words;
    words.reserve(wc);

    for (unsigned r = 0; r < runs; r++)
    {
        words.clear();
        //allocator_base::_verbose = true;

        // split
        {
            auto mem0 = allocator_base::_allocated_bytes;
            auto allocs0 = allocator_base::_allocations;

            auto start = std::chrono::high_resolution_clock::now();
            splitter(words, source, silent);
            auto end = std::chrono::high_resolution_clock::now();

            auto mem1 = allocator_base::_allocated_bytes;
            memSplit += mem1 - mem0;
            auto allocs1 = allocator_base::_allocations;
            allocsSplit += allocs1 - allocs0;

            auto dura = end - start;
            auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();
            timeSplit += msecs;
        }
#if 1
        // merge
        {
            auto mem0 = allocator_base::_allocated_bytes;
            auto allocs0 = allocator_base::_allocations;

            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < 10; i++) 
            {
                merger(words, source, silent);
            }

            auto end = std::chrono::high_resolution_clock::now();

            auto mem1 = allocator_base::_allocated_bytes;
            memMerge += mem1 - mem0;
            auto allocs1 = allocator_base::_allocations;
            allocsMerge += allocs1 - allocs0;

            auto dura = end - start;
            auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(dura).count();
            timeMerge += msecs;
        }
#endif
        allocator_base::_verbose = false;
    }

    if (!silent)
    {
        timeSplit /= runs;
        timeMerge /= runs;
        memSplit /= runs;
        memMerge /= runs;
        allocsSplit /= runs;
        allocsMerge /= runs;

        std::cout << "Time (ms):  " << std::setw(10) << timeSplit + timeMerge << "     Split: " << std::setw(10) << timeSplit << " Merge: " << std::setw(10) << timeMerge << "\n";
        std::cout << "Allocations:" << std::setw(10) << allocsSplit + allocsMerge << "     Split: " << std::setw(10) << allocsSplit << " Merge: " << std::setw(10) << allocsMerge << "\n";
        std::cout << "Memory:     " << format_memsize(memSplit + memMerge) << "     Split: " << format_memsize(memSplit) << " Merge: " << format_memsize(memMerge) << "\n";
        std::cout << "--------------------------------------------------------------\n";
    }

}

static void std_string_splitter(std::vector<StdString, BenchAllocator<StdString>>& v, const StdString& source, bool silent)
{
    if (!silent)
        std::cout << "Splitting std::string...\n";
    
    split2(source, v);
}

static void std_string_merger(const std::vector<StdString, BenchAllocator<StdString>>& v, const StdString& source, bool silent)
{
    if (!silent)
        std::cout << "Merging std::string...\n";

    StdString m;
    std::size_t i = 0;
    std::size_t count =  v.size();
    for (auto& s : v)
    {
        m.append(s);
        if (++i < count)
        {
            m.append(1, SEPARATOR);
        }
    }

    if (m != source)
        std::cout << "ERROR while splitting/merging std::string\n";
}

static void std_string_stream_merger(const std::vector<StdString, BenchAllocator<StdString>>& v, const StdString& source, bool silent)
{
    if (!silent)
        std::cout << "Merging std::string with std::ostringstream...\n";

    OStringStream ss;
    std::size_t i = 0;
    std::size_t count = v.size();
    for (auto& s : v)
    {
        ss << s;
        if (++i < count)
        {
            ss << SEPARATOR;
        }
    }

    auto r = ss.str();
    if (r != source)
        std::cout << "ERROR while splitting/merging std::string\n";
}

static void immutable_string_splitter(std::vector<RString, BenchAllocator<RString>>& v, const RString& source, bool silent)
{
    if (!silent)
        std::cout << "Splitting immutable_string...\n";
    
    split2(source, v);
}

static void immutable_string_merger(const std::vector<RString, BenchAllocator<RString>>& v, const RString& source, bool silent)
{
    if (!silent)
        std::cout << "Merging immutable_string...\n";

    std::size_t count = v.size();
    RString::builder b(count);
    std::size_t i = 0;
    
    for (auto& s : v)
    {
        b.append(s);

        if (++i < count)
        {
            b.append(RString(SSEPARATOR, 1, RString::FromStringLiteral));
        }
    }

    auto result = b.str();
    if (result != source)
        std::cout << "ERROR while splitting/merging immutable_string\n";
}

int generate_benchmark(const std::string& file, unsigned long long words)
{
    try
    {
        std::cout << "Generating test data...\n";
        auto source = generate_data_set(words);
        std::cout << "Generated " << source.size() << " bytes dataset\n";

        std::ofstream f(file, std::ios_base::binary | std::ios_base::trunc);
        if (!f.is_open())
            throw std::runtime_error(std::string("Failed to create ") + file);

        f.write(source.data(), source.size());
    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return -1;
    }

    return 0;
}

template <typename StringT>
uint64_t count_words(const StringT& s)
{
    uint64_t count = 0;
    for (auto c : s)
    {
        if (c == SEPARATOR)
            ++count;
    }

    return count;
}

int run_benchmark(const std::string& file, unsigned long long words, unsigned runs, bool silent)
{
    try
    {
        StdString data_set;

        if (!file.empty())
        {
            std::cout << "Loading test data...\n";
            std::ifstream f(file, std::ios_base::binary);
            if (!f.is_open())
                throw std::runtime_error(std::string("Failed to open ") + file);

            OStringStream ss;
            ss << f.rdbuf();
            data_set = ss.str();

            std::cout << "Loaded " << data_set.size() << " bytes dataset\n";
        }
        else
        {
            std::cout << "Generating test data...\n";
            data_set = generate_data_set(words);
            std::cout << "Generated " << data_set.size() << " bytes\n";
        }

        auto words = count_words(data_set) + 1;
        std::cout << "Data size is " << words << " words\n";

        RString source_immutable(data_set);
        
        run_benchmark_split_merge(data_set, words, std_string_splitter, std_string_merger, runs, silent);
        run_benchmark_split_merge(data_set, words, std_string_splitter, std_string_stream_merger, runs, silent);
        run_benchmark_split_merge(source_immutable, words, immutable_string_splitter, immutable_string_merger, runs, silent);

    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return -1;
    }

    return 0;
}