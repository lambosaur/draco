/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_ITERATOR_COUNTED_ITERATOR_HPP
#define RANGES_V3_ITERATOR_COUNTED_ITERATOR_HPP

#include <utility>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/iterator/concepts.hpp>
#include <range/v3/iterator/default_sentinel.hpp>
#include <range/v3/iterator/operations.hpp>
#include <range/v3/iterator/traits.hpp>

namespace ranges
{
    /// \cond
    namespace _counted_iterator_
    {
        struct access
        {
            template<typename I>
            static constexpr /*c++14*/ iter_difference_t<counted_iterator<I>> &
            count(counted_iterator<I> &ci) noexcept
            {
                return ci.cnt_;
            }

            template<typename I>
            static constexpr /*c++14*/ I &
            current(counted_iterator<I> &ci) noexcept
            {
                return ci.current_;
            }

            template<typename I>
            static constexpr I const &
            current(counted_iterator<I> const &ci) noexcept
            {
                return ci.current_;
            }
        };

        template<bool>
        struct contiguous_iterator_concept_base
        {};

        template<>
        struct contiguous_iterator_concept_base<true>
        {
            using iterator_concept = ranges::contiguous_iterator_tag;
        };
    }
    /// \endcond

    CPP_template_def(typename I)(
        requires Iterator<I>)
    struct counted_iterator
      : _counted_iterator_::contiguous_iterator_concept_base<(bool) ContiguousIterator<I>>
    {
    private:
        friend advance_fn;
        CPP_assert(Iterator<I>);
        friend _counted_iterator_::access;

        I current_{};
        iter_difference_t<I> cnt_{0};

        void post_increment_(std::true_type)
        {
            CPP_assert(std::is_void<decltype(current_++)>());
            ++current_;
        }
        auto post_increment_(std::false_type) -> decltype(current_++)
        {
            CPP_assert(!std::is_void<decltype(current_++)>());
            auto&& tmp = current_++;
            --cnt_;
            return static_cast<decltype(tmp) &&>(tmp);
        }
    public:
        using iterator_type = I;
        using difference_type = iter_difference_t<I>;

        counted_iterator() = default;

        counted_iterator(I x, iter_difference_t<I> n)
          : current_(std::move(x)), cnt_(n)
        {
            RANGES_EXPECT(n >= 0);
        }

        template<typename I2>
        CPP_ctor(counted_iterator)(counted_iterator<I2> const &i)(
            requires ConvertibleTo<I2, I>)
          : current_(_counted_iterator_::access::current(i)), cnt_(i.count())
        {}

        template<typename I2>
        auto operator=(counted_iterator<I2> const &i) ->
            CPP_ret(counted_iterator &)(
                requires ConvertibleTo<I2, I>)
        {
            current_ = _counted_iterator_::access::current(i);
            cnt_ = i.count();
        }

        I base() const
        {
            return current_;
        }

        iter_difference_t<I> count() const
        {
            return cnt_;
        }

        iter_reference_t<I> operator*()
            noexcept(noexcept(iter_reference_t<I>(*current_)))
        {
            RANGES_EXPECT(cnt_ > 0);
            return *current_;
        }
        template<typename I2 = I>
        auto operator*() const
            noexcept(noexcept(iter_reference_t<I>(*current_))) ->
            CPP_ret(iter_reference_t<I2>)(
                requires Readable<I2 const>)
        {
            RANGES_EXPECT(cnt_ > 0);
            return *current_;
        }

        counted_iterator& operator++()
        {
            RANGES_EXPECT(cnt_ > 0);
            ++current_;
            --cnt_;
            return *this;
        }

#ifdef RANGES_WORKAROUND_MSVC_677925
        template<typename I2 = I>
        auto operator++(int) ->
            CPP_ret(decltype(std::declval<I2 &>()++))(
                requires not ForwardIterator<I2>)
#else // ^^^ workaround ^^^ / vvv no workaround vvv
        CPP_member
        auto operator++(int) ->
            CPP_ret(decltype(std::declval<I &>()++))(
                requires not ForwardIterator<I>)
#endif // RANGES_WORKAROUND_MSVC_677925
        {
            RANGES_EXPECT(cnt_ > 0);
            return post_increment_(std::is_void<decltype(current_++)>());
        }

        CPP_member
        auto operator++(int) ->
            CPP_ret(counted_iterator)(
                requires ForwardIterator<I>)
        {
            auto tmp(*this);
            ++*this;
            return tmp;
        }

        CPP_member
        auto operator--() ->
            CPP_ret(counted_iterator &)(
                requires BidirectionalIterator<I>)
        {
            --current_;
            ++cnt_;
            return *this;
        }

        CPP_member
        auto operator--(int) ->
            CPP_ret(counted_iterator)(
                requires BidirectionalIterator<I>)
        {
            auto tmp(*this);
            --*this;
            return tmp;
        }

        CPP_member
        auto operator+=(difference_type n) ->
            CPP_ret(counted_iterator &)(
                requires RandomAccessIterator<I>)
        {
            RANGES_EXPECT(cnt_ >= n);
            current_ += n;
            cnt_ -= n;
            return *this;
        }

        CPP_member
        auto operator+(difference_type n) const ->
            CPP_ret(counted_iterator)(
                requires RandomAccessIterator<I>)
        {
            auto tmp(*this);
            tmp += n;
            return tmp;
        }

        CPP_member
        auto operator-=(difference_type n) ->
            CPP_ret(counted_iterator &)(
                requires RandomAccessIterator<I>)
        {
            RANGES_EXPECT(cnt_ >= -n);
            current_ -= n;
            cnt_ += n;
            return *this;
        }

        CPP_member
        auto operator-(difference_type n) const ->
            CPP_ret(counted_iterator)(
                requires RandomAccessIterator<I>)
        {
            auto tmp(*this);
            tmp -= n;
            return tmp;
        }

        CPP_member
        auto operator[](difference_type n) const ->
            CPP_ret(iter_reference_t<I>)(
                requires RandomAccessIterator<I>)
        {
            RANGES_EXPECT(cnt_ >= n);
            return current_[n];
        }

#if !RANGES_BROKEN_CPO_LOOKUP
        CPP_broken_friend_member
        friend constexpr /*c++14*/
        auto iter_move(counted_iterator const &i)
            noexcept(detail::has_nothrow_iter_move_v<I>) ->
            CPP_broken_friend_ret(iter_rvalue_reference_t<I>)(
                requires InputIterator<I>)
        {
            return ranges::iter_move(i.current_);
        }
        template<typename I2, typename S2>
        friend auto iter_swap(counted_iterator const &x,
                              counted_iterator<I2> const &y)
            noexcept(is_nothrow_indirectly_swappable<I, I2>::value) ->
            CPP_broken_friend_ret(void)(
                requires IndirectlySwappable<I2, I>)
        {
            return ranges::iter_swap(
                x.current_,
                _counted_iterator_::access::current(y));
        }
#endif
    };

#if RANGES_BROKEN_CPO_LOOKUP
    namespace _counted_iterator_
    {
        template<typename I>
        constexpr /*c++14*/
        auto iter_move(counted_iterator<I> const &i)
            noexcept(detail::has_nothrow_iter_move_v<I>) ->
            CPP_broken_friend_ret(iter_rvalue_reference_t<I>)(
                requires InputIterator<I>)
        {
            return ranges::iter_move(_counted_iterator_::access::current(i));
        }
        template<typename I1, typename I2>
        auto iter_swap(counted_iterator<I1> const &x,
                       counted_iterator<I2> const &y)
            noexcept(is_nothrow_indirectly_swappable<I1, I2>::value) ->
            CPP_broken_friend_ret(void)(
                requires IndirectlySwappable<I2, I1>)
        {
            return ranges::iter_swap(
                _counted_iterator_::access::current(x),
                _counted_iterator_::access::current(y));
        }
    }
#endif

