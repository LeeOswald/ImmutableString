#pragma once


#include <atomic>
#include <cassert>
#include <memory>
#include <type_traits>

//
// shared buffer if PODs
// no ctors, no dtors, only memcpy
//

template <typename T, typename AllocatorT = std::allocator<T>>
    requires std::is_pod_v<T>
class shared_data final
{
public:
    using allocator_type = AllocatorT;
    using allocator_traits = std::allocator_traits<allocator_type>;

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    shared_data(const shared_data&) = delete;
    shared_data& operator=(const shared_data&) = delete;
    shared_data(shared_data&&) = delete;
    shared_data& operator=(shared_data&&) = delete;

    constexpr T const* data() const noexcept
    {
        static constexpr size_type _header_size = round_up(sizeof(shared_data));

        auto start = reinterpret_cast<const std::byte*>(this + 1);
        return reinterpret_cast<const T*>(start + _header_size);
    }

    constexpr T* data() noexcept
    {
        static constexpr size_type _header_size = round_up(sizeof(shared_data));

        auto start = reinterpret_cast<std::byte*>(this + 1);
        return reinterpret_cast<T*>(start + _header_size);
    }

    constexpr size_type capacity() const noexcept
    {
        return m_capacity;
    }

    constexpr size_type size() const noexcept
    {
        return m_size;
    }

    static shared_data* create(size_type capacity, const value_type* source, size_type size, const allocator_type& allocator = allocator_type())
    {        
        static constexpr size_type _header_size = round_up(sizeof(shared_data));

        _raw_allocator a = allocator;
        size_type allocation_size = sizeof(value_type) * capacity + _header_size;
        auto raw = a.allocate(allocation_size);
        if (!raw)
            return nullptr; // allocator decides whether to throw or not

        auto header = new (static_cast<void*>(raw)) shared_data(std::move(a), capacity, source, size);
        return header;
    }

    enum class ReleaseMode
    {
        Destroy,
        Recycle
    };

    enum class ReleaseResult
    {
        MoreReferenses,
        MayRecycle,
        Destroyed
    };

    static ReleaseResult release(shared_data* s, ReleaseMode mode)
    {
        if (s)
        {
            auto prev_refs = s->m_refs.fetch_sub(1, std::memory_order_acq_rel);
            if (prev_refs == 1)
            {
                // that was the last reference
                s->m_size = 0;
                if (mode == ReleaseMode::Destroy)
                {
                    static constexpr size_type _header_size = round_up(sizeof(shared_data));
                    size_type allocation_size = sizeof(value_type) * s->m_capacity + _header_size;

                    // yep, no dtors!
                    s->m_allocator.deallocate(reinterpret_cast<std::byte*>(s), allocation_size);

                    return ReleaseResult::Destroyed;
                }

                // we want to keep this one for later usage to avoid extra allocation/deallocation
                // just set reference count to 1 since we know now we have an exclusive ownership
                s->m_refs++;
                return ReleaseResult::MayRecycle;
            }

            return ReleaseResult::MoreReferenses;
        }

        return ReleaseResult::Destroyed;
    }

private:
    template <class AllocatorT, class U>
    using _rebind_alloc = typename std::allocator_traits<AllocatorT>::template rebind_alloc<U>;

    using _raw_allocator = _rebind_alloc<allocator_type, std::byte>;

    template <typename U>
    static constexpr U round_up(U x) noexcept
    {
        U const g = alignof(value_type);
        return (x + g - 1) & (~(g - 1));
    }
    
    ~shared_data() = default;

    constexpr shared_data(_raw_allocator&& allocator, size_type capacity, const value_type* source, size_type size) noexcept
        : m_allocator(std::move(allocator))
        , m_capacity(capacity)
        , m_size(size)
        , m_refs(1)
    {
        assert(m_size <= m_capacity);

        if (size)
        {
            assert(!!source);
            std::memcpy(data(), source, size * sizeof(value_type)); // yep, no copy ctors!
        }
    }

    _raw_allocator m_allocator;
    std::atomic<size_type> m_refs;
    size_type m_capacity;
    size_type m_size;
};