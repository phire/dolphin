add_library(plugins
    PluginHost.cpp
)

add_library(plugin_abi INTERFACE
    PluginABI/plugin.h
)
target_include_directories(plugin_abi INTERFACE PluginABI/)
