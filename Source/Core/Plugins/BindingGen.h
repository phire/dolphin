#pragma once

#include "Plugins/WitFile.h"


#include <optional>
#include <source_location>
#include <string_view>
#include <typeinfo>
#include <span>

using Type = WitLexy::Type;

struct MethodBinding {
    std::string_view name = {};
    std::source_location binding_location = {};
    Type return_type = {};
    std::vector<Type> arg_types = {};
    std::type_info const& resource_type = typeid(void);
};

struct ResourceBinding {
    std::string_view name = {};
    std::source_location binding_location = {};
    const std::type_info& resource_type = typeid(void);
};

struct Bindings {
    std::span<const MethodBinding> methods;
    std::span<const ResourceBinding> resources;
};

unsigned get_resource_count();
std::optional<unsigned> get_resource_idx(std::string_view name);
std::optional<unsigned> get_resource_idx(std::type_info const& type);
std::optional<std::string_view> get_resource_name(std::type_info const& type);

Bindings get_bindings();