    template<typename I1, typename I2>
    auto operator==(counted_iterator<I1> const &x, counted_iterator<I2> const &y) ->
        CPP_ret(bool)(
            requires Common<I1, I2>)
    {
        return x.count() == y.count();
    }

    template<typename I>
    bool operator==(counted_iterator<I> const &x, default_sentinel_t)
    {
        return x.count() == 0;
    }

    template<typename I>
    bool operator==(default_sentinel_t, counted_iterator<I> const &x)
    {
        return x.count() == 0;
    }

    template<typename I1, typename I2>
    auto operator!=(counted_iterator<I1> const &x, counted_iterator<I2> const &y) ->
        CPP_ret(bool)(
            requires Common<I1, I2>)
    {
        return !(x == y);
    }

    template<typename I>
    bool operator!=(counted_iterator<I> const &x, default_sentinel_t y)
    {
        return !(x == y);
    }

    template<typename I>
    bool operator!=(default_sentinel_t x, counted_iterator<I> const &y)
    {
        return !(x == y);
    }

    template<typename I1, typename I2>
    auto operator<(counted_iterator<I1> const &x, counted_iterator<I2> const &y) ->
        CPP_ret(bool)(
            requires Common<I1, I2>)
    {
        return y.count() < x.count();
    }

    template<typename I1, typename I2>
    auto operator<=(counted_iterator<I1> const &x, counted_iterator<I2> const &y) ->
        CPP_ret(bool)(
            requires Common<I1, I2>)
    {
        return !(y < x);
    }

