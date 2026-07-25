// SPDX-License-Identifier: CC0-1.0

#pragma once

#include "Common/CommonTypes.h"
#include "Common/Assert.h"

#include <array>
#include <bit>
#include <cstddef>
#include <span>
#include <algorithm>

namespace Common
{

class U32ViewIterator
{
public:
  using value_type = const u32;
  using difference_type = std::ptrdiff_t;
  using iterator_concept  = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;

  U32ViewIterator() = default;
  explicit U32ViewIterator(std::span<const u8> bytes) : m_bytes(bytes) {
    ASSERT_MSG(COMMON, bytes.size() % sizeof(u32) == 0, "Byte span size must be a multiple of 4.");
  }

  value_type operator*() const
  {
    std::array<u8, sizeof(value_type)> arr;
    std::copy_n(m_bytes.begin(), sizeof(value_type), arr.begin());
    return std::bit_cast<value_type>(arr);
  }

  value_type operator[](std::ptrdiff_t index) const
  {
    U32ViewIterator tmp = *this;
    tmp += index;
    return *tmp;
  }

  U32ViewIterator& operator++()
  {
    m_bytes = m_bytes.subspan(sizeof(value_type));
    return *this;
  }

  U32ViewIterator operator++(int)
  {
    U32ViewIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  U32ViewIterator& operator--()
  {
    m_bytes = m_bytes.subspan(-sizeof(value_type));
    return *this;
  }

  U32ViewIterator operator--(int)
  {
    U32ViewIterator tmp = *this;
    --(*this);
    return tmp;
  }

  U32ViewIterator& operator+=(difference_type n)
  {
    m_bytes = m_bytes.subspan(n * sizeof(value_type));
    return *this;
  }

  U32ViewIterator& operator-=(difference_type n)
  {
    m_bytes = m_bytes.subspan(-n * sizeof(value_type));
    return *this;
  }

  bool operator==(const U32ViewIterator& other) const
  {
    return m_bytes.data() == other.m_bytes.data() && m_bytes.size() == other.m_bytes.size();
  }

  friend auto operator<=>(const U32ViewIterator& lhs, const U32ViewIterator& rhs) {
    return lhs.m_bytes.data() <=> rhs.m_bytes.data();
  }

  friend difference_type operator-(const U32ViewIterator& lhs, const U32ViewIterator& rhs)
  {
    return (lhs.m_bytes.data() - rhs.m_bytes.data()) / sizeof(value_type);
  }

  friend U32ViewIterator operator+(const U32ViewIterator& it, difference_type n)
  {
    U32ViewIterator tmp = it;
    tmp += n;
    return tmp;
  }

  friend U32ViewIterator operator+(difference_type n, const U32ViewIterator& it)
  {
    return it + n;
  }

  friend U32ViewIterator operator-(const U32ViewIterator& it, difference_type n)
  {
    return it + (-n);
  }

private:
  std::span<const u8> m_bytes;
};

static_assert(std::random_access_iterator<U32ViewIterator>);

class U32View : public std::ranges::view_interface<U32View>
{

public:
  explicit U32View(std::span<const u8> bytes) : m_bytes(bytes) {}

  U32ViewIterator begin() const { return U32ViewIterator(m_bytes); }
  U32ViewIterator end() const { return U32ViewIterator(m_bytes.subspan(m_bytes.size())); }

  bool valid() const { return m_bytes.size() % sizeof(u32) == 0; }

private:
  std::span<const u8> m_bytes;
};

} // namespace Common
