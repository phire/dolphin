// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <string>

#include "DolphinWX/GLInterface/GLInterface.h"

#include "VideoBackends/Null/GLExtensions/ARB_blend_func_extended.h"
#include "VideoBackends/Null/GLExtensions/ARB_buffer_storage.h"
#include "VideoBackends/Null/GLExtensions/ARB_debug_output.h"
#include "VideoBackends/Null/GLExtensions/ARB_draw_elements_base_vertex.h"
#include "VideoBackends/Null/GLExtensions/ARB_ES2_compatibility.h"
#include "VideoBackends/Null/GLExtensions/ARB_framebuffer_object.h"
#include "VideoBackends/Null/GLExtensions/ARB_get_program_binary.h"
#include "VideoBackends/Null/GLExtensions/ARB_map_buffer_range.h"
#include "VideoBackends/Null/GLExtensions/ARB_sample_shading.h"
#include "VideoBackends/Null/GLExtensions/ARB_sampler_objects.h"
#include "VideoBackends/Null/GLExtensions/ARB_sync.h"
#include "VideoBackends/Null/GLExtensions/ARB_texture_multisample.h"
#include "VideoBackends/Null/GLExtensions/ARB_uniform_buffer_object.h"
#include "VideoBackends/Null/GLExtensions/ARB_vertex_array_object.h"
#include "VideoBackends/Null/GLExtensions/ARB_viewport_array.h"
#include "VideoBackends/Null/GLExtensions/gl_1_1.h"
#include "VideoBackends/Null/GLExtensions/gl_1_2.h"
#include "VideoBackends/Null/GLExtensions/gl_1_3.h"
#include "VideoBackends/Null/GLExtensions/gl_1_4.h"
#include "VideoBackends/Null/GLExtensions/gl_1_5.h"
#include "VideoBackends/Null/GLExtensions/gl_2_0.h"
#include "VideoBackends/Null/GLExtensions/gl_3_0.h"
#include "VideoBackends/Null/GLExtensions/gl_3_1.h"
#include "VideoBackends/Null/GLExtensions/gl_3_2.h"
#include "VideoBackends/Null/GLExtensions/KHR_debug.h"
#include "VideoBackends/Null/GLExtensions/NV_primitive_restart.h"

namespace GLExtensions
{
	// Initializes the interface
	bool Init();

	// Function for checking if the hardware supports an extension
	// example: if (GLExtensions::Supports("GL_ARB_multi_map"))
	bool Supports(const std::string& name);

	// Returns OpenGL version in format 430
	u32 Version();
}
