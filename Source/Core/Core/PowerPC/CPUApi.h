// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"

#include <BasicTypes.h>
#include <export.h>

#include <functional>
#include <vector>


class FunctionRegistration {
public:
  FunctionRegistration(const char* name, const char* description, const std::vector<std::string>& arg_types) :
    m_name(name), m_description(description), m_arg_types(arg_types) {}

  std::string m_name;
  std::string m_description;
  std::vector<std::string> m_arg_types;
  void* m_function_ptr = nullptr;
};

class ModuleRegistration {
public:
  ModuleRegistration(const char* name, const char* description) : m_name(name), m_description(description) {}

  template<typename F>
  void Function(const char* name, F&& f, const char* description);

  std::string m_name;
  std::string m_description;
  std::vector<FunctionRegistration> m_functions;
};

namespace CpuApi {



ZAP_IGNORE void Init();
ZAP_IGNORE void Shutdown();

ModuleRegistration RegisterCpuApi();


}