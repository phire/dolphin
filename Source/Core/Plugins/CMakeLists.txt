add_library(plugin_abi INTERFACE)
target_include_directories(plugin_abi INTERFACE PluginABI/)

add_library(plugin_cpp INTERFACE)
target_include_directories(plugin_cpp INTERFACE PluginCpp/)

add_library(plugins
    DiscoveryModule.cpp
    ModuleManager.cpp
    LoggingModule.cpp
    PluginHost.cpp
)

target_link_libraries(plugins
PRIVATE
    plugin_abi
    fmt::fmt
)

