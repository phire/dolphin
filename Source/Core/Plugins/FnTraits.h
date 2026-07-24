#pragma once

#include <wasmtime/component/val.hh>
#include "Core/System.h"
#include "Core/Core.h"
#include "WitFile.h"


using Val = wasmtime::component::Val;
using Context = wasmtime::Store::Context;
using FuncType = wasmtime::FuncType;

using WasmtimeFn = std::monostate(*)(Context cx, const FuncType& ty, std::span<Val> params, std::span<Val> results);


template<typename T>
T UnpackArg(const wasmtime::component::Val& val, wasmtime::Store::Context cx) {
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
  } else if constexpr (std::is_same_v<T, std::vector<typename T::value_type>>) {
    auto list = val.get_list();
    std::vector<typename T::value_type> result;
    result.reserve(list.size());
    for (auto&& element : list) {
      // WTF, really? wasmtime doesn't seem to be storing these very efficiently.
      // A variant for every single element of a list? They all have the same type... sigh.
      result.push_back(UnpackArg<typename T::value_type>(element, cx));
    }
    return result;

  // } else if constexpr (std::is_class_v<T> && std::is_pointer_v<T>) {
  //   return resource_cast<T>(val.get_resource());
  } else {
    static_assert(false, "Unsupported argument type");
  }
}

template<typename T>
concept BindableArg = requires(T t) {
  std::invocable<decltype(UnpackArg<T>), wasmtime::component::Val>;
};

using TypeKind = WitLexy::TypeKind;

template<typename T>
std::vector<TypeKind> ToTypeTree();

template<> std::vector<TypeKind> ToTypeTree<void>() { return {TypeKind::Void}; }
template<> std::vector<TypeKind> ToTypeTree<uint8_t>() { return {TypeKind::U8}; }
template<> std::vector<TypeKind> ToTypeTree<uint16_t>() { return {TypeKind::U16}; }
template<> std::vector<TypeKind> ToTypeTree<uint32_t>() { return {TypeKind::U32}; }
template<> std::vector<TypeKind> ToTypeTree<uint64_t>() { return {TypeKind::U64}; }
template<> std::vector<TypeKind> ToTypeTree<int8_t>() { return {TypeKind::S8}; }
template<> std::vector<TypeKind> ToTypeTree<int16_t>() { return {TypeKind::S16}; }
template<> std::vector<TypeKind> ToTypeTree<int32_t>() { return {TypeKind::S32}; }
template<> std::vector<TypeKind> ToTypeTree<int64_t>() { return {TypeKind::S64}; }
template<> std::vector<TypeKind> ToTypeTree<float>() { return {TypeKind::F32}; }
template<> std::vector<TypeKind> ToTypeTree<double>() { return {TypeKind::F64}; }
template<> std::vector<TypeKind> ToTypeTree<bool>() { return {TypeKind::Bool}; }
template<> std::vector<TypeKind> ToTypeTree<char>() { return {TypeKind::CharType}; }
template<> std::vector<TypeKind> ToTypeTree<std::string>() { return {TypeKind::StringType}; }
template<> std::vector<TypeKind> ToTypeTree<std::string_view>() { return {TypeKind::StringType}; }

template<typename T>
std::vector<TypeKind> ToTypeTree() {
    if constexpr (std::is_same_v<T, std::vector<typename T::value_type>>) {
        //return MatchList<T, typename T::value_type>();
        std::vector<TypeKind> tree{ TypeKind::ListStart };
        std::vector<TypeKind> element_tree = ToTypeTree<typename T::value_type>();
        tree.insert(tree.end(), element_tree.begin(), element_tree.end());
        tree.push_back(TypeKind::ListEnd);
        return tree;
    } else {
        static_assert(false, "Type is not bindable");
    }
}

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
  static auto unwrap_args(std::span<Val> &params, Context cx) {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(UnpackArg<std::tuple_element_t<Is, ArgTypes>>(params[Is], cx)...);
      }(std::make_index_sequence<ArgCount>{});
  }

  static auto arg_types() {
    return std::vector<WitLexy::Type>{WitLexy::Type{ToTypeTree<Args>()}...};
  }
};

template<auto, typename>
struct FnTraits;

template<auto f, typename R, typename... Args>
struct FnTraits<f, R(*)(Args...)> : public FnTraitsBase<R, Args...> {
  using unwrap_args = FnTraitsBase<R, Args...>::unwrap_args;

  static std::monostate wrapped(Context cx, const wasmtime::FuncType& ty, std::span<Val> params, std::span<Val> results) {
    if constexpr (!std::is_void_v<R>) {
      // Call the original function with the unwrapped args
      auto result = std::apply(f, unwrap_args(params, cx));

      // And store the result into the results list
      results[0] = Val(result);
    } else {
      // Special case for void functions, since void is weird.
      std::apply(f, unwrap_args(params, cx));
    }

    return std::monostate();
  }
};

template<auto f, typename R, typename C, typename... Args>
struct FnTraits<f, R(C::*)(Args...)> : public FnTraitsBase<R, Args...> {
  using ClassType = C;

  static std::monostate wrapped(Context cx, const wasmtime::FuncType& ty, std::span<Val> params, std::span<Val> results) {
    C* this_ptr = nullptr;

    if constexpr (std::is_constructible_v<C, Core::System&>) {
      auto res = params[0].get_resource().to_host(cx);
      assert(res.unwrap().rep() == 0x12);

      // TODO: support getting the system from... somewhere else. thread local storage? Encoded into
      // the 32-bit resource representation?
      auto& system = Core::System::GetInstance();
      C obj(system);
      this_ptr = &obj;
    } else {
      static_assert(false, "Unsupported class type for method binding");
    }

    params = params.subspan(1);
    auto args_tuple = FnTraitsBase<R, Args...>::unwrap_args(params, cx);
    auto this_tuple = std::tuple_cat(std::make_tuple(this_ptr), args_tuple);

    if constexpr (!std::is_void_v<R>) {
      // Call the original function with the unwrapped args
      auto result = std::apply(f, this_tuple);

      // And store the result into the results list
      results[0] = Val(result);
    } else {
      // Special case for void functions, since void is weird.
      std::apply(f, this_tuple);
    }

    return std::monostate();
  }

};