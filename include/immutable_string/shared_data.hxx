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
    requires (!std::is_array_v<T>) && std::is_trivial_v<T> && std::is_standard_layout_v<T>
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

    [[nodiscard]] constexpr T const* data() const noexcept
    {
        auto start = reinterpret_cast<const std::byte*>(this);
        return reinterpret_cast<const T*>(start + padded_header_size());
    }

    [[nodiscard]] constexpr T* data() noexcept
    {
        auto start = reinterpret_cast<std::byte*>(this);
        return reinterpret_cast<T*>(start + padded_header_size());
    }

    [[nodiscard]] constexpr size_type capacity() const noexcept
    {
        return m_capacity;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] static shared_data* create(size_type capacity, const value_type* source, size_type size, const allocator_type& allocator = allocator_type())
    {        
        _raw_allocator a = allocator;
        size_type allocation_size = sizeof(value_type) * capacity + padded_header_size();
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

    [[nodiscard]] static const shared_data* add_ref(const shared_data* s) noexcept
    {
        assert(s);
        s->m_refs++;
        return s;
    }

    static ReleaseResult release(shared_data* s, ReleaseMode mode)
    {
        assert(s);
        
        auto prev_refs = s->m_refs.fetch_sub(1, std::memory_order_acq_rel);
        if (prev_refs == 1)
        {
            // that was the last reference
            s->m_size = 0;
            if (mode == ReleaseMode::Destroy)
            {
                size_type allocation_size = sizeof(value_type) * s->m_capacity + padded_header_size();

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

    static constexpr size_type padded_header_size() noexcept
    {
        static constexpr size_type _header_size = round_up(sizeof(shared_data));
        return _header_size;
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
    mutable std::atomic<size_type> m_refs;
    size_type m_capacity;
    size_type m_size;
};