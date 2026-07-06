
// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"
#include <wasmtime/component/val.hh>

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
  true == std::is_convertible_v<T, int32_t> || true == std::is_convertible_v<T, uint32_t> ||
          true == std::is_convertible_v<T, int64_t> || true == std::is_convertible_v<T, uint64_t> ||
          true == std::is_convertible_v<T, float> || true == std::is_convertible_v<T, double> ||
          true == std::is_convertible_v<T, std::string_view>;
  // UnpackArg<T>(wasmtime::component::Val());
};

template<typename>
struct FnTraits;

template<typename R, typename... Args>
struct FnTraits<R(&)(Args...)> {
  using ReturnType = R;
  using ArgTypes = std::tuple<Args...>;

  using CppFunctionT = R(*)(Args...);
  using WrappedFunctionT = R(*)(WrappedArg<Args>...);

  static_assert((BindableArg<Args> && ...), "All arguments must be bindable");

  template<auto& f>
  struct Invoker {
    using Val = wasmtime::component::Val;
    using Context = wasmtime::Store::Context;
    using FuncType = wasmtime::FuncType;

    static std::monostate wrapped(Context cx, const FuncType& ty, std::span<Val> params, std::span<Val> results) {
      size_t i = 0;
      if constexpr (!std::is_same_v<R, void>) {
        auto result = f(UnpackArg<Args>(params[i++])...);
        results[0] = Val(result);
      } else {
        f(UnpackArg<Args>(params[i++])...);
      }

      return std::monostate();
    }
  };
};


template<auto& f, std::size_t N>
struct Wrapped {
  using Traits = FnTraits<decltype(f)>;
  using Invoker = typename Traits::template Invoker<f>;

  Wrapped(std::string_view name, const char* (&&arg_names)[N])
  : m_name(name) {
    static_assert(std::tuple_size_v<typename Traits::ArgTypes> == N, "Argument count mismatch");

    std::copy(std::begin(arg_names), std::end(arg_names), m_arg_names.begin());

    m_cpp_ptr = reinterpret_cast<void*>(f);
    m_raw_ptr = reinterpret_cast<void*>(Invoker::wrapped);
  }

  std::string_view m_name;
  std::array<std::string_view, N> m_arg_names;

  void* m_cpp_ptr;
  void* m_raw_ptr;
};

class FunctionBinding {
public:

  template<typename T>
  FunctionBinding(T& wrapped) : m_name(wrapped.m_name), m_cpp_ptr(wrapped.m_cpp_ptr), m_wrapped_ptr(wrapped.m_raw_ptr)
  {}

  std::string m_name;
  std::string m_description;
  std::vector<std::string> m_arg_types;
  void* m_cpp_ptr = nullptr;
  void* m_wrapped_ptr = nullptr;
private:
  FunctionBinding(const char* name, const char* description, const std::vector<std::string>& arg_types) :
    m_name(name), m_description(description), m_arg_types(arg_types) {}
};


template<auto& f, std::size_t N>
FunctionBinding wrap_fn(std::string_view name, const char* (&&arg_names)[N]) {
  using Wrapped = Wrapped<f, N>;
  Wrapped wrapped(name, std::move(arg_names));

  return FunctionBinding(wrapped);
}

class ComponentBinding {
public:
  ComponentBinding(const char* name, const char* description) : m_name(name), m_description(description) {}

  void add(FunctionBinding&& binding) {
    m_functions.push_back(std::move(binding));
  }

  std::string m_name;
  std::string m_description;
  std::vector<FunctionBinding> m_functions;
};

}