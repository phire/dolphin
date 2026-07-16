// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Plugins/PluginHost.h"
#include "Common/Logging/Log.h"
#include "Plugins/ModuleManager.h"
#include "Core/PowerPC/CPUApi.h"

#include <dlfcn.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <vector>
#include <functional>

#include <wasmtime.hh>
#include <wasmtime/component/component.hh>
#include <wasmtime/component/linker.hh>

#include "Plugins/WitParser.h"

#include "Plugins/WitParserImpl.h"
#include "Plugins/WitLexy.h"

#include <lexy/input/string_input.hpp>
#include <lexy/action/parse.hpp>
#include <lexy/action/validate.hpp>
#include <lexy_ext/report_error.hpp>

#include "Common/SmallVector.h"
#include "Binding/Binding.h"

static uint64_t Module_id = 0x1000;
static std::vector<Plugins::PluginFiles> plugin_entries;

void* Plugins::GetFnPtrFunctor::GetFunction(void* data_void, String* module_name, int64_t version, String* function) {
    auto data = reinterpret_cast<GetFnPtrFunctorData*>(data_void);

    auto plugin = &plugin_entries[data->plugin_idx];


    // Because we give one of these Functors to each plugin, we now know which plugin called us
    uint64_t plugin_id [[maybe_unused]] = plugin->ModuleId;

    // In future versions of this API, we can do fancy things based on which plugin called us
    // and create per-plugin wrappers for every API function

    // For now, we just call return the function pointer
    return GetFnPtr(module_name->Data, version, function->Data);
}

using namespace wasmtime;

void hello(std::string_view msg) {
    fmt::print("hello called with message: \"{}\"\n", msg);
}

Result<std::monostate> hello_wrapper(Store::Context cx, const component::FuncType& ty, std::span<component::Val> params, std::span<component::Val> results) {
    hello(params[0].get_string());

    return std::monostate();
}

Result<std::monostate> hi(Store::Context cx, const component::FuncType& ty, std::span<component::Val> params, std::span<component::Val> results) {
    fmt::print("hi called from component!\n");
    return std::monostate();
}

struct FixedStr {
    char data[64] = {};
    size_t size = 0;
    constexpr FixedStr() : data{}, size(0) {}
    constexpr FixedStr(const FixedStr& other) = default;
    constexpr FixedStr(std::string_view str) {
        size = 0;
        while (size < str.size() && size < sizeof(data) - 1) {
            data[size] = str[size];
            ++size;
        }
        data[size] = '\0';
    }

    constexpr std::string_view view() const {
        return std::string_view(data, size);
    }
};


template <typename T, typename StrT = empty_type_string>
struct TypeTree {
    using Type = T;
    using Name = StrT;

    template <typename U>
    static constexpr void is_same() {
        static_assert(std::is_same_v<Type, U>, "Type mismatch with dolphin.wit");
    }

    template <typename NewStrT>
    static constexpr TypeTree<Type, NewStrT> named(NewStrT id) {
        return {};
    }

};

struct FixNamedType {
    FixedStr name;
    // const TypeTree type;

    constexpr FixNamedType() = default;
    constexpr FixNamedType(const WitLexy::NamedType& other) : name(other.id) {}
};

struct FixedLenFunc {
    // bool async = false;
    // bool static_ = false;
    // bool constructor = false;
    FixedStr name = {};
    Common::SmallVector<FixNamedType, 20> args = {};
    // std::array<Wit::Ty, 4> results;
};

struct FixedLenResource {
    FixedStr name;
    Common::SmallVector<FixedLenFunc, 20> methods = {};
};



// template<size_t N, auto arr>
// static constexpr auto typetree();

