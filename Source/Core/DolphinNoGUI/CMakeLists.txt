add_executable(dolphin-nogui
  Platform.cpp
  Platform.h
  PlatformHeadless.cpp
  MainNoGUI.cpp
)

if(ENABLE_X11 AND X11_FOUND)
  target_sources(dolphin-nogui PRIVATE PlatformX11.cpp)
endif()

if(WIN32)
  target_sources(dolphin-nogui PRIVATE PlatformWin32.cpp)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  pkg_check_modules(DRM REQUIRED libdrm)
  target_compile_options(dolphin-nogui PRIVATE ${DRM_CFLAGS})

  pkg_check_modules(GBM REQUIRED gbm)

  target_link_options(dolphin-nogui PRIVATE ${DRM_LDFLAGS} ${GBM_LDFLAGS})

  target_sources(dolphin-nogui PRIVATE PlatformFBDev.cpp PlatformKMS.cpp)
endif()

set_target_properties(dolphin-nogui PROPERTIES OUTPUT_NAME dolphin-emu-nogui)

target_link_libraries(dolphin-nogui
PRIVATE
  core
  uicommon
  cpp-optparse
)

if(MSVC)
  # Add precompiled header
  target_link_libraries(dolphin-nogui PRIVATE use_pch)
endif()

if(USE_DISCORD_PRESENCE)
  target_compile_definitions(dolphin-nogui PRIVATE -DUSE_DISCORD_PRESENCE)
endif()

set(CPACK_PACKAGE_EXECUTABLES ${CPACK_PACKAGE_EXECUTABLES} dolphin-nogui)
install(TARGETS dolphin-nogui RUNTIME DESTINATION ${bindir})

