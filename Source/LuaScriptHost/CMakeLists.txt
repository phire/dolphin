
add_library(LuaScriptHost SHARED
    plugin_main.cpp
    test_discovery.cpp

    type_adaptor.cpp
    function_adaptor.cpp
    callback_adaptor.cpp
    class_adaptor.cpp
)

target_link_libraries(LuaScriptHost PRIVATE lua plugin_abi)

add_custom_command(TARGET LuaScriptHost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:LuaScriptHost> ${PROJECT_BINARY_DIR}/Binaries/Plugins/$<TARGET_FILE_NAME:LuaScriptHost>
)
