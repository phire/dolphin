// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "Common/MemoryUtil.h"
#include "Common/x64ABI.h"
#include "Common/x64Emitter.h"

#include "VideoBackends/Null/VertexManager.h"

#include "VideoCommon/CPMemory.h"
#include "VideoCommon/NativeVertexFormat.h"
#include "VideoCommon/VertexShaderGen.h"

// Here's some global state. We only use this to keep track of what we've sent to the OpenGL state
// machine.

namespace NullVideo
{

NativeVertexFormat* VertexManager::CreateNativeVertexFormat()
{
	return new GLVertexFormat();
}

GLVertexFormat::GLVertexFormat()
{

}

GLVertexFormat::~GLVertexFormat()
{

}


void GLVertexFormat::Initialize(const PortableVertexDeclaration &_vtx_decl)
{
}

void GLVertexFormat::SetupVertexPointers() {
}

}
