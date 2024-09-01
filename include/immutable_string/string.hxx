#pragma once

#include "shared_data.hxx"

#include <exception>
#include <limits>
#include <string>


template <class CharT, class TraitsT = std::char_traits<CharT>, class AllocatorT = std::allocator<CharT>>
    requires (!std::is_array_v<CharT>) && std::is_trivial_v<CharT> && std::is_standard_layout_v<CharT>
class basic_immutable_string final
{
private:
    static_assert(std::is_same_v<CharT, typename TraitsT::char_type>);

    template <class AllocatorT, class U>
    using _rebind_alloc = typename std::allocator_traits<AllocatorT>::template rebind_alloc<U>;

    using _allocator = _rebind_alloc<AllocatorT, CharT>;
    using _allocator_traits = std::allocator_traits<_allocator>;

    using _storage = shared_data<CharT, AllocatorT>;
    using _storage_ptr = typename _storage::Ptr;

public:
    using traits_type = TraitsT;
    using allocator_type = AllocatorT;

    using value_type = CharT;
    using size_type = typename _allocator_traits::size_type;
    using difference_type = typename _allocator_traits::difference_type;
    using pointer = typename _allocator_traits::pointer;
    using const_pointer = typename _allocator_traits::const_pointer;
    using reference = value_type&;
    using const_reference = const value_type&;

    static constexpr size_type npos = size_type(-1);

    ~basic_immutable_string() = default;

    constexpr basic_immutable_string() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
        : m_size(IsNullTerminated)
        , m_str(_empty_str())
        , m_storage()
        , m_allocator()
    {
    }

    [[nodiscard]] static basic_immutable_string from_string_literal(const_pointer source, const allocator_type& a = allocator_type()) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
    {
        assert(source);
        size_type size = traits_type::length(source);
        assert(size <= max_size());

        return basic_immutable_string(nullptr, source, _check_size(size) | IsNullTerminated, a);
    }

    basic_immutable_string(const_pointer source, size_type size = size_type(-1), const allocator_type& a = allocator_type())
        : m_allocator(a)
    {
        if (source && *source) [[likely]]
        {
            if (size == size_type(-1))
                size = traits_type::length(source);

            auto source_len = _check_size(size);
            
            auto storage = _storage::create((source_len + 1) * sizeof(value_type), source, source_len, a);
            if (!storage) [[unlikely]]
                throw std::bad_alloc();
            
            m_storage.swap(storage); 
            
            auto data = m_storage->data();
            // _storage is always '\0'-terminated
            data[source_len] = value_type{};
            m_str = data;
            m_size = source_len | IsNullTerminated;
        }
        else [[unlikely]]
        {
            m_size = IsNullTerminated;
            m_str = _empty_str();
        }
    }

    [[nodiscard]] static basic_immutable_string attach(_storage* sb) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
    {
        // no add_ref() called
        assert(sb);
        return basic_immutable_string(sb, sb->data(), sb->size(), sb->get_allocator());
    }

    [[nodiscard]] _storage* detach() noexcept
    {
        // no release() called
        m_size = IsNullTerminated;
        m_str = _empty_str();
        auto p = m_storage.release();
        m_storage.reset();

        return p;
    }

    [[nodiscard]] constexpr bool is_shared() const noexcept
    {
        return !!m_storage;
    }

    basic_immutable_string(const basic_immutable_string& other) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
        : m_size(other.m_size)
        , m_str(other.m_str)
        , m_storage(other.m_storage ? other.m_storage->add_ref() : nullptr)
        , m_allocator(other.m_allocator)
    {
        assert((m_size & SizeMask) == 0 || !!m_str);
    }

    basic_immutable_string& operator=(const basic_immutable_string& other) noexcept
    {
        if (&other != this) [[likely]]
        {
            basic_immutable_string tmp(other);
            swap(tmp);
        }

        return *this;
    }

    basic_immutable_string(basic_immutable_string&& other) noexcept
        : basic_immutable_string()
    {
        swap(other);
    }

    basic_immutable_string& operator=(basic_immutable_string&& other) noexcept
    {
        if (&other != this) [[likely]]
        {
            basic_immutable_string tmp(std::move(other));
            swap(tmp);
        }

        return *this;
    }

    void swap(basic_immutable_string& other) noexcept(std::is_nothrow_swappable_v<allocator_type>)
    {
        using std::swap;
        m_storage.swap(other.m_storage);
        swap(m_str, other.m_str);
        swap(m_size, other.m_size);
        swap(m_allocator, other.m_allocator);
    }

    friend void swap(basic_immutable_string& a, basic_immutable_string& b) noexcept(std::is_nothrow_swappable_v<allocator_type>)
    {
        a.swap(b);
    }

    [[nodiscard]] constexpr const_pointer c_str() const noexcept
    {
        return m_str;
    }

    [[nodiscard]] constexpr size_type length() const noexcept
    {
        return m_size & SizeMask;
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return m_str;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return m_size & SizeMask;
    }

    [[nodiscard]] static constexpr size_type max_size() noexcept
    {
        return SizeMask / sizeof(value_type) - 1; // leave space for '\0'
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    [[nodiscard]] constexpr bool has_null_terminator() const noexcept
    {
        return (m_size & IsNullTerminated) != 0;
    }

    void clear() noexcept
    {
        m_storage.reset();
        m_str = _empty_str();
        m_size = IsNullTerminated;
    }

    [[nodiscard]] constexpr basic_immutable_string substr(size_type start, size_type length = npos) const
    {
        auto const sz = size();
        if (start > sz) [[unlikely]]
            throw std::out_of_range("Start position for basic_immutable_string::substr() exceeds string length");

        if (length == npos) [[likely]]
            length = sz - start;
        else if (length + start > sz) [[unlikely]]
            length = sz - start;

        if (!length)
            return basic_immutable_string();

        return basic_immutable_string(m_storage ? m_storage->add_ref().get() : nullptr, m_str + start, length, m_allocator);
    }

private:
    static size_type const IsNullTerminated = size_type(1) << (std::numeric_limits<size_type>::digits - 1);
    static size_type const SizeMask = IsNullTerminated - 1;

    constexpr basic_immutable_string(_storage* stg, const_pointer data, size_type size, const allocator_type& a) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
        : m_size(size)
        , m_str(data)
        , m_storage(stg) // no add_ref()
        , m_allocator(a)
    {
    }

    static size_type _check_size(size_type size)
    {
        if (size > max_size()) [[unlikely]]
            throw std::length_error("Cannot create immutable string this long");

        return size;
    }

    static constexpr const_pointer _empty_str() noexcept
    {
        static value_type _e = { value_type{} };
        return &_e;
    }

    size_type m_size;
    const_pointer m_str;
    _storage_ptr m_storage;
    allocator_type m_allocator;
};


using immutable_string = basic_immutable_string<char, std::char_traits<char>, std::allocator<char>>;
using immutable_wstring = basic_immutable_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;