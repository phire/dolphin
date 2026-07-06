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

namespace Plugin {
    class ComponentBinding;
}

namespace CpuApi {


ZAP_IGNORE void Init();
ZAP_IGNORE void Shutdown();

Plugin::ComponentBinding RegisterCpuApi();


}