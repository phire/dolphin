// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "VideoCommon/CPMemory.h"
#include "VideoCommon/NativeVertexFormat.h"
#include "VideoCommon/VertexManagerBase.h"\

namespace NullVideo
{
	class GLVertexFormat : public NativeVertexFormat
	{
		PortableVertexDeclaration vtx_decl;

	public:
		virtual void Initialize(const PortableVertexDeclaration &_vtx_decl) override {}
		virtual void SetupVertexPointers() override {}
	};

// Handles the OpenGL details of drawing lots of vertices quickly.
// Other functionality is moving out.
class VertexManager : public ::VertexManager
{
public:
	VertexManager();
	~VertexManager();
	NativeVertexFormat* CreateNativeVertexFormat() override {
		return new GLVertexFormat();
	}
	void CreateDeviceObjects() override;
	void DestroyDeviceObjects() override;

protected:
	virtual void ResetBuffer(u32 stride) override;
private:
	void Draw(u32 stride);
	void vFlush(bool useDstAlpha) override;
	void PrepareDrawBuffers(u32 stride);
};

}
