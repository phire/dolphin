project(lua C)
include_directories (src)

set(LUA_LIB_SRCS
    "src/lapi.c"
    "src/lcode.c"
    "src/lctype.c"
    "src/ldebug.c"
    "src/ldo.c"
    "src/ldump.c"
    "src/lfunc.c"
    "src/lgc.c"
    "src/llex.c"
    "src/lmem.c"
    "src/lobject.c"
    "src/lopcodes.c"
    "src/lparser.c"
    "src/lstate.c"
    "src/lstring.c"
    "src/ltable.c"
    "src/ltm.c"
    "src/lundump.c"
    "src/lvm.c"
    "src/lzio.c"
    "src/lauxlib.c"
    "src/lbaselib.c"
    "src/lcorolib.c"
    "src/ldblib.c"
    "src/liolib.c"
    "src/lmathlib.c"
    "src/loadlib.c"
    "src/loslib.c"
    "src/lstrlib.c"
    "src/ltablib.c"
    "src/lutf8lib.c"
    "src/linit.c"
)

# Compile as c++
set_source_files_properties(${LUA_LIB_SRCS} PROPERTIES LANGUAGE CXX )

add_library (lua STATIC ${LUA_LIB_SRCS} )

# But don't use c++ exceptions
target_compile_definitions(lua PRIVATE LUA_USE_LONGJMP)

if (UNIX)
    target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
endif()

target_include_directories(lua PUBLIC "include")

# position independant code so we can link this into a dll/so
set_property(TARGET lua PROPERTY POSITION_INDEPENDENT_CODE ON)

