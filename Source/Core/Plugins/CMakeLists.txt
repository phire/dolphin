add_library(plugin_abi INTERFACE)
target_include_directories(plugin_abi INTERFACE PluginABI/)

add_library(plugin_cpp INTERFACE)
target_include_directories(plugin_cpp INTERFACE PluginCpp/)

add_library(plugin_binding
    Binding/Binding.cpp
    Binding/Binding.h
)
target_include_directories(plugin_binding PUBLIC Binding/)

add_library(plugins
    BasicGuiModule.cpp
    CpuModule.cpp
    DiscoveryModule.cpp
    ModuleManager.cpp
    LoggingModule.cpp
    PluginHost.cpp

)

target_link_libraries(plugins
PRIVATE
    plugin_binding
    plugin_cpp
    plugin_abi
    wasmtime
    fmt::fmt
)
