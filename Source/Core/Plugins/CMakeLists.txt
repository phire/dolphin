add_library(plugin_abi INTERFACE)
target_include_directories(plugin_abi INTERFACE PluginABI/)

add_library(plugin_cpp INTERFACE)
target_include_directories(plugin_cpp INTERFACE PluginCpp/)

add_library(plugin_binding
    Binding/Binding.cpp
    Binding/Binding.h
)
target_include_directories(plugin_binding PUBLIC Binding/)

# embed dolphin.wit
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dolphin_wit.h
    COMMAND ${CMAKE_COMMAND} -E bin2c
        ${CMAKE_SOURCE_DIR}/wit/dolphin.wit
        ${CMAKE_CURRENT_BINARY_DIR}/dolphin_wit.h
    DEPENDS ${CMAKE_SOURCE_DIR}/wit/dolphin.wit
)

add_library(plugins
    BasicGuiModule.cpp
    CpuModule.cpp
    DiscoveryModule.cpp
    ModuleManager.cpp
    LoggingModule.cpp
    PluginHost.cpp
    WitParser.cpp
    dolphin_wit.h
    WitParserImpl.h
)

target_include_directories(plugins
PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/wit
)

target_link_libraries(plugins
PRIVATE
    plugin_binding
    plugin_cpp
    plugin_abi
    wasmtime
    fmt::fmt
)