template<const WitLexy::TypeKind kind>
static constexpr auto typetree() {
    if constexpr (kind == WitLexy::TypeKind::U8) {
        return TypeTree<uint8_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::U16) {
        return TypeTree<uint16_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::U32) {
        return TypeTree<uint32_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::U64) {
        return TypeTree<uint64_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S8) {
        return TypeTree<int8_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S16) {
        return TypeTree<int16_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S32) {
        return TypeTree<int32_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S64) {
        return TypeTree<int64_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::F32) {
        return TypeTree<float>{};
    } else if constexpr (kind == WitLexy::TypeKind::F64) {
        return TypeTree<double>{};
    } else if constexpr (kind == WitLexy::TypeKind::Bool) {
        return TypeTree<bool>{};
    } else if constexpr (kind == WitLexy::TypeKind::CharType) {
        return TypeTree<char>{};
    } else if constexpr (kind == WitLexy::TypeKind::StringType) {
        return TypeTree<std::string>{};
    } else {
        static_assert(false, "Unhandled typetree kind in typetree");
    }
}

template<const WitLexy::TypeKind head, const WitLexy::TypeKind... kinds> requires(sizeof...(kinds) > 0)
static constexpr auto typetree() {
    constexpr WitLexy::TypeKind kinds_array[] = {kinds...};
    constexpr auto size = sizeof...(kinds);

    if constexpr (head == WitLexy::TypeKind::ListStart) {
        auto element_type = [&]<std::size_t... Is>(std::index_sequence<Is...> is) consteval {
            return typetree<kinds_array[Is]...>();
        }(std::make_index_sequence<size-1>{});

        return TypeTree<std::vector<typename decltype(element_type)::Type>>();
    } else {
        static_assert(false, "Unhandled typetree kind in typetree");
    }
}

template<std::size_t ArgPos, auto tt, typename T>
struct CheckArgType {
    static constexpr void check() {
        auto expected_type = [&]<std::size_t... Is>(std::index_sequence<Is...> is) consteval {
            return typetree<tt[Is]...>();
        }(std::make_index_sequence<tt.size()>{});
        // auto expected_type = typetree<tt.size(), tt.data()>();
        decltype(expected_type)::template is_same<T>();
        //static_assert(, "Method argument type mismatch with dolphin.wit");
    }
};


// void printTypeV(auto& type) {
//     std::visit([](auto&& arg) constexpr {
//         using T = std::decay_t<decltype(arg)>;
//         if constexpr (std::is_same_v<T, WitLexy::TypeKind>) {
//             fmt::print("{} ", static_cast<int>(arg));
//         } else if constexpr (std::is_same_v<T, std::unique_ptr<WitLexy::ListType>>) {
//             fmt::print("ListType: ");
//             printTypeV(arg->element_type);
//         } else {
//             static_assert(false, "fmtTypeV");
//         }
//     }, type);
// }

static consteval auto parsed_wit() {
    static constexpr auto file = get_embedded_wit();
    static constexpr auto literal = lexy::string_input(file);
    return lexy::parse<WitLexy::witfile>(literal, lexy::callback<void>([](auto, auto) constexpr {}) );
}

static consteval size_t num_methods() {

    auto wit = parsed_wit();
    assert(wit.has_value());
    assert(wit.is_success());

    return wit.value().interfaces[0].resources[0].methods.size();
}

template<std::size_t N>
static consteval std::array<size_t, N> method_param_counts() {
    auto wit = parsed_wit();
    return [&]<std::size_t... Is>(std::index_sequence<Is...> is) constexpr {
        return std::array<size_t, N>{wit.value().interfaces[0].resources[0].methods[Is].type.num_params...};
    }(std::make_index_sequence<N>{});
}

