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

struct FixNamedType {
    FixedStr name  = {};
    Wit::Ty::Kind kind = Wit::Ty::Kind::Ignore;
    Wit::Ty ty = {};

    constexpr FixNamedType() = default;
    constexpr FixNamedType(const Wit::NamedType& other) : name(other.name), kind(other.type.kind), ty(other.type) {}
};

struct FixedLenFunc {
    bool async = false;
    bool static_ = false;
    bool constructor = false;
    FixedStr name = {};
    std::array<FixNamedType, 20> args;
    size_t arg_count = 0;
    // std::array<Wit::Ty, 4> results;
};

struct FixedLenResource {
    FixedStr name;
    std::array<FixedLenFunc, 20> methods;
};

template<std::size_t ArgPos, Wit::Ty::Kind kind, typename T>
struct CheckArgType {
    static constexpr void check() {
        static_assert(Wit::Ty::is_convertible_to<T>(kind), "Method argument type mismatch with dolphin.wit");
    }
};

void Plugins::Init()
{
    // InitDiscoveryModule();
    // InitLoggingModule();
    // InitBasicGuiModule();
    // InitCPUModule();

    auto check = [] constexpr -> std::string {

        static constexpr auto cpu_resource = [] constexpr -> FixedLenResource {
            auto items = parse_wit();
            auto emu_interface = std::find_if(items.begin(), items.end(), [](const auto& item) constexpr {
                return std::holds_alternative<Wit::Interface>(item) && std::get<Wit::Interface>(item).name == "emu";
            });
            // if (emu_interface == items.end()) {
            //     return "Error: emu interface not found in dolphin.wit\n";
            // }
            auto emu_items = std::get<Wit::Interface>(*emu_interface).items;
            auto cpu_resource_v = std::ranges::find_if(emu_items, [](const auto& item) constexpr {
                return std::holds_alternative<Wit::Resource>(item) && std::get<Wit::Resource>(item).name == "cpu";
            });
            // if (cpu_resource_v == emu_items.end()) {
            //     return "Error: cpu resource not found in emu interface\n";
            // }

            FixedLenResource res;
            size_t i = 0;
            auto cpu_resource = std::get<Wit::Resource>(*cpu_resource_v);
            res.name = FixedStr(cpu_resource.name);
            for (const auto& method : cpu_resource.methods) {
                auto& m = res.methods[i++];
                m.name = FixedStr(method.name);
                m.async = method.ty.async;
                m.static_ = method.ty.static_;
                m.constructor = method.ty.constructor;
                m.arg_count = method.ty.params.size();
                size_t j = 0;
                for (const auto& arg : method.ty.params) {
                    m.args[j++] = FixNamedType(arg);
                    if (j >= m.args.size()) {
                        break;
                    }
                }
                j = 0;
                // for (const auto& result : method.ty.results) {
                //     m.results[j++] = result;
                //     if (j >= m.results.size()) {
                //         break;
                //     }
                // }
                if (i >= res.methods.size()) {
                    break;
                }
            }

            return res;
        }();

        auto binder = [&](auto method) constexpr {
            using Traits = decltype(method)::Traits;

            static constexpr auto def = [&] consteval -> std::optional<FixedLenFunc> {
                auto def = std::ranges::find_if(cpu_resource.methods, [&](const auto& m) constexpr { return m.name.view() == method.binding_name(); });
                return def == cpu_resource.methods.end() ? std::nullopt : std::make_optional(*def);
            }();
            static_assert(def.has_value(), "Method not found in dolphin.wit");

            static_assert(def->arg_count == Traits::ArgCount, "Method argument count mismatch with dolphin.wit");

            [&]<std::size_t... Is>(std::index_sequence<Is...> is) constexpr {
                static constexpr auto kinds = std::make_tuple(def->args[Is].kind...);
                (CheckArgType<Is, std::get<Is>(kinds), std::tuple_element_t<Is, typename Traits::ArgTypes>>().check(), ...);
            }(std::make_index_sequence<def->arg_count>{});

        };
        CpuApi::CpuMemory::bindings(binder);

        return "";
    };

    bool bindings_ok = check() == "";

    if (!bindings_ok) {
        fmt::print(stderr, "Error: Failed to check bindings\n");
    } else {
        fmt::print("Bindings check passed!\n");
    }

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
