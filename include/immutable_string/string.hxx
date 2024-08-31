#pragma once

#include "shared_data.hxx"

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

    using _string_buffer = shared_data<CharT, AllocatorT>;
    using _string_view = std::basic_string_view<CharT, TraitsT>;

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

    struct NotOwningT {};
    static constexpr NotOwningT NotOwning;

    struct CloneT {};
    static constexpr CloneT Clone;

    ~basic_immutable_string()
    {
        _release();
    }

    constexpr basic_immutable_string() noexcept
        : m_buf(nullptr)
        , m_str(_empty_str())
        , m_size(0)
    {
    }

    constexpr basic_immutable_string(nullptr_t) noexcept
        : basic_immutable_string()
    {
    }

    constexpr basic_immutable_string(_string_view source, NotOwningT) noexcept
        : m_buf(nullptr)
        , m_str(source.data() ? source.data() : _empty_str())
        , m_size(source.size())
    {
    }

    constexpr basic_immutable_string(const char* source, NotOwningT) noexcept
        : basic_immutable_string(_string_view(source, source ? traits_type::length(source) : 0), NotOwning)
    {
    }

    basic_immutable_string(_string_view source, CloneT, const allocator_type& a = allocator_type())
    {
        if (!source.empty()) [[likely]]
        {
            auto source_len = source.size();
            
            m_buf = _string_buffer::create((source_len + 1) * sizeof(value_type), source.data(), source_len, a);
            if (m_buf) [[likely]]
            {
                auto data = m_buf->data();
                data[source_len] = value_type{};
                m_str = data;
                m_size = source_len;

                return;
            }
        }

        m_buf = nullptr;
        m_str = _empty_str();
        m_size = 0;
    }

    basic_immutable_string(const char* source, CloneT, const allocator_type& a = allocator_type())
        : basic_immutable_string(_string_view(source, source ? traits_type::length(source) : 0), Clone, a)
    {
    }

    basic_immutable_string(const basic_immutable_string& other) noexcept
        : m_buf(other.m_buf ? const_cast<_string_buffer*>(_string_buffer::add_ref(other.m_buf)) : nullptr)
    {
        if (m_buf) [[likely]]
        {
            auto data = m_buf->data();
            m_str = data;
            m_size = other.m_size;
        }
        else if (other.m_str)
        {
            m_str = other.m_str;
            m_size = other.m_size;
        }
        else
        {
            m_str = _empty_str();
            m_size = 0;
        }
    }

    basic_immutable_string& operator=(const basic_immutable_string& other) noexcept
    {
        if (&other != this) [[likely]]
        {
            basic_immutable_string tmp(other);
            swap(*this, tmp);
        }

        return *this;
    }

    basic_immutable_string(basic_immutable_string&& other) noexcept
        : basic_immutable_string()
    {
        swap(*this, other);
    }

    basic_immutable_string& operator=(basic_immutable_string&& other) noexcept
    {
        if (&other != this) [[likely]]
        {
            basic_immutable_string tmp(std::move(other));
            swap(*this, tmp);
        }

        return *this;
    }

    friend void swap(basic_immutable_string& a, basic_immutable_string& b) noexcept
    {
        using std::swap;
        swap(a.m_buf, b.m_buf);
        swap(a.m_str, b.m_str);
        swap(a.m_size, b.m_size);
    }

    [[nodiscard]] constexpr const_pointer c_str() const noexcept
    {
        return m_str;
    }

    [[nodiscard]] constexpr size_type length() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return m_str;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }

    void clear() noexcept
    {
        _release();
        m_buf = nullptr;
        m_str = _empty_str();
        m_size = 0;
    }

private:
    static constexpr const_pointer _empty_str() noexcept
    {
        static value_type _e = { value_type{} };
        return &_e;
    }

    void _release() noexcept
    {
        if (m_buf) [[likely]]
        {
            _string_buffer::release(m_buf, _string_buffer::ReleaseMode::Destroy);
        }
    }

    _string_buffer* m_buf;
    const_pointer m_str;    // cached string pointer
    size_type m_size;       // cached string length
};


using immutable_string = basic_immutable_string<char, std::char_traits<char>, std::allocator<char>>;
using immutable_wstring = basic_immutable_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;