//template<auto array>
static constexpr auto get_params() {

    static constexpr auto param_counts = method_param_counts<num_methods()>();


    static constexpr auto param_sizes = [&]<std::size_t... Ms>(std::index_sequence<Ms...> ) constexpr {
        auto wit = parsed_wit();

        auto per_method =[&]<std::size_t... Ps>(std::index_sequence<Ps...>, auto ms) consteval {
            auto params = wit.value().interfaces[0].resources[0].methods[ms()].type.params;
            return std::array<size_t, param_counts[ms()]>{params[Ps].type.type_tree.size()...};
        };

        return std::make_tuple(per_method(std::make_index_sequence<param_counts[Ms]>{}, std::integral_constant<size_t, Ms>{})... );
    }(std::make_index_sequence<param_counts.size()>{});

    return [&]<std::size_t... Ms>(std::index_sequence<Ms...>) constexpr {
        auto wit = parsed_wit();


        auto per_method = [&]<std::size_t... Ps>(std::index_sequence<Ps...> is, auto ms) constexpr {
            auto params = wit.value().interfaces[0].resources[0].methods[ms()].type.params;
            static constexpr auto sizes = std::get<ms()>(param_sizes);

            auto copy_typetree = [&]<size_t IIs, size_t... Js>(std::integral_constant<size_t, IIs>, std::index_sequence<Js...> js) consteval {
                return std::make_tuple(params[IIs].id, static_cast<WitLexy::TypeKind>(params[IIs].type.type_tree[Js])...);
            };

            return std::make_tuple(
                copy_typetree(
                    std::integral_constant<size_t, Ps>{},
                    std::make_index_sequence<sizes[Ps]>{}
                )
                ...
            );
        };
        return std::make_tuple(per_method(std::make_index_sequence<param_counts[Ms]>{}, std::integral_constant<size_t, Ms>{})...);
    }(std::make_index_sequence<param_counts.size()>{});
}

