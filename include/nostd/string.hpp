// Copyright (C) 2022-2023  ilobilo

#pragma once

// Just in case, because we can't ignore some warnings from `-Wpedantic` (about zero size arrays and anonymous structs when gnu extensions are disabled) on gcc
#if defined(__clang__)
#  pragma clang system_header
#elif defined(__GNUC__)
#  pragma GCC system_header
#endif

#include <initializer_list>  // for std::initializer_list
#include <string_view>       // for std::basic_string_view
#include <type_traits>       // for std::is_constant_evaluated, std::declval, std::false_type
#include <algorithm>         // for std::min, std::max
#include <concepts>          // for std::unsigned_integral, std::signed_integral
#include <iterator>          // for std::distance, std::next, std::iterator_traits, std::input_iterator
#include <utility>           // for std::move, std::hash
#include <compare>           // for std::strong_ordering
#include <memory>            // for std::allocator, std::swap, std::allocator_traits

#include <cstdint>
#include <cstddef>

#ifndef NOSTD_STRING_STD_HASH
#  define NOSTD_STRING_STD_HASH 1
#endif

#ifndef NOSTD_STRING_NUMERIC_CONVERSIONS
#  define NOSTD_STRING_NUMERIC_CONVERSIONS 1
#endif

#if NOSTD_STRING_NUMERIC_CONVERSIONS && !__has_include(<cstdlib>)
#  undef NOSTD_STRING_NUMERIC_CONVERSIONS
#  define NOSTD_STRING_NUMERIC_CONVERSIONS 0
#endif

#if NOSTD_STRING_NUMERIC_CONVERSIONS
#  include <cstdlib>
#endif

#ifndef NOSTD_STRING_FLOAT
#  define NOSTD_STRING_FLOAT 1
#endif

#if NOSTD_STRING_FLOAT && !NOSTD_STRING_NUMERIC_CONVERSIONS
#  undef NOSTD_STRING_FLOAT
#  define NOSTD_STRING_FLOAT 0
#endif

#ifndef NOSTD_STRING_LONG_DOUBLE
#  define NOSTD_STRING_LONG_DOUBLE 1
#endif

#if NOSTD_STRING_LONG_DOUBLE && !NOSTD_STRING_FLOAT
#  undef NOSTD_STRING_LONG_DOUBLE
#  define NOSTD_STRING_LONG_DOUBLE 0
#endif

#ifndef NOSTD_STRING_CONTAINERS_RANGES
#  define NOSTD_STRING_CONTAINERS_RANGES 1
#endif

#if NOSTD_STRING_CONTAINERS_RANGES && (__cplusplus <= 202002L || !__has_include(<ranges>) || !defined(__cpp_lib_containers_ranges))
#  undef NOSTD_STRING_CONTAINERS_RANGES
#  define NOSTD_STRING_CONTAINERS_RANGES 0
#endif

#if NOSTD_STRING_CONTAINERS_RANGES
#  include <ranges>
#endif

#ifndef NOSTD_STRING_EXCEPTIONS
#  if __cpp_exceptions
#    define NOSTD_STRING_EXCEPTIONS 1
#  else
#    define NOSTD_STRING_EXCEPTIONS 0
#  endif
#endif

#if NOSTD_STRING_EXCEPTIONS && (!__cpp_exceptions || !__has_include(<stdexcept>))
#  undef NOSTD_STRING_EXCEPTIONS
#  define NOSTD_STRING_EXCEPTIONS 0
#endif

#ifndef NOSTD_STRING_FALLBACK_ASSERT
#  define NOSTD_STRING_FALLBACK_ASSERT 1
#endif

#if NOSTD_STRING_FALLBACK_ASSERT && !__has_include(<cassert>)
#  undef NOSTD_STRING_FALLBACK_ASSERT
#  define NOSTD_STRING_FALLBACK_ASSERT 0
#endif

#ifndef NOSTD_STRING_FALLBACK_ABORT
#  define NOSTD_STRING_FALLBACK_ABORT 1
#endif

#if NOSTD_STRING_FALLBACK_ABORT && !__has_include(<cstdlib>)
#  undef NOSTD_STRING_FALLBACK_ABORT
#  define NOSTD_STRING_FALLBACK_ABORT 0
#endif

#ifndef NOSTD_STRING_FALLBACK_ABORT_FUNCTION
#  define NOSTD_STRING_FALLBACK_ABORT_FUNCTION [] (auto) { }
#endif

#if NOSTD_STRING_EXCEPTIONS
#  include <stdexcept>
#  define _NOSTD_STRING_ASSERT(x, str, e) do { if (!(x)) throw e(str); } while (0)
#elif NOSTD_STRING_FALLBACK_ASSERT
#  include <cassert>
#  define _NOSTD_STRING_ASSERT(x, str, ...) assert(x && str)
#elif NOSTD_STRING_FALLBACK_ABORT
#  if !NOSTD_STRING_NUMERIC_CONVERSIONS
#    include <cstdlib>
#  endif
#  define _NOSTD_STRING_ASSERT(x, ...) do { if (!(x)) { std::abort(); } } while (0)
#else
#  define _NOSTD_STRING_ASSERT(x, str, ...) do { if (!(x)) { NOSTD_STRING_FALLBACK_ABORT_FUNCTION (str); { while (true) { [] { } (); } } } } while (0)
#endif

#define _NOSTD_STRING_PRAGMA_IMPL(x) _Pragma(#x)
#define _NOSTD_STRING_PRAGMA(x) _NOSTD_STRING_PRAGMA_IMPL(x)

#if defined(__clang__)
#  define _NOSTD_STRING_PRAGMA_DIAG_PREFIX clang
#elif defined(__GNUC__)
#  define _NOSTD_STRING_PRAGMA_DIAG_PREFIX GCC
#endif

#define _NOSTD_STRING_DIAG_PUSH() _NOSTD_STRING_PRAGMA(_NOSTD_STRING_PRAGMA_DIAG_PREFIX diagnostic push)
#define _NOSTD_STRING_DIAG_IGN(wrn) _NOSTD_STRING_PRAGMA(_NOSTD_STRING_PRAGMA_DIAG_PREFIX diagnostic ignored wrn)
#define _NOSTD_STRING_DIAG_POP() _NOSTD_STRING_PRAGMA(_NOSTD_STRING_PRAGMA_DIAG_PREFIX diagnostic pop)

namespace nostd
{
    namespace detail
    {
        template<typename Allocator, typename = void>
        struct is_allocator : std::false_type { };

        template<typename Allocator>
        struct is_allocator<Allocator, std::void_t<typename Allocator::value_type, decltype(std::declval<Allocator&>().allocate(std::size_t { }))>> : std::true_type { };

        template<typename Allocator>
        constexpr inline bool is_allocator_v = is_allocator<Allocator>::value;

        struct uninitialized_size_tag { };

        template<typename>
        constexpr bool dependent_false = false;

#if NOSTD_STRING_CONTAINERS_RANGES
        template<typename Range, typename Type>
        concept container_compatible_range = std::ranges::input_range<Range> && std::convertible_to<std::ranges::range_reference_t<Range>, Type>;
#endif
    } // namespace detail

    // basic_string
    // based on implementations from libc++, libstdc++ and Microsoft STL
    template<typename Char, typename Traits = std::char_traits<Char>, typename Allocator = std::allocator<Char>>
    class basic_string
    {
        using alloc_traits = std::allocator_traits<Allocator>;
        using sview_type = std::basic_string_view<Char, Traits>;

        public:
        using traits_type = Traits;
        using value_type = typename traits_type::char_type;
        using allocator_type = Allocator;
        using size_type = typename alloc_traits::size_type;
        using difference_type = typename alloc_traits::difference_type;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = typename alloc_traits::pointer;
        using const_pointer = typename alloc_traits::const_pointer;
        using iterator = value_type *;
        using const_iterator = const value_type *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        constexpr static size_type npos = -1;

        private:
        constexpr static auto _terminator = value_type();

        Allocator _allocator;

_NOSTD_STRING_DIAG_PUSH()

#if defined(__clang__)
  _NOSTD_STRING_DIAG_IGN("-Wgnu-anonymous-struct")
  _NOSTD_STRING_DIAG_IGN("-Wzero-length-array")
#elif defined(__GNUC__)
  _NOSTD_STRING_DIAG_IGN("-Wpedantic") // this doesn't work
#endif

