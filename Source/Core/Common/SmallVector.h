// SPDX-License-Identifier: CC0-1.0

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <utility>
#include <ranges>

#include "Common/TypeUtils.h"

namespace Common
{

// TODO C++26: Replace with std::inplace_vector.

// An std::vector-like container that uses no heap allocations but is limited to a maximum size.
template <typename T, size_t MaxSize>
class SmallVector final
{
public:
  using value_type = T;

  constexpr SmallVector() = default;
  explicit constexpr SmallVector(size_t new_size) { resize(new_size); }

  constexpr ~SmallVector() { clear(); }

  constexpr SmallVector(const SmallVector& other)
  {
    //for (auto& value : other)
    for (const auto& value : other.view())
      emplace_back(value);
  }

  constexpr SmallVector& operator=(const SmallVector& rhs)
  {
    clear();
    for (const auto& value : rhs.view())
      emplace_back(value);
    return *this;
  }

  constexpr SmallVector(SmallVector&& other)
  {
    for (auto&& value : other.view())
      emplace_back(std::move(value));
    other.clear();
  }

  constexpr SmallVector& operator=(SmallVector&& rhs)
  {
    clear();
    for (auto&& value : rhs.view())
      emplace_back(std::move(value));
    rhs.clear();
    return *this;
  }

  constexpr void push_back(const value_type& x) { emplace_back(x); }
  constexpr void push_back(value_type&& x) { emplace_back(std::move(x)); }

  template <typename... Args>
  constexpr value_type& emplace_back(Args&&... args)
  {
    assert(m_size < MaxSize);
    return m_array[m_size++].Construct(std::forward<Args>(args)...);
  }

  constexpr void pop_back()
  {
    assert(m_size > 0);
    m_array[--m_size].Destroy();
  }

  constexpr value_type& operator[](size_t i)
  {
    assert(i < m_size);
    //return r[i];
    return m_array[i].Ref();
  }
  constexpr const value_type& operator[](size_t i) const
  {
    assert(i < m_size);
    return m_array[i].Ref();
  }

  // These don't work in constexpr contexts
  value_type* data() { return m_array.data()->Ptr(); }
  value_type* begin() { return data(); }
  value_type* end() { return data() + m_size; }

  auto data() const { return m_array.data()->Ptr(); }
  const value_type* begin() const { return data(); }
  const value_type* end() const { return data() + m_size; }

  constexpr auto view() const {
    return std::ranges::views::transform(m_array | std::views::take(m_size), [](const auto& v) constexpr { return v.Ref(); });
  }

  constexpr auto view() {
    return std::ranges::views::transform(m_array | std::views::take(m_size), [](auto& v) constexpr { return v.Ref(); });
  }

  constexpr auto ptr_view() const {
    return std::ranges::views::transform(m_array | std::views::take(m_size), [](const auto& v) constexpr { return v.Ptr(); });
  }
  constexpr auto ptr_view() {
    return std::ranges::views::transform(m_array | std::views::take(m_size), [](auto& v) constexpr { return v.Ptr(); });
  }


  constexpr size_t capacity() const { return MaxSize; }
  constexpr size_t size() const { return m_size; }

  constexpr bool empty() const { return m_size == 0; }

  constexpr void resize(size_t new_size)
  {
    assert(new_size <= MaxSize);

    while (size() < new_size)
      emplace_back();

    while (size() > new_size)
      pop_back();
  }

  constexpr void clear() { resize(0); }

private:
  std::array<ManuallyConstructedValue<T>, MaxSize> m_array;
  size_t m_size = 0;
};

}  // namespace Common
