
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 COMPONENTS Quick Gui REQUIRED)

qt_add_library(dolphin-quick STATIC
    PlatformQt.h
    PlatformQt.cpp
    QtGLContext.cpp
)

# qt_add_qml_module(dolphin-quick
#     URI "DolphinQuick"
#   #  STATIC
#     VERSION 1.0
#     QML_FILES
#         main.qml
#     SOURCES
#         Emulator.h
#         QuickGui.cpp
#         DolphinItem.h
#         DolphinItem.cpp
#         AbstractTextureNode.h
#         AbstractTextureNode.cpp
# )

target_link_libraries(dolphin-quick PRIVATE Qt6::Gui Qt6::Quick)

# we need access to Qt6's private includes
target_include_directories(dolphin-quick PRIVATE
  ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
)

#target_link_libraries(dolphin-quick PRIVATE dolphin-quickplugin)

target_compile_definitions(dolphin-quick PUBLIC QT_PLATFORM=1)
