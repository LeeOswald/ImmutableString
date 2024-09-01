#pragma once


#include <atomic>
#include <cassert>
#include <memory>
#include <type_traits>


//
// shared raw bytes buffer
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

    struct Deleter
    {
        void operator()(shared_data* p) noexcept
        {
            if (p) [[likely]]
                p->release();
        }
    };

    using Ptr = std::unique_ptr<shared_data, Deleter>;

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

    [[nodiscard]] constexpr const allocator_type get_allocator() const noexcept
    {
        return m_allocator;
    }

    [[nodiscard]] static Ptr create(size_type capacity, const value_type* source, size_type size, const allocator_type& allocator = allocator_type())
    {        
        _raw_allocator a = allocator;
        size_type allocation_size = sizeof(value_type) * capacity + padded_header_size();
        auto raw = a.allocate(allocation_size);
        if (!raw) [[unlikely]]
            return Ptr(); // allocator decides whether to throw or not

        // buffer contents are left uninitialized, no ctors called 
        return Ptr(new (static_cast<void*>(raw)) shared_data(std::move(a), capacity, source, size));
    }

    [[nodiscard]] Ptr add_ref() const noexcept
    {   
        m_refs++;
        return Ptr(const_cast<shared_data*>(this));
    }

    size_type release() noexcept
    {
        auto prev_refs = m_refs.fetch_sub(1, std::memory_order_acq_rel);
        assert(prev_refs > 0);
        if (prev_refs == 1)
        {
            // that was the last reference
            size_type allocation_size = sizeof(value_type) * m_capacity + padded_header_size();

            // no dtors called
            m_allocator.deallocate(reinterpret_cast<std::byte*>(this), allocation_size);
        }

        return prev_refs - 1;
    }

private:
    template <class AllocatorT, class U>
    using _rebind_alloc = typename std::allocator_traits<AllocatorT>::template rebind_alloc<U>;

    using _raw_allocator = _rebind_alloc<allocator_type, std::byte>;

    static constexpr size_type padded_header_size() noexcept
    {
        static constexpr size_type alignment = alignof(value_type) < alignof(void*) ? alignof(void*) : alignof(value_type);
        static constexpr size_type header_size = (sizeof(shared_data) + alignment - 1) & (~(alignment - 1));
        return header_size;
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
            std::memcpy(data(), source, size * sizeof(value_type)); // no copy ctors called
        }
    }

    _raw_allocator m_allocator;
    mutable std::atomic<size_type> m_refs;
    size_type m_capacity;
    size_type m_size;
};