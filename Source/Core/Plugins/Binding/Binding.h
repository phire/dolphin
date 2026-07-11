
// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"
#include <wasmtime/component/val.hh>

#include <tuple>
#include <functional>

namespace Plugin {

template<typename T>
T UnpackArg(wasmtime::component::Val& val) {
  // TODO: more
  if constexpr (std::is_convertible_v<T, int32_t>) {
    return val.get_s32();
  } else if constexpr (std::is_convertible_v<T, uint32_t>) {
    return val.get_u32();
  } else if constexpr (std::is_convertible_v<T, int64_t>) {
    return val.get_s64();
  } else if constexpr (std::is_convertible_v<T, uint64_t>) {
    return val.get_u64();
  } else if constexpr (std::is_convertible_v<T, float>) {
    return val.get_f32();
  } else if constexpr (std::is_convertible_v<T, double>) {
    return val.get_f64();
  } else if constexpr (std::is_convertible_v<T, std::string_view>) {
    return val.get_string();
  } else {
    static_assert(false, "Unsupported argument type");
  }
}

template<typename T>
concept BindableArg = requires(T t) {
  std::invocable<decltype(UnpackArg<T>), wasmtime::component::Val>;
};

using Val = wasmtime::component::Val;
using Context = wasmtime::Store::Context;
using FuncType = wasmtime::FuncType;


template<typename R, typename... Args>
struct FnTraitsBase {
  // Get nice error messages if the arguments are not bindable
  static_assert((BindableArg<Args> && ...), "All arguments must be bindable");

  using ReturnType = R;
  constexpr static bool is_void = std::is_void_v<R>;
  using ArgTypes = std::tuple<Args...>;
  constexpr static std::size_t ArgCount = sizeof...(Args);

  // This pulls all each args from the correct index, unpacks them, and builds a tuple
  // the first arg is found at params[0], etc
  static auto unwrap_args(std::span<wasmtime::component::Val> &params) {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(UnpackArg<std::tuple_element_t<Is, ArgTypes>>(params[Is])...);
      }(std::make_index_sequence<ArgCount>{});
  }
};

template<typename>
struct FnTraits;

template<typename R, typename... Args>
struct FnTraits<R(*)(Args...)> : public FnTraitsBase<R, Args...> {

  template<auto f>
  struct Invoker {
    static std::monostate wrapped(Context cx, const FuncType& ty, std::span<Val> params, std::span<Val> results) {

      if constexpr (!std::is_void_v<R>) {
        // Call the original function with the unwrapped args
        auto result = std::apply(f, FnTraits::unwrap_args(params));

        // And store the result into the results list
        results[0] = Val(result);
      } else {
        // Special case for void functions, since void is weird.
        std::apply(f, FnTraits::unwrap_args(params));
      }

      return std::monostate();
    }
  };
};

template<typename R, typename C, typename... Args>
struct FnTraits<R(C::*)(Args...)> : public FnTraitsBase<R, Args...> {
  using ClassType = C;

  template<auto f>
  struct Invoker {
    static std::monostate wrapped(Context cx, const FuncType& ty, std::span<Val> params, std::span<Val> results) {
      // Call the original function with the unwrapped args
      auto bound = std::bind_front(f, static_cast<C*>(nullptr));
      if constexpr (!std::is_void_v<R>) {
        auto result = std::apply(bound, FnTraits::unwrap_args(params));

         // And store the result into the results list
        results[0] = Val(result);
      } else {
        // Special case for void functions, since void is weird.
        std::apply(bound, FnTraits::unwrap_args(params));
      }
      return std::monostate();
    }
  };
};



}

template<size_t Footprint>
struct cxstring
{
    char data[Footprint];
    constexpr size_t size() const { return Footprint - 1; }
    constexpr cxstring(const char (&init)[Footprint])
    { std::copy_n(init, Footprint, data); }
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

template <auto f, auto name>
struct Method {
  using name_t = decltype(name);
  using Traits = Plugin::FnTraits<decltype(f)>;
    static constexpr std::string_view binding_name() {
        return name.view();
    }
};
