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

    struct _ptrs
    {
        typename _allocator_traits::const_pointer str;
        _storage* storage;
    };

    static constexpr typename _allocator_traits::size_type ShortStringSize = sizeof(_ptrs) / sizeof(CharT);

    union _sso_storage
    {
        _ptrs ptrs;
        CharT short_string[ShortStringSize];
    };

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

    ~basic_immutable_string()
    {
        _release();
    }

    constexpr basic_immutable_string() noexcept
        : m_size(IsNullTerminated)
    {
        m_storage.ptrs.str = _empty_str();
        m_storage.ptrs.storage = nullptr;
    }

    [[nodiscard]] static basic_immutable_string from_string_literal(const_pointer source) noexcept
    {
        assert(source);
        size_type size = traits_type::length(source);
        assert(size <= max_size());

        return basic_immutable_string(nullptr, source, _check_size(size) | IsNullTerminated);
    }

    basic_immutable_string(const_pointer source, size_type size = size_type(-1), const allocator_type& a = allocator_type())
    {
        if (source && *source) [[likely]]
        {
            if (size == size_type(-1))
                size = traits_type::length(source);

            auto source_len = _check_size(size);

            if (source_len < ShortStringSize)
            {
                // short string optimization
                std::memcpy(m_storage.short_string, source, source_len);
                m_storage.short_string[source_len] = value_type{}; // \0
                m_size = source_len | IsShortString | IsNullTerminated;
            }
            else
            {
                auto storage = _storage::create((source_len + 1) * sizeof(value_type), source, source_len, a);
                if (!storage) [[unlikely]]
                    throw std::bad_alloc();

                m_storage.ptrs.storage = storage.release();

                auto data = m_storage.ptrs.storage->data();
                data[source_len] = value_type{}; // \0
                m_storage.ptrs.str = data;
                m_size = source_len | IsNullTerminated;
            }
        }
        else [[unlikely]]
        {
            m_storage.ptrs.storage = nullptr;
            m_storage.ptrs.str = _empty_str();
            m_size = IsNullTerminated;
        }
    }

    [[nodiscard]] static basic_immutable_string attach(_storage* sb) noexcept
    {
        // no add_ref() called
        assert(sb);
        return basic_immutable_string(sb, sb->data(), sb->size());
    }

    [[nodiscard]] _storage* detach() noexcept
    {
        // no release() called
        m_size = IsNullTerminated;
        if (is_short())
            return nullptr;

        m_storage.ptrs.str = _empty_str();
        auto p = m_storage.ptrs.storage;
        m_storage.ptrs.storage = nullptr;
        return p;
    }

    [[nodiscard]] constexpr bool is_shared() const noexcept
    {
        return !is_short() && !!m_storage.ptrs.storage;
    }

    [[nodiscard]] constexpr bool is_short() const noexcept
    {
        return (m_size & IsShortString) != 0;
    }

    basic_immutable_string(const basic_immutable_string& other) noexcept
        : m_size(other.m_size)
    {
        // this will copy m_storage.short_string either
        m_storage.ptrs.storage = other.m_storage.ptrs.storage;
        m_storage.ptrs.str = other.m_storage.ptrs.str;
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

    void swap(basic_immutable_string& other) noexcept
    {
        using std::swap;
        // this will swap m_storage.short_string either
        swap(m_storage.ptrs.str, other.m_storage.ptrs.str);
        swap(m_storage.ptrs.storage, other.m_storage.ptrs.storage);
        swap(m_size, other.m_size);
    }

    friend void swap(basic_immutable_string& a, basic_immutable_string& b) noexcept
    {
        a.swap(b);
    }

    [[nodiscard]] constexpr const_pointer c_str() const noexcept
    {
        if (m_size & IsNullTerminated)
            return _get_string();

        assert(!empty());
        auto cstr = _make_cstr();
        auto data = cstr->data();
        auto size = cstr->size() | IsNullTerminated;
        _subvert_data(cstr, data, size);

        return _get_string();
    }

    [[nodiscard]] constexpr size_type length() const noexcept
    {
        return m_size & SizeMask;
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return _get_string();
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
        m_storage.ptrs.storage = nullptr;
        m_storage.ptrs.str = _empty_str();
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
        else if (length < ShortStringSize)
            return basic_immutable_string(_get_string() + start, length); // SSO

        return basic_immutable_string(_get_storage_add_ref(), _get_string() + start, length);
    }

private:
    static size_type const IsNullTerminated = size_type(1) << (std::numeric_limits<size_type>::digits - 1);
    static size_type const IsShortString = IsNullTerminated >> 1;
    static size_type const SizeMask = IsShortString - 1;

    constexpr basic_immutable_string(_storage* stg, const_pointer data, size_type size) noexcept
        : m_size(size)
    {
        assert(!is_short());
        m_storage.ptrs.str = data;
        m_storage.ptrs.storage = stg; // no add_ref()
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

    allocator_type _get_allocator() const noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
    {
        auto stg = _get_storage();
        if (stg)
            return stg->get_allocator();

        return allocator_type();
    }

    _storage_ptr _make_cstr() const
    {
        auto len = size();
        auto storage = _storage::create((len + 1) * sizeof(value_type), data(), len, _get_allocator());
        if (!storage) [[unlikely]]
            throw std::bad_alloc();
                
        // _storage is always '\0'-terminated
        auto data = storage->data();
        data[len] = value_type{};

        return storage;
    }

    void _subvert_data(_storage_ptr& stg, const_pointer data, size_type size) const noexcept
    {
        // the purpose of this is to replace self with a null-terminated string in .c_str() which is const
        _release();

        m_size = size;
        m_storage.ptrs.storage = stg.release();
        m_storage.ptrs.str = data;
    }

    void _release() const noexcept
    {
        if (!is_short() && m_storage.ptrs.storage)
            m_storage.ptrs.storage->release();
    }

    constexpr _storage* _get_storage() const noexcept
    {
        if (!is_short())
            return m_storage.ptrs.storage;

        return nullptr;
    }

    constexpr _storage* _get_storage_add_ref() const noexcept
    {
        if (!is_short() && m_storage.ptrs.storage)
            return m_storage.ptrs.storage->add_ref().release();

        return nullptr;
    }

    constexpr const_pointer _get_string() const noexcept
    {
        if (!is_short())
            return m_storage.ptrs.str;

        return m_storage.short_string;
    }

    // omg, yes, they're all mutable
    // and all of this is because of .c_str() constness 
    mutable size_type m_size;
    mutable _sso_storage m_storage;
};


using immutable_string = basic_immutable_string<char, std::char_traits<char>, std::allocator<char>>;
using immutable_wstring = basic_immutable_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;