        struct long_data
        {
            struct [[gnu::packed]]
            {
                size_type is_long : 1;
                size_type cap : sizeof(std::size_t) * __CHAR_BIT__ - 1;
            };
            size_type size;
            pointer data;
        };

        enum { min_cap = (sizeof(long_data) - 1) / sizeof(value_type) > 2 ? (sizeof(long_data) - 1) / sizeof(value_type) : 2 };
        struct short_data
        {
            struct [[gnu::packed]]
            {
                unsigned char is_long : 1;
                unsigned char size : 7;
            };
            char padding[sizeof(value_type) - 1];
            value_type data[min_cap];
        };

_NOSTD_STRING_DIAG_POP()

        static_assert(sizeof(short_data) == (sizeof(value_type) * (min_cap + 1)), "short_data has an unexpected size.");

        union storage_t
        {
            long_data _long;
            short_data _short { { false, 0 }, { }, { } };
        } storage;

        constexpr static bool fits_in_sso(size_type size)
        {
            return size < min_cap;
        }

        constexpr void long_init()
        {
            storage._long.is_long = true;
            storage._long.data = nullptr;
            storage._long.size = 0;
            storage._long.cap = 0;
        }

        constexpr void short_init()
        {
            if (auto &buffer = storage._long.data; is_long() && buffer != nullptr)
            {
                _allocator.deallocate(buffer, storage._long.cap + 1);
                buffer = nullptr;
            }

            storage._short.is_long = false;
            storage._short.size = 0;
        }

        constexpr void default_init(size_type size)
        {
            if (fits_in_sso(size))
                short_init();
            else
                long_init();
        }

        constexpr bool is_long() const noexcept
        {
            return storage._short.is_long == true;
        }

        constexpr pointer get_data() noexcept
        {
            return is_long() ? storage._long.data : storage._short.data;
        }

        constexpr const_pointer get_data() const noexcept
        {
            return is_long() ? storage._long.data : storage._short.data;
        }

        constexpr size_type get_size() const noexcept
        {
            return is_long() ? storage._long.size : storage._short.size;
        }

        constexpr void set_size(size_type size) noexcept
        {
            if (is_long())
                storage._long.size = size;
            else
                storage._short.size = size & 0x7F;
        }

        constexpr size_type get_cap() const noexcept
        {
            if (is_long())
                return storage._long.cap;
            else
                return min_cap - 1;
        }

        constexpr sview_type get_view() const noexcept
        {
            return sview_type(get_data(), get_size());
        }

        constexpr void reallocate(std::size_t new_cap, bool copy_old)
        {
            if (new_cap == storage._long.cap)
                return;

            auto old_len = storage._long.size;
            auto old_cap = storage._long.cap;
            auto &old_buffer = storage._long.data;

            auto new_len = std::min(new_cap, old_len);
            auto new_buffer = _allocator.allocate(new_cap + 1);

            if (old_buffer != nullptr)
            {
                if (old_len != 0 && copy_old == true)
                    Traits::copy(new_buffer, old_buffer, new_len);
                _allocator.deallocate(old_buffer, old_cap + 1);
            }

            storage._long.size = new_len;
            storage._long.data = new_buffer;
            storage._long.cap = new_cap;
        }

        constexpr void grow_to(size_type new_cap)
        {
            if (is_long() == true)
            {
                reallocate(new_cap, true);
                return;
            }

            auto buffer = _allocator.allocate(new_cap + 1);
            auto len = storage._short.size;

            Traits::copy(buffer, storage._short.data, len);
            Traits::assign(buffer[len], _terminator);

            long_init();
            storage._long.data = buffer;
            storage._long.size = len;
            storage._long.cap = new_cap;
        }

        constexpr void null_terminate() noexcept
        {
            auto buffer = get_data();
            if (buffer == nullptr)
                return;
            Traits::assign(buffer[get_size()], _terminator);
        }

        constexpr bool addr_in_range(const_pointer ptr) const
        {
            if (std::is_constant_evaluated())
                return false;
            return get_data() <= ptr && ptr <= get_data() + get_size();
        }

        constexpr void internal_replace_impl(auto func, size_type pos, size_type oldcount, size_type count)
        {
            auto cap = get_cap();
            auto sz = get_size();

            auto rsz = sz - oldcount + count;

            if (cap < rsz)
                grow_to(rsz);

            if (oldcount != count)
                Traits::move(get_data() + pos + count, get_data() + pos + oldcount, sz - pos - oldcount);

            func();

            set_size(rsz);
            null_terminate();
        }

        constexpr void internal_replace(size_type pos, const_pointer str, size_type oldcount, size_type count)
        {
            if (addr_in_range(str))
            {
                basic_string rstr(str, count);
                internal_replace_impl([&]() { Traits::copy(get_data() + pos, rstr.data(), count); }, pos, oldcount, count);
            }
            else internal_replace_impl([&]() { Traits::copy(get_data() + pos, str, count); }, pos, oldcount, count);
        }

        constexpr void internal_replace(size_type pos, value_type ch, size_type oldcount, size_type count)
        {
            internal_replace_impl([&]() { Traits::assign(get_data() + pos, count, ch); }, pos, oldcount, count);
        }

        constexpr void internal_insert_impl(auto func, size_type pos, size_type size)
        {
            if (size != 0)
            {
                auto cap = get_cap();
                auto sz = get_size();
                auto rsz = sz + size;

                if (cap < rsz)
                    grow_to(rsz);

                Traits::move(get_data() + pos + size, get_data() + pos, sz - pos);
                func();

                set_size(rsz);
                null_terminate();
            }
        }

        constexpr void internal_insert(size_type pos, const_pointer str, size_type count)
        {
            if (addr_in_range(str))
            {
                basic_string rstr(str, count);
                internal_insert_impl([&]() { Traits::copy(get_data() + pos, rstr.data(), count); }, pos, count);
            }
            else internal_insert_impl([&]() { Traits::copy(get_data() + pos, str, count); }, pos, count);
        }

        constexpr void internal_insert(size_type pos, value_type ch, size_type count)
        {
            internal_insert_impl([&]() { Traits::assign(get_data() + pos, count, ch); }, pos, count);
        }

        constexpr void internal_append_impl(auto func, size_type size)
        {
            if (size != 0)
            {
                auto cap = get_cap();
                auto sz = get_size();
                auto rsz = sz + size;

                if (cap < rsz)
                    grow_to(rsz);

                func(sz);
                set_size(rsz);
                null_terminate();
            }
        }

        constexpr void internal_append(const_pointer str, size_type count)
        {
            if (addr_in_range(str))
            {
                basic_string rstr(str, count);
                internal_append_impl([&](size_type pos) { Traits::copy(get_data() + pos, rstr.data(), count); }, count);
            }
            else internal_append_impl([&](size_type pos) { Traits::copy(get_data() + pos, str, count); }, count);
        }

        constexpr void internal_append(value_type ch, size_type count)
        {
            internal_append_impl([&](size_type pos) { Traits::assign(get_data() + pos, count, ch); }, count);
        }

        constexpr void internal_assign_impl(auto func, size_type size, bool copy_old)
        {
            if (fits_in_sso(size))
            {
                if (is_long() == true)
                    short_init();

                storage._short.size = size;
                func(storage._short.data);
                null_terminate();
            }
            else
            {
                if (is_long() == false)
                    long_init();
                if (storage._long.cap < size)
                    reallocate(size, copy_old);

                func(storage._long.data);
                storage._long.size = size;
                null_terminate();
            }
        }

        constexpr void internal_assign(const_pointer str, size_type size, bool copy_old = false)
        {
            if (addr_in_range(str))
            {
                basic_string rstr(str, size);
                internal_assign_impl([&](auto data) { Traits::copy(data, rstr.data(), size); }, size, copy_old);
            }
            else internal_assign_impl([&](auto data) { Traits::copy(data, str, size); }, size, copy_old);
        }

        constexpr void internal_assign(value_type ch, size_type count, bool copy_old = false)
        {
            internal_assign_impl([&](auto data) { Traits::assign(data, count, ch); }, count, copy_old);
        }

