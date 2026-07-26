#include <optional>
#include <string_view>
#include "BindingGen.h"
#include "DolphinBindings.h"

#include "Common/SmallVector.h"
#include "Plugins/WitFile.h"
#include "FnTraits.h"

struct BindingCounts {
    size_t methods = 0;
    size_t resources = 0;
};

static constexpr BindingCounts s_counts = []() constexpr {
    BindingCounts counts;
    dolphin_bindings([&] (auto item, std::source_location loc = std::source_location::current()) constexpr {
        if constexpr (item.is_method) {
            counts.methods++;
        } else if constexpr (item.is_resource) {
            counts.resources++;
        } else {
            static_assert(false, "Unhandled type in binder");
        }
    });
    return counts;
}();

constexpr std::array<WasmtimeFn, s_counts.methods> s_methods = [] consteval {
    std::array<WasmtimeFn, s_counts.methods> methods{};
    size_t index = 0;
    dolphin_bindings([&](auto item, std::source_location loc = std::source_location::current()) constexpr {
        if constexpr (item.is_method) {
            using Traits = decltype(item)::Traits;
            methods[index++] = Traits::wrapped;
        }
    });
    return methods;
}();

constexpr std::array<std::pair<std::string_view, std::type_info const*>, s_counts.resources>
s_resource_mapping = [] consteval {
    std::array<std::pair<std::string_view, std::type_info const*>, s_counts.resources> resources{};
    size_t index = 0;
    dolphin_bindings([&](auto item, std::source_location loc = std::source_location::current()) constexpr {
        if constexpr (item.is_resource) {
            resources[index++] = std::make_pair(item.binding_name(), &typeid(typename decltype(item)::ResourceType));
        }
    });
    return resources;
}();


unsigned get_resource_count() {
    return s_counts.resources;
}

using resource_iterator = decltype(s_resource_mapping)::const_iterator;

std::optional<unsigned> get_resource_idx(std::string_view name) {
    for (unsigned i = 0; i < s_resource_mapping.size(); ++i) {
        if (s_resource_mapping[i].first == name) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<unsigned> get_resource_idx(std::type_info const& type) {
    for (unsigned i = 0; i < s_resource_mapping.size(); ++i) {
        if (s_resource_mapping[i].second == &type) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> get_resource_name(std::type_info const& type) {
    for (unsigned i = 0; i < s_resource_mapping.size(); ++i) {
        if (s_resource_mapping[i].second == &type) {
            return s_resource_mapping[i].first;
        }
    }
    return std::nullopt;
}

template <typename T>
static consteval resource_iterator get_resource() {
    return std::ranges::find_if(s_resource_mapping, [&](const auto& resource) {
        return resource.second == &typeid(std::remove_cvref_t<T>);
    });
}

struct StaticBindings {
    Common::SmallVector<MethodBinding, s_counts.methods> methods;
    Common::SmallVector<ResourceBinding, s_counts.resources> resources;
};

Bindings get_bindings() {
    static StaticBindings s_bindings = [] constexpr {

        Common::SmallVector<MethodBinding, s_counts.methods> methods;
        Common::SmallVector<ResourceBinding, s_counts.resources> resources;

        // Collect all bindings
        dolphin_bindings([&](auto item, std::source_location loc = std::source_location::current()) constexpr {
            if constexpr (item.is_method) {
                using Traits = decltype(item)::Traits;
                methods.emplace_back(item.binding_name(), loc, WitLexy::Type{ToTypeTree<typename Traits::ReturnType>()}, Traits::arg_types(), typeid(typename Traits::ClassType));
            } else if constexpr (item.is_resource) {
                resources.emplace_back(item.binding_name(), loc, typeid(typename decltype(item)::ResourceType));
            } else {
                static_assert(false, "Unhandled type in binder");
            }
        });

        return StaticBindings{std::move(methods), std::move(resources)};
    }();

    return Bindings{
        std::span(s_bindings.methods),
        std::span(s_bindings.resources)
    };
}