void Plugins::Init()
{
    // InitDiscoveryModule();
    // InitLoggingModule();
    // InitBasicGuiModule();
    // InitCPUModule();

    static constexpr auto file = get_embedded_wit();
    auto literal = lexy::string_input(file);
    auto result = lexy::validate<WitLexy::witfile>(literal, lexy_ext::report_error);
    fmt::print("Parsed dolphin.wit with lexy: {}\n", result.is_success());

    auto wit = lexy::parse<WitLexy::witfile>(literal, lexy_ext::report_error);
    fmt::print("{} {}\n", result.is_success(), wit.has_value());

    for (auto& method : wit.value().interfaces[0].resources[0].methods) {
        fmt::print("Method: {}\n", method.id);
        for (auto& arg : method.type.params) {
            fmt::print("  Arg: {} Type: ", arg.id);
            // printTypeV(arg.type);
            fmt::print("\n");
        }
    }

    auto check = [] constexpr -> std::string {

        static constexpr auto cpu_resource = [] consteval -> FixedLenResource {
            auto wit = parsed_wit();
            // static_assert(wit.is_success(), "Failed to parse dolphin.wit");

            FixedLenResource res;

            // if (!wit.is_success()) {
            //     return res;
            // }

            static constexpr auto method_params = get_params();
            using MethodParams = decltype(method_params);

            auto methods = [&]<std::size_t... Is>(std::index_sequence<Is...> is) constexpr {
                auto typefn = [&]<size_t IIS, std::size_t... Js>(std::index_sequence<Js...>, std::integral_constant<std::size_t, IIS>) constexpr {
                        static constexpr auto params = std::get<IIS>(method_params);
                        auto paramfn = [&]<size_t JJs, size_t... Ks>(std::integral_constant<std::size_t, JJs>,  std::index_sequence<Ks...>) constexpr {
                            static constexpr auto param = std::get<JJs>(params);
                            return typetree<std::get<Ks+1>(param)...>().named(std::get<0>(param));
                        };

                        return std::make_tuple(
                            paramfn(
                                std::integral_constant<std::size_t, Js>{},
                                std::make_index_sequence<
                                    std::tuple_size_v<
                                        std::tuple_element_t<Js, decltype(params)>
                                    > - 1
                                >{}
                            )...
                        );
                    };
                return std::make_tuple(
                    typefn(
                        std::make_index_sequence<std::tuple_size_v<std::tuple_element_t<Is, MethodParams>>>{},
                        std::integral_constant<std::size_t, Is>{}
                    )
                ...);


            }(std::make_index_sequence<std::tuple_size_v<MethodParams>>{});

            return res;
        }();

        // auto binder = [&](auto method) constexpr {
        //     using Traits = decltype(method)::Traits;

        //     static constexpr auto def = [&] consteval -> std::optional<FixedLenFunc> {
        //         auto view = cpu_resource.methods.view();
        //         auto def = std::ranges::find_if(view, [&](const auto& m) constexpr { return m.name.view() == method.binding_name(); });
        //         return def == view.end() ? std::nullopt : std::make_optional(*def);
        //     }();
        //     static_assert(def.has_value(), "Method not found in dolphin.wit");

        //     static_assert(def->args.size() == Traits::ArgCount, "Method argument count mismatch with dolphin.wit");

        //     [&]<std::size_t... Is>(std::index_sequence<Is...> is) constexpr {
        //         (CheckArgType<Is, ExtractTypeTree<def->args[Is].type.type_tree.size()>{}(def->args[Is].type.type_tree), std::tuple_element_t<Is, typename Traits::ArgTypes>>().check(), ...);
        //     }(std::make_index_sequence<def->args.size()>{});

        // };
        // CpuApi::CpuMemory::bindings(binder);

        return "";
    };

    // bool bindings_ok = check() == "";

    // if (!bindings_ok) {
    //     fmt::print(stderr, "Error: Failed to check bindings\n");
    // } else {
    //     fmt::print("Bindings check passed!\n");
    // }

    //auto items = parse_wit();

    fmt::print("Compiling module\n");

    auto src = R"(
        (component
            (type (;0;) (func (param "msg" string)))
            (type (;1;) (func))
            (import "hello" (func $hello (;0;) (type 0)))
            (import "hi" (func $hi (;1;) (type 1)))

            (core module $main
                (type (;0;) (func (param i32 i32)))
                (type (;1;) (func))
                (data $.rodata (;0;) (i32.const 1048576) "Hello from my-wasm-component!")
                (import "$root" "hello" (func $hello (;0;) (type 0)))
                (import "$root" "hi" (func $hi (;1;) (type 1)))
                (table (;0;) 3 3 funcref)
                (memory (;0;) 17)
                (export "memory" (memory 0))
                (export "run" (func $rrun))
                (func $rrun (type 1)
                    (;call $hi;)
                    i32.const 1048576
                    i32.const 29
                    call $hello
                )
            )
            (core module $wit-component-shim-module (;1;)
                (type (;0;) (func (param i32 i32)))
                (table (;0;) 1 1 funcref)
                (export "0" (func 0))
                (export "$imports" (table 0))
                (func (;0;) (type 0) (param i32 i32)
                    local.get 0
                    local.get 1
                    i32.const 0
                    call_indirect (type 0)
                )
            )
            (core module $wit-component-fixup (;2;)
                (type (;0;) (func (param i32 i32)))
                (import "" "0" (func (;0;) (type 0)))
                (import "" "$imports" (table (;0;) 1 1 funcref))
                (elem (;0;) (i32.const 0) func 0)
            )
            (core instance $wit-component-shim-instance (;0;) (instantiate $wit-component-shim-module))
            (core func $hi (;0;) (canon lower (func $hi)))
            (alias core export $wit-component-shim-instance "0" (core func $indirect-$root-hello (;0;)))
            (core instance $$root (;1;)
                (export "hi" (func $hi))
                (export "hello" (func $indirect-$root-hello))
            )
            (core instance $main (;2;) (instantiate $main
                    (with "$root" (instance $$root))
                )
            )
            (alias core export $main "memory" (core memory $memory (;0;)))

            (alias core export $wit-component-shim-instance "$imports" (core table $"shim table" (;0;)))
            (core func $"#core-func2 indirect-$root-hello" (@name "indirect-$root-hello") (;2;) (canon lower (func $hello) (memory $memory) string-encoding=utf8))
            (type (;1;) (func))

            (core instance $fixup-args (;3;)
                (export "$imports" (table $"shim table"))
                (export "0" (func $"#core-func2 indirect-$root-hello"))
            )
            (core instance $fixup (;4;) (instantiate $wit-component-fixup
                    (with "" (instance $fixup-args))
                )
            )

            (alias core export $main "run" (core func $run (;3;)))
            (func $run (;1;) (type 1) (canon lift (core func $run)))
            (export $"#func2 run" (@name "run") (;2;) "run" (func $run))
        )
    )";

    wasmtime::Engine engine;

    auto component = wasmtime::component::Component::compile(engine, src).unwrap();

    fmt::print("Initializing...\n");

    wasmtime::Store store(engine);

    wasmtime::component::Linker linker(engine);

    linker.root().add_func("hello", &hello_wrapper).unwrap();
    linker.root().add_func("hi", &hi).unwrap();

    fmt::print("Instantiating module...\n");

    auto instance = linker.instantiate(store, component).unwrap();

    fmt::print("Calling module...\n");

    if (auto index = instance.get_export_index(store, nullptr, "run")) {
        if (auto run = instance.get_func(store, *index)) {
            run->call(store, {}, {}).unwrap();
        } else {
            fmt::print("get_func failed\n");
        }
    } else {
        fmt::print("failed to find run function\n");
    }



    fmt::print("done!\n");

    std::exit(0);
}

