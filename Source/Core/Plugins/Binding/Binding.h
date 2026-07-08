
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

template<auto f>
struct Wrapped {
  using Traits = FnTraits<decltype(f)>;
  //using Invoker = typename Traits::template Invoker<f>;
  static constexpr std::size_t ArgCount = Traits::ArgCount;

  Wrapped(std::string_view name) : m_name(name) {
    // m_cpp_ptr = reinterpret_cast<void*>(f);
    // m_raw_ptr = reinterpret_cast<void*>(Invoker::wrapped);
  }

  void* GetWrapped() const {
    using Invoker = typename Traits::template Invoker<f>;
    return reinterpret_cast<void*>(Invoker::wrapped);
  }

  std::string_view m_name;

  // void* m_cpp_ptr;
  // void* m_raw_ptr;
};

template<auto f>
auto wrap(std::string_view name) {
  //static_assert(!Wrapped<f>::Traits::IsMethod, "Cannot wrap a method with this function");
  return Wrapped<f>(name);
}

struct ArgDesc {
  std::string_view GetName() const { return m_name; }
  std::string_view GetDescription() const { return m_description; }

  ArgDesc(const char* name, const char* description) : m_name(name), m_description(description) {}
  ArgDesc(const char* name) : m_name(name) {}
  ArgDesc() = default;
private:
  std::string_view m_name = std::string_view();
  std::string_view m_description = std::string_view();
};

struct TypedArgDesc : public ArgDesc {
  TypedArgDesc(ArgDesc&& desc, wasmtime::component::ValType kind) : ArgDesc(std::move(desc)), m_kind(kind) {}

private:
  wasmtime::component::ValType m_kind;
};

template<std::size_t N>
struct Sig : std::array<ArgDesc, N> {
  using std::array<ArgDesc, N>::array;
  Sig(const ArgDesc (&args)[N]) {
    std::copy(std::begin(args), std::end(args), this->begin());
  }
};

template <std::size_t N>
Sig(const ArgDesc (&)[N]) -> Sig<N>;

class MethodBinding {
public:
  template<typename T, typename W, std::size_t N>
  MethodBinding(T& resource, W& wrapped, std::string_view name, Sig<N> arg_names) : m_name(name)
  {
    static_assert(W::ArgCount == N, "Argument count mismatch");
  }

  std::string m_name;
  std::string m_description;
  std::vector<ArgDesc> m_args;
  //void* m_cpp_ptr = nullptr;
  void* m_wrapped_ptr = nullptr;
};

template<class T>
struct Resource {

  Resource(std::string_view name, std::string_view description) : m_name(name), m_description(description) {}

  template<typename W, std::size_t N>
  void add(W &&wrapped, Sig<N> arg_names) {
    m_methods.push_back(MethodBinding(*this, wrapped, "", std::move(arg_names)));
  }

  std::vector<MethodBinding> m_methods;

  std::string_view m_name;
  std::string_view m_description;
};

class ResourceBinding {
public:
  ResourceBinding() = default;
  template<typename T>
  ResourceBinding(T&& resource) : m_name(resource.m_name), m_description(resource.m_description), m_methods(std::move(resource.m_methods)) {}

  std::string m_name;
  std::string m_description;
  std::vector<MethodBinding> m_methods;
};

class FunctionBinding {
public:

  template<typename T, std::size_t N>
  FunctionBinding(T& wrapped, std::string_view name, Sig<N> arg_names) : m_name(name)
  {
    static_assert(T::Traits::ArgCount == N, "Argument count mismatch");

    m_wrapped_ptr = wrapped.GetWrapped();
    // for (std::size_t i = 0; i < T::Traits::ArgCount; ++i) {
    //   m_args.push_back(TypedArgDesc(arg_names[i], T::Traits::ArgTypes[i]));
    // }
    std::copy(arg_names.begin(), arg_names.end(), std::back_inserter(m_args));
  }

  std::string m_name;
  std::string m_description;
  std::vector<ArgDesc> m_args;
  //void* m_cpp_ptr = nullptr;
  void* m_wrapped_ptr = nullptr;
private:
};


template<auto f, std::size_t N>
FunctionBinding wrap_fn(std::string_view name, Sig<N> arg_names) {
  Wrapped<f> wrapped(name);

  return FunctionBinding(wrapped, name, std::move(arg_names));
}

template<auto& f, std::size_t N>
FunctionBinding wrap_fn(std::string_view name, const ArgDesc (&arg_names)[N]) {
  return wrap_fn<&f>(name, Sig<N>(arg_names));
}

class ComponentBinding {
public:
  ComponentBinding() = default;
  ComponentBinding(const char* name, const char* description) : m_name(name), m_description(description) {}

  void add(FunctionBinding&& binding) {
    m_functions.push_back(std::move(binding));
  }

  std::string m_name;
  std::string m_description;
  std::vector<FunctionBinding> m_functions;
};

}