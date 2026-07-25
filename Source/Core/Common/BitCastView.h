// SPDX-License-Identifier: CC0-1.0

#pragma once

#include "Common/CommonTypes.h"
#include "Common/Assert.h"

#include <array>
#include <bit>
#include <cstddef>
#include <optional>
#include <span>
#include <algorithm>

namespace Common
{

template <typename To, std::ranges::view V>
requires (
    std::is_trivially_copyable_v<To>
    && std::is_trivially_copyable_v<std::ranges::range_value_t<V>>
    && std::random_access_iterator<std::ranges::iterator_t<V>>
    && sizeof(To) >= sizeof(std::ranges::range_value_t<V>)
    && (sizeof(To) / sizeof(std::ranges::range_value_t<V>)) * sizeof(std::ranges::range_value_t<V>) == sizeof(To)
)
class BitCastViewIterator
{
  using from_type = std::ranges::range_value_t<V>;
  using from_iterator = std::ranges::iterator_t<V>;
  constexpr static size_t step = sizeof(To) / sizeof(from_type);
public:
  using value_type = To;
  using difference_type = std::ptrdiff_t;

  BitCastViewIterator() = default;
  explicit BitCastViewIterator(from_iterator iter) : m_iter(iter) {
  }

  value_type operator*() const
  requires (step > 1)
  {
    std::array<from_type, step> arr;
    std::copy_n(m_iter, step, arr.begin());
    return std::bit_cast<value_type>(arr);
  }

  value_type& operator*() const
  requires (step == 1)
  {
    return std::bit_cast<value_type>(*m_iter);
  }


  value_type operator[](std::ptrdiff_t index) const
  {
    BitCastViewIterator tmp = *this;
    tmp += index;
    return *tmp;
  }

  BitCastViewIterator& operator++()
  {
    m_iter += step;
    return *this;
  }

  BitCastViewIterator operator++(int)
  {
    BitCastViewIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  BitCastViewIterator& operator--()
  {
    m_iter -= step;
    return *this;
  }

  BitCastViewIterator operator--(int)
  {
    BitCastViewIterator tmp = *this;
    --(*this);
    return tmp;
  }

  BitCastViewIterator& operator+=(difference_type n)
  {
    m_iter += n * step;
    return *this;
  }

  BitCastViewIterator& operator-=(difference_type n)
  {
    m_iter -= n * step;
    return *this;
  }

  bool operator==(const BitCastViewIterator& other) const = default;

  friend auto operator<=>(const BitCastViewIterator& lhs, const BitCastViewIterator& rhs) = default;

  friend difference_type operator-(const BitCastViewIterator& lhs, const BitCastViewIterator& rhs)
  {
    return (lhs.m_iter - rhs.m_iter) / step;
  }

  friend BitCastViewIterator operator+(const BitCastViewIterator& it, difference_type n)
  {
    BitCastViewIterator tmp = it;
    tmp += n;
    return tmp;
  }

  friend BitCastViewIterator operator+(difference_type n, const BitCastViewIterator& it)
  {
    return it + n;
  }

  friend BitCastViewIterator operator-(const BitCastViewIterator& it, difference_type n)
  {
    return it + (-n);
  }

private:
  from_iterator m_iter;
};

static_assert(std::random_access_iterator<BitCastViewIterator<u32, std::span<const u8>>>);

template <typename To, std::ranges::view V>
class BitCastViewImpl : public std::ranges::view_interface<BitCastViewImpl<To, V>>
{
  using from_type = std::ranges::range_value_t<V>;
  using from_iterator = std::ranges::iterator_t<V>;
public:
  explicit BitCastViewImpl(V view) : m_view(view) {}

  BitCastViewIterator<To, V> begin() const {
    ASSERT_MSG(COMMON, (m_view.size() * sizeof(from_type)) % sizeof(To) == 0, "Byte span size must be a multiple of value_type.");
    return BitCastViewIterator<To, V>(m_view.begin());
   }
  BitCastViewIterator<To, V> end() const {
     return BitCastViewIterator<To, V>(m_view.end());
  }

  bool valid() const { return m_view.size() % sizeof(To) == 0; }

private:
  V m_view;
};

template <typename To, std::ranges::view V>

auto BitCastView(V view) -> std::optional<BitCastViewImpl<To, V>> {
  if (view.size() % sizeof(To) != 0)
    return std::nullopt;
  return BitCastViewImpl<To, V>(view);
}


} // namespace Common
