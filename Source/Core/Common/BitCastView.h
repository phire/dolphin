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
    && std::contiguous_iterator<std::ranges::iterator_t<V>>
    && sizeof(To) >= sizeof(std::ranges::range_value_t<V>)
    && (sizeof(To) / sizeof(std::ranges::range_value_t<V>)) * sizeof(std::ranges::range_value_t<V>) == sizeof(To)
)
class BitCastViewIterator
{
  using from_type = std::ranges::range_value_t<V>;
  using from_iterator = std::ranges::iterator_t<V>;
  using from_view = V;
  constexpr static size_t step = sizeof(To) / sizeof(from_type);
public:
  using value_type = To;
  using difference_type = std::ptrdiff_t;

  using iterator_concept  = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;

  BitCastViewIterator() = default;
  explicit BitCastViewIterator(V view) : m_view(view) {
    ASSERT_MSG(COMMON, (view.size() * sizeof(from_type)) % sizeof(value_type) == 0, "Byte span size must be a multiple of value_type.");
  }

  value_type operator*() const
  requires (step > 1)
  {
    std::array<from_type, step> arr;
    std::copy_n(m_view.begin(), step, arr.begin());
    return std::bit_cast<value_type>(arr);
  }

  value_type& operator*() const
  requires (step == 1)
  {
    return std::bit_cast<value_type>(*m_view.begin());
  }


  value_type operator[](std::ptrdiff_t index) const
  {
    BitCastViewIterator tmp = *this;
    tmp += index;
    return *tmp;
  }

  BitCastViewIterator& operator++()
  {
    m_view = m_view.subspan(step);
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
    m_view = m_view.subspan(-step);
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
    m_view = m_view.subspan(n * step);
    return *this;
  }


  BitCastViewIterator& operator-=(difference_type n)
  {
    m_view = m_view.subspan(-n * step);
    return *this;
  }

  bool operator==(const BitCastViewIterator& other) const
  {
    return m_view.begin() == other.m_view.begin() && m_view.size() == other.m_view.size();
  }

  friend auto operator<=>(const BitCastViewIterator& lhs, const BitCastViewIterator& rhs) {
    return lhs.m_view.begin() <=> rhs.m_view.begin();
  }

  friend difference_type operator-(const BitCastViewIterator& lhs, const BitCastViewIterator& rhs)
  {
    return (lhs.m_view.begin() - rhs.m_view.begin()) / step;
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
  V m_view;
};

static_assert(std::random_access_iterator<BitCastViewIterator<u32, std::span<const u8>>>);

template <typename To, std::ranges::view V>

class BitCastViewImpl : public std::ranges::view_interface<BitCastViewImpl<To, V>>
{
public:
  explicit BitCastViewImpl(V view) : m_view(view) {}

  BitCastViewIterator<To, V> begin() const { return BitCastViewIterator<To, V>(m_view); }
  BitCastViewIterator<To, V> end() const {
     BitCastViewIterator<To, V> iter(m_view);
     iter += (m_view.size() * sizeof(std::ranges::range_value_t<V>)) / sizeof(To);
     return iter;
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
