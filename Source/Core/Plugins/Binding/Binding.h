
// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"


template<size_t Footprint>
struct cxstring
{
    char data[Footprint];
    constexpr size_t size() const { return Footprint - 1; }
    constexpr cxstring(const char (&init)[Footprint])
    { std::copy_n(init, Footprint, data); }
    constexpr cxstring(const std::string_view& str)
    {
        size_t i = 0;
        for (; i < str.size() && i < Footprint - 1; ++i) {
            data[i] = str[i];
        }
        data[i] = '\0';
    }

};

template<auto str>
struct type_string
{
    using type_string_tag = void;
    static constexpr const char* data()
    {
         return str.data;
    }
    static constexpr size_t size()
    {
        return str.size();
    }
    static constexpr std::string_view view()
    {
        return std::string_view{data(), size()};
    }
};

template<cxstring str>
constexpr auto operator"" _t()
{
    return type_string<str>{};
}

constexpr auto empty_type_string_v = type_string<cxstring("")>{};
using empty_type_string = decltype(empty_type_string_v);

template<auto, typename>
struct FnTraits;

template <auto f, auto name>
struct Method {
  using name_t = decltype(name);
  using Traits = FnTraits<f, decltype(f)>;
  static constexpr std::string_view binding_name() {
      return name.view();
  }

  static constexpr bool is_method = true;

  static constexpr auto fn_ptr = f;

};