        public:
        explicit basic_string(detail::uninitialized_size_tag, size_type size, const allocator_type &alloc)
            : _allocator { alloc }
        {
            if (!fits_in_sso(size))
            {
                long_init();
                reallocate(size, false);
            }
            else short_init();

            set_size(size);
        }

        constexpr basic_string() noexcept(noexcept(allocator_type { }))
            : basic_string { allocator_type() } { }

        explicit constexpr basic_string(const allocator_type &alloc) noexcept
            : _allocator { alloc }
        {
            short_init();
        }

        constexpr basic_string(size_type count, value_type ch, const allocator_type &alloc = allocator_type { })
            requires (detail::is_allocator_v<Allocator>)
            : _allocator { alloc }
        {
            _NOSTD_STRING_ASSERT(count <= max_size(), "nostd::basic_string::basic_string(): constructed string size would exceed max_size()", std::length_error);
            internal_assign(ch, count);
        }

        constexpr basic_string(const basic_string &str, size_type pos, size_type count, const allocator_type &alloc = allocator_type { })
            : _allocator { alloc }
        {
            _NOSTD_STRING_ASSERT(pos <= str.get_size(), "nostd::basic_string::basic_string(): pos out of range", std::out_of_range);
            const auto len = std::min(count, str.get_size() - pos);
            internal_assign(str.data() + pos, len);
        }

        constexpr basic_string(const basic_string &str, size_type pos, const allocator_type &alloc = allocator_type { })
            : basic_string { str, pos, npos, alloc } { }

        constexpr basic_string(const value_type *str, size_type count, const allocator_type &alloc = allocator_type { })
            : _allocator { alloc }
        {
            _NOSTD_STRING_ASSERT(count <= max_size(), "nostd::basic_string::basic_string(): constructed string size would exceed max_size()", std::length_error);
            internal_assign(str, count);
        }

        constexpr basic_string(const value_type *str, const allocator_type &alloc = allocator_type { })
            requires (detail::is_allocator_v<Allocator>)
            : basic_string { str, Traits::length(str), alloc } { }

        template<std::input_iterator InputIterator>
        constexpr basic_string(InputIterator first, InputIterator last, const allocator_type &alloc = allocator_type { })
            : _allocator { alloc }
        {
            const auto len = std::distance(first, last);
            _NOSTD_STRING_ASSERT(len <= max_size(), "nostd::basic_string::basic_string(): constructed string size would exceed max_size()", std::length_error);
            internal_assign(const_pointer { first }, len);
        }

        constexpr basic_string(const basic_string &str, const allocator_type &alloc)
            : _allocator { alloc }
        {
            const auto len = str.length();
            internal_assign(str.data(), len);
        }

        constexpr basic_string(const basic_string &str)
            : basic_string { str, allocator_type { } } { }

        constexpr basic_string(basic_string &&str, const allocator_type &alloc)
            : _allocator { alloc }, storage { std::move(str.storage) }
        {
            if (str.is_long() && alloc != str._allocator)
            {
                const auto len = str.storage._long.size;
                internal_assign(str.storage._long.data, len);
            }
            else
            {
                storage = str.storage;
                str.short_init();
            }
        }
        constexpr basic_string(basic_string &&str) noexcept
            : basic_string { str, std::move(str._allocator) } { }

        constexpr basic_string(std::initializer_list<value_type> ilist, const allocator_type &alloc = allocator_type { })
            : _allocator { alloc }
        {
            const auto len = ilist.size();
            _NOSTD_STRING_ASSERT(len <= max_size(), "nostd::basic_string::basic_string(): constructed string size would exceed max_size()", std::length_error);
            internal_assign(const_pointer { ilist.begin() }, len);
        }

        template<typename Type> requires (std::is_convertible_v<const Type &, sview_type>)
        constexpr basic_string(const Type &t, size_type pos, size_type count, const allocator_type &alloc = allocator_type { })
            : _allocator { alloc }
        {
            sview_type sv { t };
            _NOSTD_STRING_ASSERT(pos <= sv.length(), "nostd::basic_string::basic_string(): pos out of range", std::out_of_range);

            sview_type ssv = sv.substr(pos, count);
            const auto len = ssv.length();
            _NOSTD_STRING_ASSERT(len <= max_size(), "nostd::basic_string::basic_string(): constructed string size would exceed max_size()", std::length_error);
            internal_assign(ssv.data(), len);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        explicit constexpr basic_string(const Type &t, const allocator_type &alloc = allocator_type { })
            : _allocator { alloc }
        {
            sview_type sv { t };
            const auto len = sv.length();
            _NOSTD_STRING_ASSERT(len <= max_size(), "nostd::basic_string::basic_string(): constructed string size would exceed max_size()", std::length_error);
            internal_assign(sv.data(), len);
        }

        // constexpr basic_string(basic_string &&str, size_type pos, size_type count, const allocator_type &a = allocator_type());
        // constexpr basic_string(basic_string &&str, size_type pos, const allocator_type &a = allocator_type());

#if __cplusplus > 202002L
        basic_string(std::nullptr_t) = delete;
#endif

#if NOSTD_STRING_CONTAINERS_RANGES
        template<detail::container_compatible_range<Char> Range>
        constexpr basic_string(std::from_range_t, Range &&range, const Allocator &alloc = Allocator { })
            : basic_string { std::ranges::begin(range), std::ranges::end(range), alloc } { }
#endif

        constexpr ~basic_string()
        {
            if (is_long())
            {
                if (auto buffer = storage._long.data)
                    _allocator.deallocate(buffer, storage._long.cap + 1);
            }
        }

        constexpr basic_string &operator=(const basic_string &str)
        {
            return assign(str);
        }

        constexpr basic_string &operator=(basic_string &&str) noexcept (
            alloc_traits::propagate_on_container_move_assignment::value ||
            alloc_traits::is_always_equal::value
        )
        {
            return assign(str);
        }

        constexpr basic_string &operator=(const value_type *str)
        {
            return assign(str, Traits::length(str));
        }

        constexpr basic_string &operator=(value_type ch)
        {
            return assign(addressof(ch), 1);
        }

        constexpr basic_string &operator=(std::initializer_list<value_type> ilist)
        {
            return assign(ilist.begin(), ilist.size());
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &operator=(const Type &t)
        {
            sview_type sv(t);
            return assign(sv);
        }

#if __cplusplus > 202002L
        constexpr basic_string &operator=(std::nullptr_t) = delete;
#endif

        constexpr basic_string &assign(size_type count, value_type ch)
        {
            _NOSTD_STRING_ASSERT(count <= max_size(), "nostd::basic_string::basic_string(): resulted string size would exceed max_size()", std::length_error);
            internal_assign(ch, count);
            return *this;
        }

        constexpr basic_string &assign(const basic_string &str)
        {
            internal_assign(str.data(), str.size());
            return *this;
        }

        constexpr basic_string &assign(const basic_string &str, size_type pos, size_type count = npos)
        {
            _NOSTD_STRING_ASSERT(pos <= str.get_size(), "nostd::basic_string::assign(): pos out of range", std::out_of_range);
            internal_assign(str.data(), std::min(count, str.size() - pos));
            return *this;
        }

        constexpr basic_string &assign(basic_string &&str) noexcept (
            alloc_traits::propagate_on_container_move_assignment::value ||
            alloc_traits::is_always_equal::value
        )
        {
            if (_allocator == str._allocator)
                swap(str);
            else
                internal_assign(str.data(), str.size());

            return *this;
        }

        constexpr basic_string &assign(const value_type *str, size_type count)
        {
            _NOSTD_STRING_ASSERT(count <= max_size(), "nostd::basic_string::assign(): resulted string size would exceed max_size()", std::length_error);
            internal_assign(str, count);
            return *this;
        }

        constexpr basic_string &assign(const value_type *str)
        {
            return assign(str, Traits::length(str));
        }

        template<typename InputIterator>
        constexpr basic_string &assign(InputIterator first, InputIterator last)
        {
            const auto len = std::distance(first, last);
            _NOSTD_STRING_ASSERT(len <= max_size(), "nostd::basic_string::assign(): resulted string size would exceed max_size()", std::length_error);
            internal_assign(const_pointer { first }, len);
            return *this;
        }

        constexpr basic_string &assign(std::initializer_list<value_type> ilist)
        {
            const auto len = ilist.size();
            _NOSTD_STRING_ASSERT(len <= max_size(), "nostd::basic_string::assign(): resulted string size would exceed max_size()", std::length_error);
            internal_assign(const_pointer { ilist.begin() }, len);
            return *this;
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &assign(const Type &t)
        {
            sview_type sv { t };
            return assign(sv.data(), sv.length());
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &assign(const Type &t, size_type pos, size_type count = npos)
        {
            const auto sv = sview_type { t } .substr(pos, count);
            const auto len = sv.length();
            _NOSTD_STRING_ASSERT(len <= max_size(), "nostd::basic_string::assign(): resulted string size would exceed max_size()", std::length_error);
            return assign(sv.data(), len);
        }

#if NOSTD_STRING_CONTAINERS_RANGES
        template<detail::container_compatible_range<Char> Range>
        constexpr basic_string &assign_range(Range &&range)
        {
            basic_string str { std::from_range, std::forward<Range>(range), _allocator };
            _NOSTD_STRING_ASSERT(str.get_size() <= max_size(), "nostd::basic_string::assign_range(): resulted string size would exceed max_size()", std::length_error);
            return assign();
        }
#endif

        constexpr allocator_type get_allocator() const noexcept
        {
            return _allocator;
        }

        constexpr reference operator[](size_type pos)
        {
            return get_data()[pos];
        }

        constexpr const_reference operator[](size_type pos) const
        {
            return get_data()[pos];
        }

        constexpr reference at(size_type pos)
        {
            _NOSTD_STRING_ASSERT(pos < get_size(), "nostd::basic_string::at(): pos out of range", std::out_of_range);
            return get_data()[pos];
        }

        constexpr const_reference at(size_type pos) const
        {
            _NOSTD_STRING_ASSERT(pos < get_size(), "nostd::basic_string::at(): pos out of range", std::out_of_range);
            return get_data()[pos];
        }

        constexpr reference front()
        {
            return get_data()[0];
        }

        constexpr const_reference front() const
        {
            return get_data()[0];
        }

        constexpr reference back()
        {
            return get_data()[get_size() - 1];
        }

        constexpr const_reference back() const
        {
            return get_data()[get_size() - 1];
        }

        constexpr const value_type *data() const noexcept
        {
            return get_data();
        }

        constexpr value_type *data() noexcept
        {
            return get_data();
        }

        constexpr const value_type *c_str() const noexcept
        {
            return get_data();
        }

        constexpr operator sview_type() const noexcept
        {
            return get_view();
        }

        constexpr iterator begin() noexcept
        {
            return get_data();
        }

        constexpr const_iterator begin() const noexcept
        {
            return get_data();
        }

        constexpr const_iterator cbegin() const noexcept
        {
            return get_data();
        }

        constexpr iterator end() noexcept
        {
            return get_data() + get_size();
        }

        constexpr const_iterator end() const noexcept
        {
            return get_data() + get_size();
        }

        constexpr const_iterator cend() const noexcept
        {
            return get_data() + get_size();
        }

        constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator { end() };
        }

        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator { end() };
        }

        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator { cend() };
        }

        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator { begin() };
        }

        constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator { begin() };
        }

        constexpr const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator { cbegin() };
        }

        constexpr bool empty() const noexcept
        {
            return get_size() == 0;
        }

        constexpr size_type size() const noexcept
        {
            return get_size();
        }

        constexpr size_type length() const noexcept
        {
            return get_size();
        }

        constexpr size_type max_size() const noexcept
        {
            return (alloc_traits::max_size(_allocator) - 1) / 2;
        }

        constexpr void reserve(size_type cap)
        {
            _NOSTD_STRING_ASSERT(cap <= max_size(), "nostd::basic_string::reserve(): allocated memory size would exceed max_size()", std::length_error);
            if (cap <= get_cap())
                return;

            const auto new_cap = std::max(cap, get_size());
            if (new_cap == get_cap())
                return;

            grow_to(new_cap);
        }

        [[deprecated]] void reserve()
        {
            shrink_to_fit();
        }

        constexpr size_type capacity() const noexcept
        {
            return get_cap();
        }

        constexpr void shrink_to_fit()
        {
            if (is_long() == false)
                return;

            reallocate(get_size(), true);
        }

        constexpr void clear() noexcept
        {
            set_size(0);
        }

        constexpr basic_string &insert(size_type pos, size_type count, value_type ch)
        {
            _NOSTD_STRING_ASSERT(get_size() + count <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::insert(): pos out of range", std::out_of_range);
            insert(std::next(cbegin(), pos), count, ch);
            return *this;
        }

        constexpr basic_string &insert(size_type pos, const value_type *str)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::insert(): pos out of range", std::out_of_range);
            const auto len = Traits::length(str);
            _NOSTD_STRING_ASSERT(get_size() + len <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            internal_insert(pos, str, len);
            return *this;
        }

        constexpr basic_string &insert(size_type pos, const value_type *str, size_type count)
        {
            _NOSTD_STRING_ASSERT(get_size() + count <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::insert(): pos out of range", std::out_of_range);
            internal_insert(pos, str, count);
            return *this;
        }

        constexpr basic_string &insert(size_type pos, const basic_string &str)
        {
            _NOSTD_STRING_ASSERT(get_size() + str.get_size() <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::insert(): pos out of range", std::out_of_range);
            internal_insert(pos, const_pointer { str.get_data() }, str.get_size());
            return *this;
        }

        constexpr basic_string &insert(size_type pos, const basic_string &str, size_type pos_str, size_type count = npos)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size() && pos_str <= str.get_size(), "nostd::basic_string::insert(): pos or pos_str out of range", std::out_of_range);
            count = std::min(count, str.length() - pos_str);
            _NOSTD_STRING_ASSERT(get_size() + count <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            return insert(pos, str.data() + pos_str, count);
        }

        constexpr iterator insert(const_iterator pos, value_type ch)
        {
            return insert(pos, 1, ch);
        }

        constexpr iterator insert(const_iterator pos, size_type count, value_type ch)
        {
            _NOSTD_STRING_ASSERT(get_size() + count <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            const auto spos = std::distance(cbegin(), pos);
            internal_insert(spos, ch, count);
            return std::next(begin(), spos);
        }

        template<typename InputIterator>
        constexpr iterator insert(const_iterator pos, InputIterator first, InputIterator last)
        {
            const auto spos = std::distance(cbegin(), pos);
            const auto len = std::distance(first, last);
            _NOSTD_STRING_ASSERT(get_size() + len <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            internal_insert(spos, const_pointer { first }, len);
            return std::next(begin(), spos);
        }

        constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
        {
            _NOSTD_STRING_ASSERT(get_size() + ilist.size() <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            const auto spos = std::distance(cbegin(), pos);
            internal_insert(spos, const_pointer { ilist.begin() }, ilist.size());
            return std::next(begin(), spos);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &insert(size_type pos, const Type &t)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::insert(): pos out of range", std::out_of_range);
            sview_type sv { t };
            _NOSTD_STRING_ASSERT(get_size() + sv.length() <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            internal_insert(pos, const_pointer { sv.data() }, sv.length());
            return *this;
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &insert(size_type pos, const Type &t, size_type pos_str, size_type count = npos)
        {
            sview_type sv { t };
            _NOSTD_STRING_ASSERT(pos <= get_size() && pos_str <= sv.length(), "nostd::basic_string::insert(): pos or pos_str out of range", std::out_of_range);
            sview_type ssv = sv.substr(pos_str, count);
            _NOSTD_STRING_ASSERT(get_size() + ssv.length() <= max_size(), "nostd::basic_string::insert(): resulted string size would exceed max_size()", std::length_error);
            internal_insert(pos, const_pointer { ssv.data() }, ssv.length());
            return *this;
        }

#if NOSTD_STRING_CONTAINERS_RANGES
        template<detail::container_compatible_range<Char> Range>
        constexpr iterator insert_range(const_iterator pos, Range &&range)
        {
            basic_string str { std::from_range, std::forward<Range>(range), _allocator };
            _NOSTD_STRING_ASSERT(get_size() + str.get_size() <= max_size(), "nostd::basic_string::insert_range(): resulted string size would exceed max_size()", std::length_error);
            return insert(pos - begin(), str);
        }
#endif

        constexpr basic_string &erase(size_type pos = 0, size_type count = npos)
        {
            const auto sz = get_size();
            auto buffer = get_data();

            _NOSTD_STRING_ASSERT(pos <= sz, "nostd::basic_string::erase(): pos out of range", std::out_of_range);

            count = std::min(count, sz - pos);

            const auto left = sz - (pos + count);
            if (left != 0)
                Traits::move(buffer + pos, buffer + pos + count, left);

            const auto new_size = pos + left;
            set_size(new_size);
            null_terminate();

            return *this;
        }

        constexpr iterator erase(const_iterator position)
        {
            const auto pos = std::distance(cbegin(), position);
            erase(pos, 1);
            return begin() + pos;
        }

        constexpr iterator erase(const_iterator first, const_iterator last)
        {
            const auto pos = std::distance(cbegin(), first);
            const auto len = std::distance(first, last);
            erase(pos, len);
            return begin() + pos;
        }

        constexpr void push_back(value_type ch)
        {
            _NOSTD_STRING_ASSERT(get_size() + 1 <= max_size(), "nostd::basic_string::push_back(): resulted string size would exceed max_size()", std::length_error);
            append(1, ch);
        }

        constexpr void pop_back()
        {
            erase(end() - 1);
        }

        constexpr basic_string &append(size_type count, value_type ch)
        {
            _NOSTD_STRING_ASSERT(get_size() + count <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(ch, count);
            return *this;
        }

        constexpr basic_string &append(const basic_string &str)
        {
            _NOSTD_STRING_ASSERT(get_size() + str.get_size() <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(str.get_data(), str.get_size());
            return *this;
        }

        constexpr basic_string &append(const basic_string &str, size_type pos, size_type count = npos)
        {
            _NOSTD_STRING_ASSERT(pos <= str.get_size(), "nostd::basic_string::append(): pos out of range", std::out_of_range);
            sview_type ssv = sview_type { str } .substr(pos, count);
            _NOSTD_STRING_ASSERT(get_size() + ssv.length() <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(ssv.data(), ssv.length());
            return *this;
        }

        constexpr basic_string &append(const value_type *str, size_type count)
        {
            _NOSTD_STRING_ASSERT(get_size() + count <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(str, count);
            return *this;
        }

        constexpr basic_string &append(const value_type *str)
        {
            const auto len = Traits::length(str);
            _NOSTD_STRING_ASSERT(get_size() + len <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            return append(str, len);
        }

        template<typename InputIterator>
        constexpr basic_string &append(InputIterator first, InputIterator last)
        {
            const auto len = std::distance(first, last);
            _NOSTD_STRING_ASSERT(get_size() + len <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(const_pointer { first }, len);
            return *this;
        }

        constexpr basic_string &append(std::initializer_list<value_type> ilist)
        {
            _NOSTD_STRING_ASSERT(get_size() + ilist.size() <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(const_pointer { ilist.begin() }, ilist.size());
            return *this;
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &append(const Type &t)
        {
            sview_type sv { t };
            _NOSTD_STRING_ASSERT(get_size() + sv.length() <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(sv.data(), sv.size());
            return *this;
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &append(const Type &t, size_type pos, size_type count = npos)
        {
            sview_type sv { t };
            _NOSTD_STRING_ASSERT(pos < sv.length(), "nostd::basic_string::append(): pos out of range", std::out_of_range);
            sview_type ssv = sv.substr(pos, count);
            _NOSTD_STRING_ASSERT(get_size() + ssv.length() <= max_size(), "nostd::basic_string::append(): resulted string size would exceed max_size()", std::length_error);
            internal_append(ssv.data(), ssv.length());
            return *this;
        }

#if NOSTD_STRING_CONTAINERS_RANGES
        template<detail::container_compatible_range<Char> Range>
        constexpr basic_string &append_range(Range &&range)
        {
            basic_string str { std::from_range, std::forward<Range>(range), _allocator };
            _NOSTD_STRING_ASSERT(get_size() + str.get_size() <= max_size(), "nostd::basic_string::insert_range(): resulted string size would exceed max_size()", std::length_error);
            return append(str);
        }
#endif

        constexpr basic_string &operator+=(const basic_string &str)
        {
            return append(str);
        }

        constexpr basic_string &operator+=(value_type ch)
        {
            push_back(ch);
            return *this;
        }

        constexpr basic_string &operator+=(const value_type *str)
        {
            return append(str);
        }

        constexpr basic_string &operator+=(std::initializer_list<value_type> ilist)
        {
            return append(ilist);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &operator+=(const Type &t)
        {
            return append(sview_type(t));
        }

        constexpr int compare(const basic_string &str) const noexcept
        {
            return get_view().compare(str.get_view());
        }

        constexpr int compare(size_type pos1, size_type count1, const basic_string &str) const
        {
            return get_view().compare(pos1, count1, str.get_view());
        }

        constexpr int compare(size_type pos1, size_type count1, const basic_string &str, size_type pos2, size_type count2 = npos) const
        {
            return get_view().compare(pos1, count1, str.get_view(), pos2, count2);
        }

        constexpr int compare(const value_type *str) const
        {
            return get_view().compare(str);
        }

        constexpr int compare(size_type pos1, size_type count1, const value_type *str) const
        {
            return get_view().compare(pos1, count1, str);
        }

        constexpr int compare(size_type pos1, size_type count1, const value_type *str, size_type count2) const
        {
            return get_view().compare(pos1, count1, str, count2);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr int compare(const Type &t) const noexcept(noexcept(std::is_nothrow_convertible_v<const Type &, sview_type>))
        {
            return get_view().compare(sview_type(t));
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr int compare(size_type pos1, size_type count1, const Type &t) const
        {
            return get_view().compare(pos1, count1, sview_type(t));
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr int compare(size_type pos1, size_type count1, const Type &t, size_type pos2, size_type count2 = npos) const
        {
            return get_view().compare(pos1, count1, sview_type(t), pos2, count2);
        }

        constexpr bool starts_with(sview_type sv) const noexcept
        {
            return get_view().starts_with(sv);
        }

        constexpr bool starts_with(Char ch) const noexcept
        {
            return get_view().starts_with(ch);
        }

        constexpr bool starts_with(const Char *str) const
        {
            return get_view().starts_with(str);
        }

        constexpr bool ends_with(sview_type sv) const noexcept
        {
            return get_view().ends_with(sv);
        }

        constexpr bool ends_with(Char ch) const noexcept
        {
            return get_view().ends_with(ch);
        }

        constexpr bool ends_with(const Char *str) const
        {
            return get_view().ends_with(str);
        }

        constexpr bool contains(sview_type sv) const noexcept
        {
            return get_view().contains(sv);
        }

        constexpr bool contains(Char ch) const noexcept
        {
            return get_view().contains(ch);
        }

        constexpr bool contains(const Char *str) const
        {
            return get_view().contains(str);
        }

        constexpr basic_string &replace(size_type pos, size_type count, const basic_string &str)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::replace(): pos out of range", std::out_of_range);
            return replace(pos, count, str, 0, str.length());
        }

        constexpr basic_string &replace(const_iterator first, const_iterator last, const basic_string &str)
        {
            const auto pos = std::distance(cbegin(), first);
            const auto count = std::distance(first, last);
            return replace(pos, count, str, 0, str.length());
        }

        constexpr basic_string &replace(size_type pos, size_type count, const basic_string &str, size_type pos2, size_type count2 = npos)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size() && pos2 <= str.get_size(), "nostd::basic_string::replace(): pos or pos_str out of range", std::out_of_range);
            count2 = std::min(count2, str.length() - pos2);
            sview_type ssv = sview_type { str } .substr(pos2, count2);
            return replace(pos, count, ssv.data(), ssv.length());
        }

        template<typename InputIterator>
        constexpr basic_string &replace(const_iterator first, const_iterator last, InputIterator first2, InputIterator last2)
        {
            return replace(first, last, const_pointer { first2 }, std::distance(first2, last2));
        }

        constexpr basic_string &replace(size_type pos, size_type count, const value_type *str, size_type count2)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::replace(): pos out of range", std::out_of_range);
            count = std::min(count, length() - pos);
            _NOSTD_STRING_ASSERT(get_size() - count + count2 <= max_size(), "nostd::basic_string::replace(): resulted string size would exceed max_size()", std::length_error);
            internal_replace(pos, const_pointer { str }, count, count2);
            return *this;
        }

        constexpr basic_string &replace(const_iterator first, const_iterator last, const value_type *str, size_type count2)
        {
            size_type pos = std::distance(cbegin(), first);
            size_type count = std::distance(first, last);

            return replace(pos, count, str, count2);
        }

        constexpr basic_string &replace(size_type pos, size_type count, const value_type *str)
        {
            return replace(pos, count, str, Traits::length(str));
        }

        constexpr basic_string &replace(const_iterator first, const_iterator last, const value_type *str)
        {
            return replace(first, last, str, Traits::length(str));
        }

        constexpr basic_string &replace(size_type pos, size_type count, size_type count2, value_type ch)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::replace(): pos out of range", std::out_of_range);
            count = std::min(count, length() - pos);
            _NOSTD_STRING_ASSERT(get_size() - count + count2 <= max_size(), "nostd::basic_string::replace(): resulted string size would exceed max_size()", std::length_error);
            internal_replace(pos, ch, count, count2);
            return *this;
        }

        constexpr basic_string &replace(const_iterator first, const_iterator last, size_type count2, value_type ch)
        {
            const auto pos = std::distance(cbegin(), first);
            const auto count = std::distance(first, last);

            _NOSTD_STRING_ASSERT(get_size() - count + count2 <= max_size(), "nostd::basic_string::replace(): resulted string size would exceed max_size()", std::length_error);
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::replace(): pos out of range", std::out_of_range);
            internal_replace(pos, ch, count, count2);
            return *this;
        }

        constexpr basic_string &replace(const_iterator first, const_iterator last, std::initializer_list<value_type> ilist)
        {
            return replace(first, last, const_pointer { ilist.begin() }, ilist.size());
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &replace(size_type pos, size_type count, const Type &t)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::replace(): pos out of range", std::out_of_range);
            sview_type sv(t);
            return replace(pos, count, sv.data(), sv.length());
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &replace(const_iterator first, const_iterator last, const Type &t)
        {
            sview_type sv(t);
            return replace(first, last, sv.data(), sv.length());
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr basic_string &replace(size_type pos, size_type count, const Type &t, size_type pos2, size_type count2 = npos)
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::replace(): pos out of range", std::out_of_range);
            auto sv = sview_type(t).substr(pos2, count2);
            return replace(pos, count, sv.data(), sv.length());
        }

#if NOSTD_STRING_CONTAINERS_RANGES
        template<detail::container_compatible_range<Char> Range>
        constexpr iterator replace_with_range(const_iterator first, const_iterator last, Range &&range)
        {
            basic_string str { std::from_range, std::forward<Range>(range), _allocator };
            return replace(first, last, str); // replace checks for max_size()
        }
#endif

        constexpr basic_string substr(size_type pos = 0, size_type count = npos) const
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::substr(): pos out of range", std::out_of_range);
            return basic_string(*this, pos, count);
        }

        constexpr size_type copy(value_type *str, size_type count, size_type pos = 0) const
        {
            _NOSTD_STRING_ASSERT(pos <= get_size(), "nostd::basic_string::copy(): pos out of range", std::out_of_range);
            return get_view().copy(str, count, pos);
        }

        constexpr void resize(size_type count, value_type ch)
        {
            _NOSTD_STRING_ASSERT(get_size() + count <= max_size(), "nostd::basic_string::resize(): resulted string size would exceed max_size()", std::length_error);
            const auto cap = get_cap();
            const auto sz = get_size();
            const auto rsz = count + sz;

            if (sz < rsz)
            {
                if (cap < rsz)
                    grow_to(rsz);
                Traits::assign(get_data() + sz, count, ch);
            }
            set_size(rsz);
            null_terminate();
        }

        constexpr void resize(size_type count)
        {
            resize(count, _terminator);
        }

        template<typename Operation>
        constexpr void resize_and_overwrite(size_type, Operation)
        {
            static_assert(detail::dependent_false<Char>, "nostd::basic_string::resize_and_overwrite(count, op) not implemented!");
        }

        constexpr void swap(basic_string &str) noexcept(alloc_traits::propagate_on_container_swap::value || alloc_traits::is_always_equal::value)
        {
            using std::swap;
            swap(storage, str.storage);
            swap(_allocator, str._allocator);
        }

        constexpr size_type find(const basic_string &str, size_type pos = 0) const noexcept
        {
            return get_view().find(sview_type(str), pos);
        }

        constexpr size_type find(const value_type *str, size_type pos, size_type count) const noexcept
        {
            return get_view().find(str, pos, count);
        }

        constexpr size_type find(const value_type *str, size_type pos = 0) const noexcept
        {
            return get_view().find(str, pos);
        }

        constexpr size_type find(value_type ch, size_type pos = 0) const noexcept
        {
            return get_view().find(ch, pos);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr size_type find(const Type &t, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const Type &, sview_type>)
        {
            return get_view().find(sview_type(t), pos);
        }

        constexpr size_type rfind(const basic_string &str, size_type pos = npos) const noexcept
        {
            return get_view().rfind(sview_type(str), pos);
        }

        constexpr size_type rfind(const value_type *str, size_type pos, size_type count) const noexcept
        {
            return get_view().rfind(str, pos, count);
        }

        constexpr size_type rfind(const value_type *str, size_type pos = npos) const noexcept
        {
            return get_view().rfind(str, pos);
        }

        constexpr size_type rfind(value_type ch, size_type pos = npos) const noexcept
        {
            return get_view().rfind(ch, pos);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr size_type rfind(const Type &t, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const Type &, sview_type>)
        {
            return get_view().rfind(sview_type(t), pos);
        }

        constexpr size_type find_first_of(const basic_string &str, size_type pos = 0) const noexcept
        {
            return get_view().find_first_of(sview_type(str), pos);
        }

        constexpr size_type find_first_of(const value_type *str, size_type pos, size_type count) const noexcept
        {
            return get_view().find_first_of(str, pos, count);
        }

        constexpr size_type find_first_of(const value_type *str, size_type pos = 0) const noexcept
        {
            return get_view().find_first_of(str, pos);
        }

        constexpr size_type find_first_of(value_type ch, size_type pos = 0) const noexcept
        {
            return get_view().find_first_of(ch, pos);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr size_type find_first_of(const Type &t, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const Type &, sview_type>)
        {
            return get_view().find_first_of(sview_type(t), pos);
        }

        constexpr size_type find_first_not_of(const basic_string &str, size_type pos = 0) const noexcept
        {
            return get_view().find_last_not_of(sview_type(str), pos);
        }

        constexpr size_type find_first_not_of(const value_type *str, size_type pos, size_type count) const noexcept
        {
            return get_view().find_last_not_of(str, pos, count);
        }

        constexpr size_type find_first_not_of(const value_type *str, size_type pos = 0) const noexcept
        {
            return get_view().find_last_not_of(str, pos);
        }

        constexpr size_type find_first_not_of(value_type ch, size_type pos = 0) const noexcept
        {
            return get_view().find_first_not_of(ch, pos);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr size_type find_first_not_of(const Type &t, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const Type &, sview_type>)
        {
            return get_view().find_first_not_of(sview_type(t), pos);
        }

        constexpr size_type find_last_of(const basic_string &str, size_type pos = npos) const noexcept
        {
            return get_view().find_last_of(sview_type(str), pos);
        }

        constexpr size_type find_last_of(const value_type *str, size_type pos, size_type count) const noexcept
        {
            return get_view().find_last_of(str, pos, count);
        }

        constexpr size_type find_last_of(const value_type *str, size_type pos = npos) const noexcept
        {
            return get_view().find_last_of(str, pos);
        }

        constexpr size_type find_last_of(value_type ch, size_type pos = npos) const noexcept
        {
            return get_view().find_last_of(ch, pos);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr size_type find_last_of(const Type &t, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const Type &, sview_type>)
        {
            return get_view().find_last_of(sview_type(t), pos);
        }

        constexpr size_type find_last_not_of(const basic_string &str, size_type pos = npos) const noexcept
        {
            return get_view().find_last_not_of(sview_type(str), pos);
        }

        constexpr size_type find_last_not_of(const value_type *str, size_type pos, size_type count) const noexcept
        {
            return get_view().find_last_not_of(str, pos, count);
        }

        constexpr size_type find_last_not_of(const value_type *str, size_type pos = npos) const noexcept
        {
            return get_view().find_last_not_of(str, pos);
        }

        constexpr size_type find_last_not_of(value_type ch, size_type pos = npos) const noexcept
        {
            return get_view().find_last_not_of(ch, pos);
        }

        template<typename Type> requires (
            std::is_convertible_v<const Type &, sview_type> &&
            !std::is_convertible_v<const Type &, const Char *>
        )
        constexpr size_type find_last_not_of(const Type &t, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const Type &, sview_type>)
        {
            return get_view().find_last_not_of(sview_type(t), pos);
        }

        friend constexpr basic_string operator+(const basic_string &lhs, const basic_string &rhs)
        {
            const auto lhs_sz = lhs.size();
            const auto rhs_sz = rhs.size();
            basic_string ret(detail::uninitialized_size_tag(), lhs_sz + rhs_sz, basic_string::alloc_traits::select_on_container_copy_construction(lhs.get_allocator()));
            auto buffer = ret.get_data();
            Traits::copy(buffer, lhs.data(), lhs_sz);
            Traits::copy(buffer + lhs_sz, rhs.data(), rhs_sz);
            ret.null_terminate();
            return ret;
        }

        friend constexpr basic_string operator+(basic_string &&lhs, const basic_string &rhs)
        {
            return std::move(lhs.append(rhs));
        }

        friend constexpr basic_string operator+(const basic_string &lhs, basic_string &&rhs)
        {
            return std::move(rhs.insert(0, lhs));
        }

        friend constexpr basic_string operator+(basic_string &&lhs, basic_string &&rhs)
        {
            return std::move(lhs.append(rhs));
        }

        friend constexpr basic_string operator+(const Char *lhs, const basic_string &rhs)
        {
            const auto lhs_sz = Traits::length(lhs);
            const auto rhs_sz = rhs.size();
            basic_string ret(detail::uninitialized_size_tag(), lhs_sz + rhs_sz, basic_string::alloc_traits::select_on_container_copy_construction(rhs.get_allocator()));
            auto buffer = ret.get_data();
            Traits::copy(buffer, lhs, lhs_sz);
            Traits::copy(buffer + lhs_sz, rhs.data(), rhs_sz);
            ret.null_terminate();
            return ret;
        }

        friend constexpr basic_string operator+(const Char *lhs, basic_string &&rhs)
        {
            return std::move(rhs.insert(0, lhs));
        }

        friend constexpr basic_string operator+(Char lhs, const basic_string &rhs)
        {
            const auto rhs_sz = rhs.size();
            basic_string ret(detail::uninitialized_size_tag(), rhs_sz + 1, basic_string::alloc_traits::select_on_container_copy_construction(rhs.get_allocator()));
            auto buffer = ret.get_data();
            Traits::assign(buffer, 1, lhs);
            Traits::copy(buffer + 1, rhs.data(), rhs_sz);
            ret.null_terminate();
            return ret;
        }

        friend constexpr basic_string operator+(Char lhs, basic_string &&rhs)
        {
            rhs.insert(rhs.begin(), lhs);
            return std::move(rhs);
        }

        friend constexpr basic_string operator+(const basic_string &lhs, const Char *rhs)
        {
            const auto lhs_sz = lhs.size();
            const auto rhs_sz = Traits::length(rhs);
            basic_string ret(detail::uninitialized_size_tag(), lhs_sz + rhs_sz, basic_string::alloc_traits::select_on_container_copy_construction(lhs.get_allocator()));
            auto buffer = ret.get_data();
            Traits::copy(buffer, lhs.data(), lhs_sz);
            Traits::copy(buffer + lhs_sz, rhs, rhs_sz);
            ret.null_terminate();
            return ret;
        }

        friend constexpr basic_string operator+(basic_string &&lhs, const Char *rhs)
        {
            return std::move(lhs.append(rhs));
        }

        friend constexpr basic_string operator+(const basic_string &lhs, Char rhs)
        {
            const auto lhs_sz = lhs.size();
            basic_string ret(detail::uninitialized_size_tag(), lhs_sz + 1, basic_string::alloc_traits::select_on_container_copy_construction(lhs.get_allocator()));
            auto buffer = ret.get_data();
            Traits::copy(buffer, lhs.data(), lhs_sz);
            Traits::assign(buffer + lhs_sz, 1, rhs);
            ret.null_terminate();
            return ret;
        }

        friend constexpr basic_string operator+(basic_string &&lhs, Char rhs)
        {
            lhs.push_back(rhs);
            return std::move(lhs);
        }
    };

    template<typename Char, typename Traits, typename Allocator>
    constexpr bool operator==(const basic_string<Char, Traits, Allocator> &lhs, const basic_string<Char, Traits, Allocator> &rhs) noexcept
    {
        return lhs.compare(rhs) == 0;
    }

    template<typename Char, typename Traits, typename Allocator>
    constexpr bool operator==(const basic_string<Char, Traits, Allocator> &lhs, const Char *rhs)
    {
        return lhs.compare(rhs) == 0;
    }

    template<typename Char, typename Traits, typename Allocator>
    constexpr std::strong_ordering operator<=>(const basic_string<Char, Traits, Allocator> &lhs, const basic_string<Char, Traits, Allocator> &rhs) noexcept
    {
        return lhs.compare(rhs) <=> 0;
    }

    template<typename Char, typename Traits, typename Allocator>
    constexpr std::strong_ordering operator<=>(const basic_string<Char, Traits, Allocator> &lhs, const Char *rhs)
    {
        return lhs.compare(rhs) <=> 0;
    }

    // swap
    template<typename Char, typename Traits, typename Allocator>
    constexpr void swap(basic_string<Char, Traits, Allocator> &lhs, basic_string<Char, Traits, Allocator> &rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    // erasure
    template<typename Char, typename Traits, typename Allocator, typename U>
    constexpr typename basic_string<Char, Traits, Allocator>::size_type erase(basic_string<Char, Traits, Allocator> &c, const U &value)
    {
        auto it = std::remove(c.begin(), c.end(), value);
        const auto r = std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }

    template<typename Char, typename Traits, typename Allocator, typename Pred>
    constexpr typename basic_string<Char, Traits, Allocator>::size_type erase_if(basic_string<Char, Traits, Allocator> &c, Pred pred)
    {
        auto it = std::remove_if(c.begin(), c.end(), pred);
        const auto r = std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }

    // deduction guides
    template<typename InputIt, typename Allocator = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
    basic_string(InputIt, InputIt, Allocator = Allocator()) -> basic_string<typename std::iterator_traits<InputIt>::value_type, std::char_traits<typename std::iterator_traits<InputIt>::value_type>, Allocator>;

    template<typename Char, typename Traits, typename Allocator = std::allocator<Char>>
    explicit basic_string(std::basic_string_view<Char, Traits>, const Allocator & = Allocator()) -> basic_string<Char, Traits, Allocator>;

    template<typename Char, typename Traits, typename Allocator = std::allocator<Char>>
    basic_string(std::basic_string_view<Char, Traits>, typename basic_string<Char, Traits, Allocator>::size_type, typename basic_string<Char, Traits, Allocator>::size_type, const Allocator & = Allocator()) -> basic_string<Char, Traits, Allocator>;

#if NOSTD_STRING_CONTAINERS_RANGES
    template<std::ranges::input_range Range, typename Allocator = std::allocator<std::ranges::range_value_t<Range>>>
    basic_string(std::from_range_t, Range&&, Allocator = Allocator()) -> basic_string<std::ranges::range_value_t<Range>, std::char_traits<std::ranges::range_value_t<Range>>, Allocator>;
#endif

    // basic_string typedef-names
    using string = basic_string<char>;
    using u8string = basic_string<char8_t>;
    using u16string = basic_string<char16_t>;
    using u32string = basic_string<char32_t>;
    using wstring = basic_string<wchar_t>;

#if NOSTD_STRING_NUMERIC_CONVERSIONS
    // numeric conversions
    inline int stoi(const string &str, std::size_t *pos = nullptr, int base = 10)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtol(cstr, &ptr, base);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }

    inline long stol(const string &str, std::size_t *pos = nullptr, int base = 10)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtol(cstr, &ptr, base);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }

    inline long long stoll(const string &str, std::size_t *pos = nullptr, int base = 10)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtoll(cstr, &ptr, base);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }

    inline unsigned long stoul(const string &str, std::size_t *pos = nullptr, int base = 10)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtoul(cstr, &ptr, base);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }

    inline unsigned long long stoull(const string &str, std::size_t *pos = nullptr, int base = 10)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtoull(cstr, &ptr, base);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }

#  if NOSTD_STRING_FLOAT
    inline float stof(const string &str, std::size_t *pos = nullptr)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtof(cstr, &ptr);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }

    inline double stod(const string &str, std::size_t *pos = nullptr)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtod(cstr, &ptr);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }

#    if NOSTD_STRING_LONG_DOUBLE
    inline long double stold(const string &str, std::size_t *pos = nullptr)
    {
        auto cstr = str.c_str();
        char *ptr = const_cast<char *>(cstr);

        auto ret = strtold(cstr, &ptr);
        if (pos != nullptr)
            *pos = cstr - ptr;

        return ret;
    }
#    endif
#  endif

    namespace detail
    {
        template<typename Type>
        constexpr std::size_t to_chars_len(Type value)
        {
            constexpr Type b1 = 10;
            constexpr Type b2 = 100;
            constexpr Type b3 = 1000;
            constexpr Type b4 = 10000;

            for (std::size_t i = 1; ; i += 4, value /= b4)
            {
                if (value < b1)
                    return i;
                if (value < b2)
                    return i + 1;
                if (value < b3)
                    return i + 2;
                if (value < b4)
                    return i + 3;
            }
        }

        static constexpr char digits[201] =
                "0001020304050607080910111213141516171819"
                "2021222324252627282930313233343536373839"
                "4041424344454647484950515253545556575859"
                "6061626364656667686970717273747576777879"
                "8081828384858687888990919293949596979899";

        constexpr void to_chars(char *first, std::size_t len, auto val)
        {
            std::size_t pos = len - 1;
            while (val >= 100)
            {
                const auto num = (val % 100) * 2;
                val /= 100;
                first[pos] = digits[num + 1];
                first[pos - 1] = digits[num];
                pos -= 2;
            }
            if (val >= 10)
            {
                const auto num = val * 2;
                first[1] = digits[num + 1];
                first[0] = digits[num];
            }
            else first[0] = '0' + val;
        }

        template<std::signed_integral Type, std::unsigned_integral UType = std::make_unsigned_t<Type>>
        [[gnu::always_inline]]
        constexpr inline string to_string(Type value)
        {
            const auto negative = value < 0;
            const UType uvalue = negative ? static_cast<UType>(~value) + static_cast<UType>(1) : value;
            const auto length = to_chars_len(uvalue);
            string str(length + negative, '-');
            to_chars(&str[negative], length, uvalue);
            return str;
        }

        template<std::unsigned_integral Type>
        [[gnu::always_inline]]
        constexpr inline string to_string(Type value)
        {
            string str(to_chars_len(value), '\0');
            to_chars(&str[0], str.length(), value);
            return str;
        }
    } // namespace detail

    constexpr inline string to_string(int val) { return detail::to_string(val); }
    constexpr inline string to_string(unsigned val) { return detail::to_string(val); }
    constexpr inline string to_string(long val) { return detail::to_string(val); }
    constexpr inline string to_string(unsigned long val) { return detail::to_string(val); }
    constexpr inline string to_string(long long val) { return detail::to_string(val); }
    constexpr inline string to_string(unsigned long long val) { return detail::to_string(val); }

#  if NOSTD_STRING_FLOAT
    // constexpr inline string to_string(float val);
    // constexpr inline string to_string(double val);

#    if NOSTD_STRING_LONG_DOUBLE
    // constexpr inline string to_string(long double val);
#    endif
#  endif

    // constexpr inline wstring to_wstring(int val);
    // constexpr inline wstring to_wstring(unsigned val);
    // constexpr inline wstring to_wstring(long val);
    // constexpr inline wstring to_wstring(unsigned long val);
    // constexpr inline wstring to_wstring(long long val);
    // constexpr inline wstring to_wstring(unsigned long long val);

#  if NOSTD_STRING_FLOAT
    // constexpr inline wstring to_wstring(float val);
    // constexpr inline wstring to_wstring(double val);

#    if NOSTD_STRING_LONG_DOUBLE
    // constexpr inline wstring to_wstring(long double val);
#    endif
#  endif
#endif

#if NOSTD_STRING_STD_HASH
    // hash support
    namespace detail
    {
        inline uint64_t MurmurHash2_64A(const void *key, uint64_t len, uint64_t seed)
        {
            const uint64_t m = 0xC6A4A7935BD1E995;
            const int r = 47;

            uint64_t h = seed ^ (len * m);

            const uint64_t *data = static_cast<const uint64_t*>(key);
            const uint64_t *end = data + (len / 8);

            while (data != end)
            {
                uint64_t k = 0;
                k = *(data++);

                k *= m;
                k ^= k >> r;
                k *= m;

                h ^= k;
                h *= m;
            }

            auto data2 = static_cast<const uint8_t*>(static_cast<const void*>(data));

            switch (len & 7)
            {
                case 7: h ^= static_cast<uint64_t>(data2[6]) << 48; [[fallthrough]];
                case 6: h ^= static_cast<uint64_t>(data2[5]) << 40; [[fallthrough]];
                case 5: h ^= static_cast<uint64_t>(data2[4]) << 32; [[fallthrough]];
                case 4: h ^= static_cast<uint64_t>(data2[3]) << 24; [[fallthrough]];
                case 3: h ^= static_cast<uint64_t>(data2[2]) << 16; [[fallthrough]];
                case 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  [[fallthrough]];
                case 1: h ^= static_cast<uint64_t>(data2[0]);
                    h *= m;
            };

            h ^= h >> r;
            h *= m;
            h ^= h >> r;

            return h;
        }

        template<typename Char, typename Allocator, typename String = basic_string<Char, std::char_traits<Char>, Allocator>>
        struct string_hash_base
        {
            [[nodiscard]] std::size_t operator()(const String &str) const noexcept
            {
                return MurmurHash2_64A(str.c_str(), str.length() * sizeof(Char), 0xC70F6907UL);
            }
        };
    } // namespace detail
#endif

    inline namespace literals
    {
        inline namespace string_literals
        {
_NOSTD_STRING_DIAG_PUSH()

#if defined(__clang__)
  _NOSTD_STRING_DIAG_IGN("-Wuser-defined-literals")
#elif defined(__GNUC__)
  _NOSTD_STRING_DIAG_IGN("-Wliteral-suffix")
#endif
            // suffix for basic_string literals
            constexpr inline string operator""s(const char *str, std::size_t len) { return string { str, len }; }
            constexpr inline u8string operator""s(const char8_t *str, std::size_t len) { return u8string { str, len }; }
            constexpr inline u16string operator""s(const char16_t *str, std::size_t len) { return u16string { str, len }; }
            constexpr inline u32string operator""s(const char32_t *str, std::size_t len) { return u32string { str, len }; }
            constexpr inline wstring operator""s(const wchar_t *str, std::size_t len) { return wstring { str, len }; }

_NOSTD_STRING_DIAG_POP()
        } // namespace string_literals
    } // namespace literals
} // namespace nostd

#if NOSTD_STRING_STD_HASH
// hash support
namespace std
{
    template<typename Allocator>
    struct hash<nostd::basic_string<char, std::char_traits<char>, Allocator>> : nostd::detail::string_hash_base<char, Allocator> { };

    template<typename Allocator>
    struct hash<nostd::basic_string<char8_t, std::char_traits<char8_t>, Allocator>> : nostd::detail::string_hash_base<char8_t, Allocator> { };

    template<typename Allocator>
    struct hash<nostd::basic_string<char16_t, std::char_traits<char16_t>, Allocator>> : nostd::detail::string_hash_base<char16_t, Allocator> { };

    template<typename Allocator>
    struct hash<nostd::basic_string<char32_t, std::char_traits<char32_t>, Allocator>> : nostd::detail::string_hash_base<char32_t, Allocator> { };

    template<typename Allocator>
    struct hash<nostd::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator>> : nostd::detail::string_hash_base<wchar_t, Allocator> { };
} // namespace std
#endif
