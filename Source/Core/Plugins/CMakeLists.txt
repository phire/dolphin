add_library(plugin_abi INTERFACE)
target_include_directories(plugin_abi INTERFACE PluginABI/)

add_library(plugins
    PluginHost.cpp
)

target_link_libraries(plugins
PRIVATE
    plugin_abi
    fmt::fmt
)

