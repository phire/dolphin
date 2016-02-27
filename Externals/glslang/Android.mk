# Basic Android.mk for glslang in PPSSPP

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libglslang
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := \
    glslang/GenericCodeGen/CodeGen.cpp \
    glslang/GenericCodeGen/Link.cpp \
    glslang/MachineIndependent/Constant.cpp \
    glslang/MachineIndependent/glslang_tab.cpp \
    glslang/MachineIndependent/InfoSink.cpp \
    glslang/MachineIndependent/Initialize.cpp \
    glslang/MachineIndependent/Intermediate.cpp \
    glslang/MachineIndependent/intermOut.cpp \
    glslang/MachineIndependent/IntermTraverse.cpp \
    glslang/MachineIndependent/limits.cpp \
    glslang/MachineIndependent/linkValidate.cpp \
    glslang/MachineIndependent/parseConst.cpp \
    glslang/MachineIndependent/ParseHelper.cpp \
    glslang/MachineIndependent/PoolAlloc.cpp \
    glslang/MachineIndependent/reflection.cpp \
    glslang/MachineIndependent/RemoveTree.cpp \
    glslang/MachineIndependent/Scan.cpp \
    glslang/MachineIndependent/ShaderLang.cpp \
    glslang/MachineIndependent/SymbolTable.cpp \
    glslang/MachineIndependent/Versions.cpp \
    glslang/MachineIndependent/preprocessor/Pp.cpp \
    glslang/MachineIndependent/preprocessor/PpAtom.cpp \
    glslang/MachineIndependent/preprocessor/PpContext.cpp \
    glslang/MachineIndependent/preprocessor/PpMemory.cpp \
    glslang/MachineIndependent/preprocessor/PpScanner.cpp \
    glslang/MachineIndependent/preprocessor/PpSymbols.cpp \
    glslang/MachineIndependent/preprocessor/PpTokens.cpp \
    glslang/OSDependent/Unix/ossource.cpp \
    SPIRV/disassemble.cpp \
    SPIRV/doc.cpp \
    SPIRV/GlslangToSpv.cpp \
    SPIRV/InReadableOrder.cpp \
    SPIRV/SpvBuilder.cpp \
    SPIRV/SPVRemapper.cpp \
    OGLCompilersDLL/InitializeDll.cpp

LOCAL_CFLAGS := -O3 -fsigned-char -fno-strict-aliasing -Wall -Wno-multichar -D__STDC_CONSTANT_MACROS
LOCAL_CPPFLAGS := -fno-exceptions -std=gnu++11 -fno-rtti -Wno-reorder
LOCAL_C_INCLUDES := $(LOCAL_PATH)/ext $(LOCAL_PATH)/ext/libzip ..

#Portable native and separate code on android in future is easy you needs add files 
#by ($(target_arch_ABI),arquitecture (armeabi-v7a , armeabi , x86 , MIPS)
# ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(findstring armeabi-v7a,$(TARGET_ARCH_ABI)),armeabi-v7a)
LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DARM -DARMEABI_V7A
else ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DARM -DARMEABI -march=armv6
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_CFLAGS := $(LOCAL_CFLAGS) -D_ARCH_64 -DARM64
else ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS := $(LOCAL_CFLAGS) -D_M_IX86
else ifeq ($(TARGET_ARCH_ABI),x86_64)
LOCAL_CFLAGS := $(LOCAL_CFLAGS) -D_M_X64
endif

include $(BUILD_STATIC_LIBRARY)