    template<typename I1, typename I2>
    auto operator>(counted_iterator<I1> const &x, counted_iterator<I2> const &y) ->
        CPP_ret(bool)(
            requires Common<I1, I2>)
    {
        return y < x;
    }

    template<typename I1, typename I2>
    auto operator>=(counted_iterator<I1> const &x, counted_iterator<I2> const &y) ->
        CPP_ret(bool)(
            requires Common<I1, I2>)
    {
        return !(x < y);
    }

    template<typename I1, typename I2>
    auto operator-(counted_iterator<I1> const &x, counted_iterator<I2> const &y) ->
        CPP_ret(iter_difference_t<I2>)(
            requires Common<I1, I2>)
    {
        return y.count() - x.count();
    }

    template<typename I>
    iter_difference_t<I>
    operator-(counted_iterator<I> const &x, default_sentinel_t)
    {
        return -x.count();
    }

    template<typename I>
    iter_difference_t<I>
    operator-(default_sentinel_t, counted_iterator<I> const &y)
    {
        return y.count();
    }

    template<typename I>
    auto operator+(iter_difference_t<I> n, counted_iterator<I> const &x) ->
        CPP_ret(counted_iterator<I>)(
            requires RandomAccessIterator<I>)
    {
        return x + n;
    }

    template<typename I>
    auto make_counted_iterator(I i, iter_difference_t<I> n) ->
        CPP_ret(counted_iterator<I>)(
            requires Iterator<I>)
    {
        return {std::move(i), n};
    }

    template<typename I>
    struct readable_traits<counted_iterator<I>>
      : detail::if_then_t<
            (bool) Readable<I>,
            readable_traits<I>,
            meta::nil_>
    {};

    template<typename I>
    inline constexpr /*c++14*/
    auto advance_fn::operator()(counted_iterator<I> &i, iter_difference_t<I> n) const ->
        CPP_ret(void)(
            requires Iterator<I>)
    {
        RANGES_EXPECT(n <= i.cnt_);
        advance(i.current_, n);
        i.cnt_ -= n;
    }
}

/// \cond
namespace ranges
{
    namespace _counted_iterator_
    {
        template<typename I, typename = void>
        struct iterator_traits_
        {
            using difference_type = iter_difference_t<I>;
            using value_type = void;
            using reference = void;
            using pointer = void;
            using iterator_category = std::output_iterator_tag;
        };

        template<typename I>
        struct iterator_traits_<I, meta::if_c<InputIterator<I>>>
          : std::iterator_traits<I>
        {
            using pointer = void;
        };
    } // namespace _counted_iterator_
}

namespace std
{
    template<typename I>
    struct iterator_traits< ::ranges::counted_iterator<I>>
      : ::ranges::_counted_iterator_::iterator_traits_<I>
    {};
}
/// \endcond

#endif
