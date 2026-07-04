// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"

#include <BasicTypes.h>
#include <export.h>

#include <functional>
#include <vector>


u64 ConvertArg(u64 arg) {
  return arg;
}

u64 ConvertArg(u32 arg) {
  return static_cast<u64>(arg);
}

template<typename T>
u64 ConvertArg(T arg) {
  return arg.operator void*();
}

u64 ConvertArg(float arg) {
  return static_cast<u64>(std::bit_cast<u32>(arg));
}

u64 ConvertArg(double arg) {
  return std::bit_cast<u64>(arg);
}

template<typename ArgType>
class WrappedArg {
public:
  using type = u64;
  WrappedArg(ArgType arg) {
    m_value = ConvertArg(arg);
  }

  u64 Unwrap() {
    return static_cast<ArgType>(m_value);
  }

private:
  u64 m_value;
};

template<const auto f, typename R, typename... Args>
class WrappedFunction {
public:
  static R wrapped(WrappedArg<Args>... args) {
    return f(args.Unwrap()...);
  }
};

class FunctionRegistration {
public:
  FunctionRegistration(const char* name, const char* description, const std::vector<std::string>& arg_types) :
    m_name(name), m_description(description), m_arg_types(arg_types) {}

  std::string m_name;
  std::string m_description;
  std::vector<std::string> m_arg_types;
  void* m_function_ptr = nullptr;
  void* m_wrapped_function_ptr = nullptr;
};


template<typename R, typename... Args>
struct FunctionMaker {
    using FnPtr = R(*)(Args...);

    template <FnPtr f>
    static FunctionRegistration make(const char* name) {
        std::vector<std::string> arg_types;
        auto fn = FunctionRegistration(name, "", arg_types);

        fn.m_function_ptr = reinterpret_cast<void*>(f);
        fn.m_wrapped_function_ptr = reinterpret_cast<void*>(WrappedFunction<f, R, Args...>::wrapped);

        return fn;
    }
};

class ModuleRegistration {
public:
  ModuleRegistration(const char* name, const char* description) : m_name(name), m_description(description) {}

  template<typename F>
  void Function(const char* name, const F&& f, const char* description);


  //template<typename R, typename... Args, R(Args...)* Fn>
//   void Funct(const char* name) {
//     std::vector<std::string> arg_types;
//     auto& fn = m_functions.emplace_back(name, "", arg_types);

//     //fn.wrapped_function_ptr = reinterpret_cast<void*>(WrappedFunction<Fn, R, Args...>::wrapped);
//   }

  std::string m_name;
  std::string m_description;
  std::vector<FunctionRegistration> m_functions;
};



namespace CpuApi {



ZAP_IGNORE void Init();
ZAP_IGNORE void Shutdown();

ModuleRegistration RegisterCpuApi();


}