add_library(plugin_abi INTERFACE)
target_include_directories(plugin_abi INTERFACE PluginABI/)

add_library(plugin_cpp INTERFACE)
target_include_directories(plugin_cpp INTERFACE PluginCpp/)

add_library(plugin_binding
    Binding/Binding.cpp
    Binding/Binding.h
)
target_include_directories(plugin_binding PUBLIC Binding/)

set(DOLPHIN_WIT ${CMAKE_SOURCE_DIR}/wit/dolphin.wit)

# embed dolphin.wit
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dolphin_wit.h
    COMMAND ${CMAKE_COMMAND} -E bin2c
        ${DOLPHIN_WIT}
        ${CMAKE_CURRENT_BINARY_DIR}/dolphin_wit.h
    DEPENDS ${DOLPHIN_WIT}
)

add_library(bindingGen STATIC
    BindingGen.cpp
    BindingGen.h
    DolphinBindings.h
)

target_include_directories(bindingGen
SYSTEM PUBLIC
    # We only need wasmtime's headers for this.
    "$<TARGET_PROPERTY:wasmtime,INTERFACE_INCLUDE_DIRECTORIES>"
PRIVATE
    // Pull in any include directories we might need when generating bindings
    "$<TARGET_PROPERTY:common,INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:core,INCLUDE_DIRECTORIES>"
)

add_library(bindingCheck STATIC
    BindingCheck.cpp
    BindingCheck.h
    ${CMAKE_CURRENT_BINARY_DIR}/dolphin_wit.h
)

target_compile_definitions(bindingCheck
    PRIVATE
        "DOLPHIN_WIT_PATH=\"${DOLPHIN_WIT}\""
)

target_include_directories(bindingCheck
PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/wit
)

target_link_libraries(bindingCheck
PUBLIC
    plugin_binding
PRIVATE
    bindingGen
    fmt::fmt
    lexy
    lexy_ext
)

add_executable(bindingCheckTool
    BindingCheckTool.cpp
)

target_link_libraries(bindingCheckTool
PRIVATE
    fmt::fmt
    bindingCheck
)

add_custom_command(
    COMMENT "Checking exported bindings against dolphin.wit"
    OUTPUT binding_check_stamp.h
    COMMAND bindingCheckTool
    DEPENDS bindingCheckTool
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
    ${CMAKE_CURRENT_BINARY_DIR}/binding_check_stamp.h
)

target_include_directories(plugins
PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/wit
)

target_link_libraries(plugins
PRIVATE
    bindingGen
    plugin_binding
    plugin_cpp
    plugin_abi
    wasmtime
    fmt::fmt
    lexy
    lexy_ext
)