std::vector<Plugins::PluginFiles> Plugins::GetAllPlugins()
{
    auto PluginDir = File::GetUserPath(D_LOAD_IDX) + "Plugins";

    fmt::print("Scanning for plugins in {}\n", PluginDir);

    const File::FSTEntry entries = File::ScanDirectoryTree(PluginDir, true);

    for(const auto& entry : entries.children)
    {
        PluginFiles file = {entry.physicalName, entry.virtualName, false};
        if(!AlreadyAdded(file)) {
            fmt::print("Found {}, {}\n", file.physicalName, file.virtualName);
            plugin_entries.push_back(file);
        }
    }

    return plugin_entries;
}

bool Plugins::AlreadyAdded(Plugins::PluginFiles plugin)
{
    for(PluginFiles& file : plugin_entries)
    {
        if(file.virtualName == plugin.virtualName)
            return true;
    }

    return false;
}

void Plugins::LoadPlugin(u32 id)
{
        fmt::print("Loading plugin {}\n", plugin_entries.at(id).physicalName);
        // Attempt to load the .so file
        void* handle = dlopen(plugin_entries.at(id).physicalName.c_str(), RTLD_NOW);

        if (!handle)
        {
            fmt::print("dlopen of {} failed: {}\n", plugin_entries.at(id).physicalName, dlerror());
            return;
        }



        // Locate the plugin_init function
        plugin_entries.at(id).plugin_init = reinterpret_cast<void (*)(void*)>(dlsym(handle, "plugin_init"));
        plugin_entries.at(id).plugin_requestShutdown = reinterpret_cast<void (*)(uint64_t)>(dlsym(handle, "plugin_requestShutdown"));

        if (!plugin_entries.at(id).plugin_init)
        {
            fmt::print("{} did not contain plugin_init function: {}\n", plugin_entries.at(id).physicalName, dlerror());
            return;
        }

        if (!plugin_entries.at(id).plugin_requestShutdown)
        {
            fmt::print("{} did not contain plugin_requestShutdown function: {}\n", plugin_entries.at(id).physicalName, dlerror());
            return;
        }

        plugin_entries.at(id).Active = true;
        plugin_entries.at(id).ModuleId = Module_id++;

        plugin_entries.at(id).FnPtrFunctor = GetFnPtrFunctor(id);

        // Actually call the plugin_init method
        plugin_entries.at(id).plugin_init(plugin_entries.at(id).FnPtrFunctor);
}

void Plugins::ShutdownPlugin(u32 id)
{
    fmt::print("Requesting shutdown of {}\n", plugin_entries.at(id).virtualName);
    plugin_entries.at(id).Active = false;
    plugin_entries.at(id).plugin_requestShutdown(Module_id);
}
