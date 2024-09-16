#pragma once


#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace ims
{

namespace detail
{

template <class StringT>
struct string_const_iterator
{
    using iterator_concept = std::contiguous_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename StringT::value_type;
    using difference_type = typename StringT::difference_type;
    using pointer = typename StringT::const_pointer;
    using reference = const value_type&;

    constexpr string_const_iterator(pointer ptr = pointer{}) noexcept
        : _ptr(ptr)
    {
    }

    [[nodiscard]] constexpr reference operator*() const noexcept
    {
        assert(_ptr);
        return *_ptr;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept 
    {
        return std::pointer_traits<pointer>::pointer_to(**this);
    }

    constexpr string_const_iterator& operator++() noexcept
    {
        assert(_ptr);
        ++_ptr;
        return *this;
    }

    constexpr string_const_iterator operator++(int) noexcept 
    {
        string_const_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr string_const_iterator& operator--() noexcept
    {
        assert(_ptr);
        --_ptr;
        return *this;
    }

    constexpr string_const_iterator operator--(int) noexcept
    {
        string_const_iterator tmp = *this;
        --*this;
        return tmp;
    }

    constexpr string_const_iterator& operator+=(const difference_type offset) noexcept 
    {
        _ptr += offset;
        return *this;
    }

    [[nodiscard]] constexpr string_const_iterator operator+(const difference_type offset) const noexcept 
    {
        string_const_iterator tmp = *this;
        tmp += offset;
        return tmp;
    }

    [[nodiscard]] friend constexpr string_const_iterator operator+(const difference_type offset, string_const_iterator next) noexcept 
    {
        next += offset;
        return next;
    }

    constexpr string_const_iterator& operator-=(const difference_type offset) noexcept 
    {
        return *this += -offset;
    }

    [[nodiscard]] constexpr string_const_iterator operator-(const difference_type offset) const noexcept 
    {
        string_const_iterator tmp = *this;
        tmp -= offset;
        return tmp;
    }

    [[nodiscard]] constexpr difference_type operator-(const string_const_iterator& right) const noexcept 
    {
        return _ptr - right._ptr;
    }

    [[nodiscard]] constexpr reference operator[](const difference_type offset) const noexcept 
    {
        return *(*this + offset);
    }

    [[nodiscard]] constexpr bool operator==(const string_const_iterator& right) const noexcept 
    {
        return _ptr == right._ptr;
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(const string_const_iterator& right) const noexcept 
    {
        return _ptr <=> right._ptr;
    }

    pointer _ptr;
};

template <class StringViewT, class CharT>
concept IsStringViewish =
    requires(const StringViewT s)
{
    { s.data() } -> std::convertible_to<CharT const*>;
    { s.size() } -> std::convertible_to<std::size_t>;
};


template <typename T, class TraitsT = std::char_traits<T>, typename AllocatorT = std::allocator<T>>
    requires (!std::is_array_v<T>) && std::is_trivial_v<T> && std::is_standard_layout_v<T>
class shared_data final
{
public:
    using traits_type = TraitsT;
    using allocator_type = AllocatorT;
    using allocator_traits = std::allocator_traits<allocator_type>;

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    shared_data(const shared_data&) = delete;
    shared_data& operator=(const shared_data&) = delete;
    shared_data(shared_data&&) = delete;
    shared_data& operator=(shared_data&&) = delete;

    struct deleter final
    {
        void operator()(shared_data* p) noexcept
        {
            if (p)
                p->release();
        }
    };

    using ptr = std::unique_ptr<shared_data, deleter>;

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

    [[nodiscard]] static shared_data* create(size_type capacity, const value_type* source, size_type size, const allocator_type& allocator = allocator_type())
    {
        if (size > max_size())
            throw std::length_error("Cannot create string this long");

        _raw_allocator a = allocator;
        size_type allocation_size = sizeof(value_type) * (capacity + 1) + padded_header_size();
        auto raw = a.allocate(allocation_size);
        if (!raw) [[unlikely]]
            return nullptr; // allocator decides whether to throw or not

        // buffer contents are left uninitialized, no ctors called 
        auto result = new (static_cast<void*>(raw)) shared_data(std::move(a), capacity, source, size);
        *(result->data() + size) = value_type{}; // always null-terminate
        return result;
    }

    [[nodiscard]] static constexpr size_type max_size() noexcept
    {
        return (std::numeric_limits<size_type>::max() - padded_header_size()) / sizeof(value_type) - 1; // for '\0'
    }

    [[nodiscard]] constexpr void add_ref() const noexcept
    {
        m_refs++;
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

    void append(const value_type* source, size_type size)
    {
        if (size) [[likely]]
        {
            assert(source);
            if (size > m_capacity - m_size)
                throw std::length_error("Not enough room left");

            traits_type::copy(data() + m_size, source, size);
            m_size += size;

            *(data() + m_size) = value_type{}; // always null-terminate
        }
    }

private:
    template <class Al, class U>
    using _rebind_alloc = typename std::allocator_traits<Al>::template rebind_alloc<U>;

    using _raw_allocator = _rebind_alloc<allocator_type, std::byte>;

    static constexpr size_type padded_header_size() noexcept;

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
            assert(source);
            assert(size <= m_capacity);
            traits_type::copy(data(), source, size);
        }
    }

    _raw_allocator m_allocator;
    mutable std::atomic<size_type> m_refs;
    size_type m_capacity;
    size_type m_size;
};

template <typename T, class TraitsT, typename AllocatorT>
    requires (!std::is_array_v<T>) && std::is_trivial_v<T> && std::is_standard_layout_v<T>
constexpr typename shared_data<T, TraitsT, AllocatorT>::size_type shared_data<T, TraitsT, AllocatorT>::padded_header_size() noexcept
{
    constexpr size_type alignment = alignof(value_type) < alignof(void*) ? alignof(void*) : alignof(value_type);
    constexpr size_type header_size = (sizeof(shared_data) + alignment - 1) & (~(alignment - 1));
    return header_size;
}


template <class SharedDataT>
struct size_and_pointers
{
    using value_type = typename SharedDataT::value_type;
    using size_type = typename SharedDataT::size_type;

    // assume pointers are at least 4-bytes aligned, so we can steal two lowest bytes
    static size_type const IsSsoString = 0x1;
    static size_type const IsNullTerminated = 0x2;
    static size_type const PointerMask = ~(IsSsoString | IsNullTerminated);
    static size_type const MaxSize = std::numeric_limits<size_type>::max() - 1; 

    union pointer_and_flags
    {
        size_type flags;
        SharedDataT* shared;
    } u;

    size_type size;
    value_type const* string_data;

    constexpr void initialize(SharedDataT* shared, value_type const* str, size_type sz, bool null_terminated) noexcept
    {
        u.shared = shared; // no add_ref()

        assert((u.flags & ~PointerMask) == 0); // check for misaligned pointer
        if (null_terminated)
            u.flags |= IsNullTerminated;

        size = sz;
        string_data = str;
    }

    [[nodiscard]] constexpr SharedDataT* get_shared() const noexcept
    {
        pointer_and_flags tmp;

        tmp.shared = u.shared;
        tmp.flags &= PointerMask;

        return tmp.shared;
    }
    
    [[nodiscard]] constexpr bool is_null_terminated() const noexcept
    {
        return (u.flags & (IsSsoString | IsNullTerminated)) != 0;
    }

    [[nodiscard]] constexpr bool is_sso() const noexcept
    {
        return (u.flags & IsSsoString) != 0;
    }

    constexpr void swap(size_and_pointers& other) noexcept
    {
        using std::swap;
        swap(u.flags, other.u.flags);
        swap(size, other.size);
        swap(string_data, other.string_data);
    }
};


template <typename CharT, typename = void>
struct raw_from_char;

template <typename CharT>
struct raw_from_char<CharT, std::enable_if_t<sizeof(CharT) == 1, void>>
{
    using raw_type = std::uint8_t;
};

template <typename CharT>
struct raw_from_char<CharT, std::enable_if_t<sizeof(CharT) == 2, void>>
{
    using raw_type = std::uint16_t;
};

template <typename CharT>
struct raw_from_char<CharT, std::enable_if_t<sizeof(CharT) == 4, void>>
{
    using raw_type = std::uint32_t;
};


template <class SharedDataT>
struct sso_storage
{
    using traits_type = typename SharedDataT::traits_type;
    using value_type = typename SharedDataT::value_type;
    using size_type = typename SharedDataT::size_type;
    using raw_type = typename raw_from_char<value_type>::raw_type;
    using twin_type = size_and_pointers<SharedDataT>;

    static_assert(sizeof(value_type) == sizeof(raw_type));
    static_assert(alignof(value_type) == alignof(raw_type));

    static raw_type const IsSsoString = raw_type(twin_type::IsSsoString);
    static raw_type const IsNullTerminated = raw_type(twin_type::IsNullTerminated); // however, SSO string is always null-terminated
    static raw_type const SizeMask = ~(IsSsoString | IsNullTerminated);
    static unsigned const SizeShift = 2;

    static constexpr size_type SsoAreaSize = sizeof(twin_type) / sizeof(value_type);
    static constexpr size_type MaxSize = SsoAreaSize - 2; // 1 for size_and_flags, 1 for '\0'

    union
    {
        raw_type size_and_flags;
        value_type string_data[SsoAreaSize];
    };

    [[nodiscard]] constexpr value_type const* data() const noexcept
    {
        return &string_data[1];
    }

    [[nodiscard]] constexpr value_type* data() noexcept
    {
        return &string_data[1];
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return size_type(size_and_flags & SizeMask) >> SizeShift;
    }

    constexpr void initialize(const value_type* src, size_type size) noexcept
    {
        assert(src);
        assert(size > 0);
        assert(size <= MaxSize);
        size_and_flags = static_cast<raw_type>((size << SizeShift) | IsSsoString | IsNullTerminated);
        auto d = data();
        traits_type::copy(d, src, size);
        // always null-terminate SSO
        d[size] = value_type{};
    }
};


} // namespace detail {}


template <class CharT, class TraitsT = std::char_traits<CharT>, class AllocatorT = std::allocator<CharT>>
class basic_immutable_string final
{
private:
    static_assert(std::is_same_v<CharT, typename TraitsT::char_type>);

    template <class Al, class U>
    using _rebind_alloc = typename std::allocator_traits<Al>::template rebind_alloc<U>;

    using _allocator = _rebind_alloc<AllocatorT, CharT>;
    using _allocator_traits = std::allocator_traits<_allocator>;

    using _shared_data = detail::shared_data<CharT, TraitsT, AllocatorT>;

public:
    struct FromStringLiteralT {};
    static constexpr FromStringLiteralT FromStringLiteral = {}; // this implies null terminator presence

    using traits_type = TraitsT;
    using allocator_type = AllocatorT;

    using value_type = CharT;
    using size_type = typename _allocator_traits::size_type;
    using difference_type = typename _allocator_traits::difference_type;
    using pointer = typename _allocator_traits::pointer;
    using const_pointer = typename _allocator_traits::const_pointer;
    using reference = value_type&;
    using const_reference = const value_type&;

private:
    union _universal_string_storage
    {
        using sso_storage_t = detail::sso_storage<_shared_data>;
        using size_and_pointers_t = detail::size_and_pointers<_shared_data>;
        
        sso_storage_t sso;
        size_and_pointers_t ptrs;

        static_assert(sizeof(sso) == sizeof(ptrs));

        constexpr _universal_string_storage() noexcept
        {
            ptrs.initialize(nullptr, &_e, 0, true);
        }

        constexpr _universal_string_storage(const value_type* src, FromStringLiteralT) noexcept
        {
            assert(src);
            auto sz = traits_type::length(src);
            ptrs.initialize(nullptr, src, sz, true);
        }

        constexpr _universal_string_storage(const value_type* src, size_type size, FromStringLiteralT) noexcept
        {
            assert(src);
            assert(traits_type::length(src) == size);
            ptrs.initialize(nullptr, src, size, true);
        }

        _universal_string_storage(const value_type* src, size_type size, const allocator_type& al)
        {
            assert((size == 0) || !!src);

            if (size == size_type(-1))
                size = traits_type::length(src);

            if (size > size_and_pointers_t::MaxSize)
                throw std::length_error("Cannot create string this long");

            if (!size) [[unlikely]]
            {
                ptrs.initialize(nullptr, &_e, 0, true);
            }
            else if (size <= sso_storage_t::MaxSize)
            {
                // we can do SSO
                sso.initialize(src, size);
            }
            else
            {
                auto sd = _shared_data::create(size, src, size, al);
                ptrs.initialize(sd, sd->data(), size, true); // shared_data does null-terminate
            }
        }

        constexpr _universal_string_storage(_shared_data* stg, const_pointer str, size_type sz, bool null_terminated) noexcept
        {
            assert(stg);
            ptrs.initialize(stg, str, sz, null_terminated); // no add_ref()
        }

        constexpr _universal_string_storage(const _universal_string_storage& other) noexcept
        {
            std::memcpy(&ptrs, &other.ptrs, sizeof(ptrs));
            if (!ptrs.is_sso())
            {
                auto sd = ptrs.get_shared();
                if (sd)
                    sd->add_ref();
            }
        }

        constexpr void swap(_universal_string_storage& other) noexcept
        {
            ptrs.swap(other.ptrs);
        }
    };

public:
    static constexpr size_type npos = size_type(-1);

    using const_iterator = detail::string_const_iterator<basic_immutable_string>;
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;

    ~basic_immutable_string()
    {
        _release();
    }

    constexpr basic_immutable_string() noexcept
        : m_storage()
    {
    }

    basic_immutable_string(nullptr_t) = delete;

    constexpr basic_immutable_string(const_pointer source, FromStringLiteralT) noexcept
        : m_storage(source, FromStringLiteral)
    {
    }

    constexpr basic_immutable_string(const_pointer source, size_type size, FromStringLiteralT) noexcept
        : m_storage(source, size, FromStringLiteral)
    {
    }

    basic_immutable_string(const_pointer source, size_type size = size_type(-1), const allocator_type& a = allocator_type())
        : m_storage(source, size, a)
    {
    }

    template <detail::IsStringViewish<value_type> StringViewT>
    basic_immutable_string(const StringViewT& str, const allocator_type& a = allocator_type())
        : basic_immutable_string(str.data(), str.size(), a)
    {
    }

#if TESTING
public:
#else
private:
#endif
    [[nodiscard]] constexpr bool _is_shared() const noexcept
    {
        if (_is_short())
            return false;

        return !!m_storage.ptrs.get_shared();
    }


    [[nodiscard]] constexpr bool _is_short() const noexcept
    {
        return m_storage.ptrs.is_sso();
    }

    [[nodiscard]] constexpr bool _has_null_terminator() const noexcept
    {
        return m_storage.ptrs.is_null_terminated();
    }

public:
    basic_immutable_string(const basic_immutable_string& other) noexcept
        : m_storage(other.m_storage)
    {
    }

    basic_immutable_string& operator=(const basic_immutable_string& other) noexcept
    {
        basic_immutable_string tmp(other);
        swap(tmp);
        return *this;
    }

    basic_immutable_string(basic_immutable_string&& other) noexcept
        : basic_immutable_string()
    {
        swap(other);
    }

    basic_immutable_string& operator=(basic_immutable_string&& other) noexcept
    {
        basic_immutable_string tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    void swap(basic_immutable_string& other) noexcept
    {
        m_storage.swap(other.m_storage);
    }

    friend void swap(basic_immutable_string& a, basic_immutable_string& b) noexcept
    {
        a.swap(b);
    }

    [[nodiscard]] constexpr const_pointer c_str() const
    {
        if (_has_null_terminator())
            return data();

        assert(!empty());
        auto clone = _make_cstr();
        
        _release();
        auto d = clone->data();
        m_storage.ptrs.initialize(clone, d, clone->size(), true);

        return d;
    }

    [[nodiscard]] constexpr size_type length() const noexcept
    {
        return _is_short() ? m_storage.sso.size() : m_storage.ptrs.size;
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return _is_short() ? m_storage.sso.data() : m_storage.ptrs.string_data;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return length();
    }

    [[nodiscard]] static constexpr size_type max_size() noexcept
    {
        return _shared_data::max_size();
    }

    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
    {
        auto stg = _get_shared_no_add_ref();
        if (stg)
            return stg->get_allocator();

        return allocator_type();
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    [[nodiscard]] constexpr basic_immutable_string substr(size_type start, size_type len = npos) const
    {
        auto const sz = size();
        if (start > sz) [[unlikely]]
            throw std::out_of_range("Start position for basic_immutable_string::substr() exceeds string length");

        if (len == npos) [[likely]]
            len = sz - start;
        else if (len + start > sz) [[unlikely]]
            len = sz - start;

        if (!len) [[unlikely]]
            return basic_immutable_string();

        auto stg = _get_shared_add_ref();
        if (!stg)
            return basic_immutable_string(data() + start, len);

        return basic_immutable_string(stg, data() + start, len, false);
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return const_iterator{ data() };
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return const_iterator{ data() + size() };
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return end();
    }

    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }

    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
    {
        return rend();
    }

    [[nodiscard]] constexpr bool operator==(const basic_immutable_string& o) noexcept
    {
        auto asz = size();
        auto bsz = o.size();
        if (asz != bsz)
            return false;

        if (asz == 0)
            return true;

        auto ad = data();
        auto bd = o.data();
        if (ad == bd)
            return true;

        return traits_type::compare(ad, bd, asz) == 0;
    }

    [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept
    {
        assert(index < size());
        return data() + index;
    }

    constexpr size_type copy(pointer dest, size_type count, size_type pos = 0) const
    {
        assert(dest);
        auto const sz = size();
        if (pos > sz) [[unlikely]]
            throw std::out_of_range("Trying to copy from beyond the end of the string");

        if (count + pos > sz) [[unlikely]]
            count = sz - pos;

        if (!count) [[unlikely]]
            return count;

        auto const src = data() + pos;
        traits_type::copy(dest, src, count);

        return count;
    }

    template <detail::IsStringViewish<value_type> StringViewT>
    [[nodiscard]] constexpr size_type find(const StringViewT& what, size_type start_pos = 0) const noexcept
    {
        assert(what.data());
        return _traits_find(data(), size(), start_pos, what.data(), what().size());
    }

    [[nodiscard]] constexpr size_type find(const value_type* what, size_type start_pos = 0) const noexcept
    {
        assert(what);
        auto length = traits_type::length(what);
        return _traits_find(data(), size(), start_pos, what, length);
    }

    [[nodiscard]] constexpr size_type find(value_type ch, size_type start_pos = 0) const noexcept
    {
        return _traits_find_ch(data(), size(), start_pos, ch);
    }

    template <detail::IsStringViewish<value_type> StringViewT>
    [[nodiscard]] constexpr size_type rfind(const StringViewT& what, size_type start_pos = npos) const noexcept
    {
        assert(what.data());
        return _traits_rfind(data(), size(), start_pos, what.data(), what().size());
    }

    [[nodiscard]] constexpr size_type rfind(const value_type* what, size_type start_pos = npos) const noexcept
    {
        assert(what);
        auto length = traits_type::length(what);
        return _traits_rfind(data(), size(), start_pos, what, length);
    }

    [[nodiscard]] constexpr size_type rfind(value_type ch, size_type start_pos = npos) const noexcept
    {
        return _traits_rfind_ch(data(), size(), start_pos, ch);
    }

    class builder final
    {
    public:
        ~builder() = default;

        builder(size_type reserve = 4096, const allocator_type& a = allocator_type())
            : m_storage(_shared_data::create(std::max(reserve, MinReserve), nullptr, 0, a))

        {
        }

        builder(const builder&) = delete;
        builder& operator=(const builder&) = delete;
        builder(builder&&) = default;
        builder& operator=(builder&&) = default;

        template <detail::IsStringViewish<value_type> StringT>
        builder& append(const StringT& str)
        {
            auto add_sz = str.size();
            if (!add_sz) [[unlikely]]
                return *this;

            auto my_sz = m_storage->size();
            auto my_cap = m_storage->capacity();
            auto new_sz = my_sz + add_sz;
            if (new_sz <= my_cap) [[likely]]
            {
                m_storage->append(str.data(), add_sz);
            }
            else
            {
                size_type new_cap = std::max(new_sz, my_cap + my_cap / 2);
                typename _shared_data::ptr new_storage(_shared_data::create(new_cap, m_storage->data(), my_sz, m_storage->get_allocator()));
                new_storage->append(str.data(), add_sz);
                m_storage.swap(new_storage);
            }

            return *this;
        }

        builder& append(const char* str)
        {
            return append(std::basic_string_view<value_type, traits_type>(str));
        }

        basic_immutable_string str() const
        {
            m_storage->add_ref();
            return basic_immutable_string(m_storage.get(), m_storage->data(), m_storage->size(), true);
        }

    private:
        static const size_type MinReserve = 1024;
        _shared_data::ptr m_storage;
    };

    template <class StringT>
    [[nodiscard]] friend builder operator+(const basic_immutable_string& a, StringT&& b)
    {
        builder bld;
        bld.append(a).append(std::forward<StringT>(b));
        return bld;
    }

    template <class StringT>
    [[nodiscard]] friend builder&& operator+(builder&& bld, StringT&& a)
    {
        bld.append(std::forward<StringT>(a));
        return std::move(bld);
    }

    basic_immutable_string(const builder& b)
        : basic_immutable_string(b.str())
    {
    }

    template <class OStreamT>
    friend OStreamT& operator<<(OStreamT& stream, const basic_immutable_string& str)
    {
        stream << std::basic_string_view<value_type, traits_type>(str.data(), str.size());
        return stream;
    }

private:
    constexpr basic_immutable_string(_shared_data* stg, const_pointer str, size_type sz, bool null_terminated) noexcept
        : m_storage(stg, str, sz, null_terminated) //  no add_ref()
    {
    }

    [[nodiscard]] constexpr _shared_data* _get_shared_no_add_ref() const noexcept
    {
        return !_is_short() ? m_storage.ptrs.get_shared() : nullptr;
    }

    [[nodiscard]] constexpr _shared_data* _get_shared_add_ref() const noexcept
    {
        auto ptr = _get_shared_no_add_ref();
        if (ptr)
            ptr->add_ref();

        return ptr;
    }

    [[nodiscard]] _shared_data* _make_cstr() const
    {
        auto len = size();
        auto storage = _shared_data::create(len + 1, data(), len, get_allocator());
        if (!storage) [[unlikely]]
            throw std::bad_alloc();
                
        return storage;
    }

    void _release() const noexcept
    {
        //auto sd = m_storage.ptrs
        auto sd = _get_shared_no_add_ref();
        if (sd)
            sd->release();
    }

    static constexpr size_type _traits_find(const_pointer haystack, size_type hay_size, size_type start_pos, const_pointer needle, size_type needle_size) noexcept
    {
        // search [haystack, haystack + hay_size) for [needle, needle + needle_size), at/after start_pos
        if (needle_size > hay_size || start_pos > hay_size - needle_size) 
        {
            return npos;
        }

        if (needle_size == 0) 
        { 
            // empty string always matches
            return start_pos;
        }

        const auto possible_matches_end = haystack + (hay_size - needle_size) + 1;
        for (auto match_try = haystack + start_pos;; ++match_try) 
        {
            match_try = traits_type::find(match_try, static_cast<size_type>(possible_matches_end - match_try), *needle);
            if (!match_try) 
            { 
                // didn't find first character
                return npos;
            }

            if (traits_type::compare(match_try, needle, needle_size) == 0) 
            { 
                // match
                return static_cast<size_type>(match_try - haystack);
            }
        }
    }

    static constexpr size_type _traits_find_ch(const_pointer haystack, size_type hay_size, size_type start_pos, value_type ch) noexcept
    {
        // search [haystack, haystack + hay_size) for ch, at/after start_pos
        if (start_pos < hay_size) 
        {
            const auto found_at = traits_type::find(haystack + start_pos, hay_size - start_pos, ch);
            if (found_at) 
            {
                return static_cast<size_type>(found_at - haystack);
            }
        }

        return npos; // no match
    }

    static constexpr size_type _traits_rfind(const_pointer haystack, size_type hay_size, size_type start_pos, const_pointer needle, size_type needle_size) noexcept
    {
        // search [haystack, haystack + hay_size) for [needle, needle + needle_size) beginning before start_pos
        if (needle_size == 0) 
        {
            return std::min(start_pos, hay_size); // empty string always matches
        }

        if (needle_size <= hay_size) 
        { 
            // room for match, look for it
            for (auto match_try = haystack + std::min(start_pos, hay_size - needle_size);; --match_try) 
            {
                if (traits_type::eq(*match_try, *needle) && traits_type::compare(match_try, needle, needle_size) == 0) 
                {
                    return static_cast<size_type>(match_try - haystack); // found a match
                }

                if (match_try == haystack) 
                {
                    break; // at beginning, no more chance for match
                }
            }
        }

        return npos; // no match
    }

    static constexpr size_type _traits_rfind_ch(const_pointer haystack, size_type hay_size, size_type start_pos, value_type ch) noexcept 
    {
        // search [haystack, haystack + hay_size) for ch before start_pos
        if (hay_size != 0) 
        { 
            // room for match, look for it
            for (auto match_try = haystack + std::min(start_pos, hay_size - 1);; --match_try) 
            {
                if (traits_type::eq(*match_try, ch)) 
                {
                    return static_cast<size_type>(match_try - haystack); // found a match
                }

                if (match_try == haystack) 
                {
                    break; // at beginning, no more chance for match
                }
            }
        }

        return npos; // no match
    }

    mutable _universal_string_storage m_storage;

    static constexpr value_type _e = { value_type{} };
};


using immutable_string = basic_immutable_string<char, std::char_traits<char>, std::allocator<char>>;
using immutable_wstring = basic_immutable_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;

} // namespace ims {}