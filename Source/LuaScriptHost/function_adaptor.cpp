// Copyright 2021 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "function_adaptor.h"

// This is a workaround for the fact that c++ doesn't provide a sane way to call
// a function ptr with an unknown number of arguments.
static void* CallFn0(void*(*fn)(void), ArgHolder *args) {
    return fn();
}
static void* CallFn1(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*)>(fn);
    return fn_args(args[0].val);
}
static void* CallFn2(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val);
}
static void* CallFn3(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val);
}
static void* CallFn4(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val);
}
static void* CallFn5(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val);
}
static void* CallFn6(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val);
}
static void* CallFn7(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val);
}
static void* CallFn8(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val);
}
static void* CallFn9(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val);
}
static void* CallFn10(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val);
}
static void* CallFn11(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val);
}
static void* CallFn12(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val);
}
static void* CallFn13(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val);
}
static void* CallFn14(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val, args[13].val);
}
static void* CallFn15(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val, args[13].val, args[14].val);
}
static void* CallFn16(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val, args[13].val, args[14].val, args[15].val);
}
static void* CallFn17(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val, args[13].val, args[14].val, args[15].val, args[16].val);
}
static void* CallFn18(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val, args[13].val, args[14].val, args[15].val, args[16].val, args[17].val);
}
static void* CallFn19(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val, args[13].val, args[14].val, args[15].val, args[16].val, args[17].val, args[18].val);
}
static void* CallFn20(void*(*fn)(void), ArgHolder *args) {
    auto fn_args = reinterpret_cast<void* (*)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*)>(fn);
    return fn_args(args[0].val, args[1].val, args[2].val, args[3].val, args[4].val, args[5].val, args[6].val, args[7].val, args[8].val, args[9].val, args[10].val, args[11].val, args[12].val, args[13].val, args[14].val, args[15].val, args[16].val, args[17].val, args[18].val, args[19].val);
}

std::array<void*(*)(void*(*)(void), ArgHolder *), 21> CallWithArgs = {
    CallFn0, CallFn1, CallFn2, CallFn3, CallFn4, CallFn5,
    CallFn6,  CallFn7,  CallFn8,  CallFn9,  CallFn10,
    CallFn11, CallFn12, CallFn13, CallFn14, CallFn15,
    CallFn16, CallFn17, CallFn18, CallFn19, CallFn20,
};