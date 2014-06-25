// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "Common/Atomic.h"
#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Common/StringUtil.h"
#include "Common/Thread.h"
#include "Common/Timer.h"

#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/Movie.h"

#include "VideoBackends/Null/FramebufferManager.h"
#include "VideoBackends/Null/GLUtil.h"
#include "VideoBackends/Null/main.h"
#include "VideoBackends/Null/ProgramShaderCache.h"
#include "VideoBackends/Null/RasterFont.h"
#include "VideoBackends/Null/Render.h"
#include "VideoBackends/Null/StreamBuffer.h"
#include "VideoBackends/Null/TextureCache.h"
#include "VideoBackends/Null/VertexManager.h"

#include "VideoCommon/BPFunctions.h"
#include "VideoCommon/BPStructs.h"
#include "VideoCommon/DriverDetails.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/FPSCounter.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/PixelEngine.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VertexLoader.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexShaderGen.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VideoConfig.h"

#if defined(HAVE_WX) && HAVE_WX
#include "DolphinWX/WxUtils.h"
#endif

#ifdef _WIN32
#include <mmsystem.h>
#endif

#ifdef _WIN32
#endif
#if defined _WIN32 || defined HAVE_LIBAV
#include "VideoCommon/AVIDump.h"
#endif

extern int OSDInternalW, OSDInternalH;

namespace NullVideo
{

enum MultisampleMode {
	MULTISAMPLE_OFF,
	MULTISAMPLE_2X,
	MULTISAMPLE_4X,
	MULTISAMPLE_8X,
	MULTISAMPLE_SSAA_4X,
};


VideoConfig g_ogl_config;

// Declarations and definitions
// ----------------------------
static int s_fps = 0;
static GLuint s_ShowEFBCopyRegions_VBO = 0;
static GLuint s_ShowEFBCopyRegions_VAO = 0;
static SHADER s_ShowEFBCopyRegions;

static RasterFont* s_pfont = nullptr;

// 1 for no MSAA. Use s_MSAASamples > 1 to check for MSAA.
static int s_MSAASamples = 1;
static int s_LastMultisampleMode = 0;

static u32 s_blendMode;

static bool s_vsync;

#if defined(HAVE_WX) && HAVE_WX
static std::thread scrshotThread;
#endif

int GetNumMSAASamples(int MSAAMode)
{
	int samples;
	switch (MSAAMode)
	{
		case MULTISAMPLE_OFF:
			samples = 1;
			break;

		case MULTISAMPLE_2X:
			samples = 2;
			break;

		case MULTISAMPLE_4X:
		case MULTISAMPLE_SSAA_4X:
			samples = 4;
			break;

		case MULTISAMPLE_8X:
			samples = 8;
			break;

		default:
			samples = 1;
	}

	if (samples <= g_ogl_config.max_samples) return samples;

	// TODO: move this to InitBackendInfo
	OSD::AddMessage(StringFromFormat("%d Anti Aliasing samples selected, but only %d supported by your GPU.", samples, g_ogl_config.max_samples), 10000);
	return g_ogl_config.max_samples;
}

void ApplySSAASettings() {
	// GLES3 doesn't support SSAA
	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
	{
		if (g_ActiveConfig.iMultisampleMode == MULTISAMPLE_SSAA_4X) {
			if (g_ogl_config.bSupportSampleShading) {
				glEnable(GL_SAMPLE_SHADING_ARB);
				glMinSampleShadingARB(s_MSAASamples);
			} else {
				// TODO: move this to InitBackendInfo
				OSD::AddMessage("SSAA Anti Aliasing isn't supported by your GPU.", 10000);
			}
		} else if (g_ogl_config.bSupportSampleShading) {
			glDisable(GL_SAMPLE_SHADING_ARB);
		}
	}
}

void GLAPIENTRY ErrorCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	const char *s_source;
	const char *s_type;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API_ARB:             s_source = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   s_source = "Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: s_source = "Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     s_source = "Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB:     s_source = "Application"; break;
		case GL_DEBUG_SOURCE_OTHER_ARB:           s_source = "Other"; break;
		default:                                  s_source = "Unknown"; break;
	}
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR_ARB:               s_type = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: s_type = "Deprecated"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  s_type = "Undefined"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:         s_type = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:         s_type = "Performance"; break;
		case GL_DEBUG_TYPE_OTHER_ARB:               s_type = "Other"; break;
		default:                                    s_type = "Unknown"; break;
	}
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB:   ERROR_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB: WARN_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
		case GL_DEBUG_SEVERITY_LOW_ARB:    WARN_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
		default:                           ERROR_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
	}
}

// Two small Fallbacks to avoid GL_ARB_ES2_compatibility
void GLAPIENTRY DepthRangef(GLfloat neardepth, GLfloat fardepth)
{
	glDepthRange(neardepth, fardepth);
}
void GLAPIENTRY ClearDepthf(GLfloat depthval)
{
	glClearDepth(depthval);
}

void InitDriverInfo()
{
	std::string svendor = std::string(g_ogl_config.gl_vendor);
	std::string srenderer = std::string(g_ogl_config.gl_renderer);
	std::string sversion = std::string(g_ogl_config.gl_version);
	DriverDetails::Vendor vendor = DriverDetails::VENDOR_UNKNOWN;
	DriverDetails::Driver driver = DriverDetails::DRIVER_UNKNOWN;
	double version = 0.0;
	u32 family = 0;

	// Get the vendor first
	if (svendor == "NVIDIA Corporation" && srenderer != "NVIDIA Tegra")
		vendor = DriverDetails::VENDOR_NVIDIA;
	else if (svendor == "ATI Technologies Inc." || svendor == "Advanced Micro Devices, Inc.")
		vendor = DriverDetails::VENDOR_ATI;
	else if (std::string::npos != sversion.find("Mesa"))
		vendor = DriverDetails::VENDOR_MESA;
	else if (std::string::npos != svendor.find("Intel"))
		vendor = DriverDetails::VENDOR_INTEL;
	else if (svendor == "ARM")
		vendor = DriverDetails::VENDOR_ARM;
	else if (svendor == "http://limadriver.org/")
	{
		vendor = DriverDetails::VENDOR_ARM;
		driver = DriverDetails::DRIVER_LIMA;
	}
	else if (svendor == "Qualcomm")
		vendor = DriverDetails::VENDOR_QUALCOMM;
	else if (svendor == "Imagination Technologies")
		vendor = DriverDetails::VENDOR_IMGTEC;
	else if (svendor == "NVIDIA Corporation" && srenderer == "NVIDIA Tegra")
		vendor = DriverDetails::VENDOR_TEGRA;
	else if (svendor == "Vivante Corporation")
		vendor = DriverDetails::VENDOR_VIVANTE;

	// Get device family and driver version...if we care about it
	switch (vendor)
	{
		case DriverDetails::VENDOR_QUALCOMM:
		{
			if (std::string::npos != srenderer.find("Adreno (TM) 3"))
				driver = DriverDetails::DRIVER_QUALCOMM_3XX;
			else
				driver = DriverDetails::DRIVER_QUALCOMM_2XX;
			double glVersion;
			sscanf(g_ogl_config.gl_version, "OpenGL ES %lg V@%lg", &glVersion, &version);
		}
		break;
		case DriverDetails::VENDOR_ARM:
			// Currently the Mali-T line has two families in it.
			// Mali-T6xx and Mali-T7xx
			// These two families are similar enough that they share bugs in their drivers.
			if (std::string::npos != srenderer.find("Mali-T"))
			{
				driver = DriverDetails::DRIVER_ARM_MIDGARD;
				// Mali drivers provide no way to explicitly find out what video driver is running.
				// This is similar to how we can't find the Nvidia driver version in Windows.
				// Good thing is that ARM introduces a new video driver about once every two years so we can
				// find the driver version by the features it exposes.
				// r2p0 - No OpenGL ES 3.0 support (We don't support this)
				// r3p0 - OpenGL ES 3.0 support
				// r4p0 - Supports 'GL_EXT_shader_pixel_local_storage' extension.

				if (GLExtensions::Supports("GL_EXT_shader_pixel_local_storage"))
					version = 400;
				else
					version = 300;
			}
			else if (std::string::npos != srenderer.find("Mali-4") ||
			         std::string::npos != srenderer.find("Mali-3") ||
			         std::string::npos != srenderer.find("Mali-2"))
			{
				driver = DriverDetails::DRIVER_ARM_UTGARD;
			}
		break;
		case DriverDetails::VENDOR_MESA:
		{
			if (svendor == "nouveau")
				driver = DriverDetails::DRIVER_NOUVEAU;
			else if (svendor == "Intel Open Source Technology Center")
				driver = DriverDetails::DRIVER_I965;
			else if (std::string::npos != srenderer.find("AMD") || std::string::npos != srenderer.find("ATI"))
				driver = DriverDetails::DRIVER_R600;

			int major = 0;
			int minor = 0;
			int release = 0;
			sscanf(g_ogl_config.gl_version, "%*s Mesa %d.%d.%d", &major, &minor, &release);
			version = 100*major + 10*minor + release;
		}
		break;
		case DriverDetails::VENDOR_INTEL: // Happens in OS X
			sscanf(g_ogl_config.gl_renderer, "Intel HD Graphics %d", &family);
			/*
			int glmajor = 0;
			int glminor = 0;
			int major = 0;
			int minor = 0;
			int release = 0;
			sscanf(g_ogl_config.gl_version, "%d.%d INTEL-%d.%d.%d", &glmajor, &glminor, &major, &minor, &release);
			version = 10000*major + 1000*minor + release;
			*/
		break;
		case DriverDetails::VENDOR_NVIDIA:
		{
			int glmajor = 0;
			int glminor = 0;
			int glrelease = 0;
			int major = 0;
			int minor = 0;
			// TODO: this is known to be broken on windows
			// nvidia seems to have removed their driver version from this string, so we can't get it.
			// hopefully we'll never have to workaround nvidia bugs
			sscanf(g_ogl_config.gl_version, "%d.%d.%d NVIDIA %d.%d", &glmajor, &glminor, &glrelease, &major, &minor);
			version = 100*major + minor;
		}
		break;
		// We don't care about these
		default:
		break;
	}
	DriverDetails::Init(vendor, driver, version, family);
}

// Init functions
Renderer::Renderer()
{
	OSDInternalW = 0;
	OSDInternalH = 0;

	s_fps=0;
	s_ShowEFBCopyRegions_VBO = 0;
	s_blendMode = 0;
	InitFPSCounter();

	bool bSuccess = true;

	// Init extension support.
	if (!GLExtensions::Init())
	{
		// OpenGL 2.0 is required for all shader based drawings. There is no way to get this by extensions
		PanicAlert("GPU: OGL ERROR: Does your video card support OpenGL 2.0?");
		bSuccess = false;
	}

	g_ogl_config.gl_vendor = (const char*)glGetString(GL_VENDOR);
	g_ogl_config.gl_renderer = (const char*)glGetString(GL_RENDERER);
	g_ogl_config.gl_version = (const char*)glGetString(GL_VERSION);
	g_ogl_config.glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	InitDriverInfo();

	// check for the max vertex attributes
	GLint numvertexattribs = 0;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &numvertexattribs);
	if (numvertexattribs < 16)
	{
		PanicAlert("GPU: OGL ERROR: Number of attributes %d not enough.\n"
				"GPU: Does your video card support OpenGL 2.x?",
				numvertexattribs);
		bSuccess = false;
	}

	// check the max texture width and height
	GLint max_texture_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&max_texture_size);
	if (max_texture_size < 1024)
	{
		PanicAlert("GL_MAX_TEXTURE_SIZE too small at %i - must be at least 1024.",
				max_texture_size);
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_framebuffer_object"))
	{
		// We want the ogl3 framebuffer instead of the ogl2 one for better blitting support.
		// It's also compatible with the gles3 one.
		PanicAlert("GPU: ERROR: Need GL_ARB_framebuffer_object for multiple render targets.\n"
				"GPU: Does your video card support OpenGL 3.0?");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_vertex_array_object"))
	{
		// This extension is used to replace lots of pointer setting function.
		// Also gles3 requires to use it.
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_vertex_array_object.\n"
				"GPU: Does your video card support OpenGL 3.0?");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_map_buffer_range"))
	{
		// ogl3 buffer mapping for better streaming support.
		// The ogl2 one also isn't in gles3.
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_map_buffer_range.\n"
				"GPU: Does your video card support OpenGL 3.0?");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_uniform_buffer_object"))
	{
		// ubo allow us to keep the current constants on shader switches
		// we also can stream them much nicer and pack into it whatever we want to
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_uniform_buffer_object.\n"
				"GPU: Does your video card support OpenGL 3.1?");
		bSuccess = false;
	}
	else if (DriverDetails::HasBug(DriverDetails::BUG_BROKENUBO))
	{
		PanicAlert("Buggy GPU driver detected.\n"
				"Please either install the closed-source GPU driver or update your Mesa 3D version.");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_sampler_objects") && bSuccess)
	{
		// Our sampler cache uses this extension. It could easyly be workaround and it's by far the
		// highest requirement, but it seems that no driver lacks support for it.
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_sampler_objects."
				"GPU: Does your video card support OpenGL 3.3?"
				"Please report this issue, then there will be a workaround");
		bSuccess = false;
	}

	// OpenGL 3 doesn't provide GLES like float functions for depth.
	// They are in core in OpenGL 4.1, so almost every driver should support them.
	// But for the oldest ones, we provide fallbacks to the old double functions.
	if (!GLExtensions::Supports("GL_ARB_ES2_compatibility") && GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
	{
		glDepthRangef = DepthRangef;
		glClearDepthf = ClearDepthf;
	}

	g_Config.backend_info.bSupportsDualSourceBlend = GLExtensions::Supports("GL_ARB_blend_func_extended");
	g_Config.backend_info.bSupportsPrimitiveRestart = !DriverDetails::HasBug(DriverDetails::BUG_PRIMITIVERESTART) &&
				((GLExtensions::Version() >= 310) || GLExtensions::Supports("GL_NV_primitive_restart"));
	g_Config.backend_info.bSupportsEarlyZ = GLExtensions::Supports("GL_ARB_shader_image_load_store");

	// Desktop OpenGL supports the binding layout if it supports 420pack
	// OpenGL ES 3.1 supports it implicitly without an extension
	g_Config.backend_info.bSupportsBindingLayout = GLExtensions::Supports("GL_ARB_shading_language_420pack");

	g_ogl_config.bSupportsGLSLCache = GLExtensions::Supports("GL_ARB_get_program_binary");
	g_ogl_config.bSupportsGLPinnedMemory = GLExtensions::Supports("GL_AMD_pinned_memory");
	g_ogl_config.bSupportsGLSync = GLExtensions::Supports("GL_ARB_sync");
	g_ogl_config.bSupportsGLBaseVertex = GLExtensions::Supports("GL_ARB_draw_elements_base_vertex");
	g_ogl_config.bSupportsGLBufferStorage = GLExtensions::Supports("GL_ARB_buffer_storage");
	g_ogl_config.bSupportsMSAA = GLExtensions::Supports("GL_ARB_texture_multisample");
	g_ogl_config.bSupportSampleShading = GLExtensions::Supports("GL_ARB_sample_shading");
	g_ogl_config.bSupportOGL31 = GLExtensions::Version() >= 310;
	g_ogl_config.bSupportViewportFloat = GLExtensions::Supports("GL_ARB_viewport_array");

	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGLES3)
	{
		if (strstr(g_ogl_config.glsl_version, "3.0"))
		{
			g_ogl_config.eSupportedGLSLVersion = GLSLES_300;
		}
		else
		{
			g_ogl_config.eSupportedGLSLVersion = GLSLES_310;
			g_Config.backend_info.bSupportsBindingLayout = true;
		}
	}
	else
	{
		if (strstr(g_ogl_config.glsl_version, "1.00") || strstr(g_ogl_config.glsl_version, "1.10") || strstr(g_ogl_config.glsl_version, "1.20"))
		{
			PanicAlert("GPU: OGL ERROR: Need at least GLSL 1.30\n"
					"GPU: Does your video card support OpenGL 3.0?\n"
					"GPU: Your driver supports GLSL %s", g_ogl_config.glsl_version);
			bSuccess = false;
		}
		else if (strstr(g_ogl_config.glsl_version, "1.30"))
		{
			g_ogl_config.eSupportedGLSLVersion = GLSL_130;
			g_Config.backend_info.bSupportsEarlyZ = false; // layout keyword is only supported on glsl150+
		}
		else if (strstr(g_ogl_config.glsl_version, "1.40"))
		{
			g_ogl_config.eSupportedGLSLVersion = GLSL_140;
			g_Config.backend_info.bSupportsEarlyZ = false; // layout keyword is only supported on glsl150+
		}
		else
		{
			g_ogl_config.eSupportedGLSLVersion = GLSL_150;
		}
	}
#if defined(_DEBUG) || defined(DEBUGFAST)
	if (GLExtensions::Supports("GL_KHR_debug"))
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
		glDebugMessageCallback( ErrorCallback, nullptr );
		glEnable( GL_DEBUG_OUTPUT );
	}
	else if (GLExtensions::Supports("GL_ARB_debug_output"))
	{
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
		glDebugMessageCallbackARB( ErrorCallback, nullptr );
		glEnable( GL_DEBUG_OUTPUT );
	}
#endif
	int samples;
	glGetIntegerv(GL_SAMPLES, &samples);
	if (samples > 1)
	{
		// MSAA on default framebuffer isn't working because of glBlitFramebuffer.
		// It also isn't useful as we don't render anything to the default framebuffer.
		// We also try to get a non-msaa fb, so this only happens when forced by the driver.
		PanicAlert("MSAA on default framebuffer isn't supported.\n"
			"Please avoid forcing dolphin to use MSAA by the driver.\n"
			"%d samples on default framebuffer found.", samples);
		bSuccess = false;
	}

	if (!bSuccess)
	{
		// Not all needed extensions are supported, so we have to stop here.
		// Else some of the next calls might crash.
		return;
	}

	glGetIntegerv(GL_MAX_SAMPLES, &g_ogl_config.max_samples);
	if (g_ogl_config.max_samples < 1 || !g_ogl_config.bSupportsMSAA)
		g_ogl_config.max_samples = 1;

	UpdateActiveConfig();

	OSD::AddMessage(StringFromFormat("Video Info: %s, %s, %s",
				g_ogl_config.gl_vendor,
				g_ogl_config.gl_renderer,
				g_ogl_config.gl_version), 5000);

	WARN_LOG(VIDEO,"Missing OGL Extensions: %s%s%s%s%s%s%s%s%s%s",
			g_ActiveConfig.backend_info.bSupportsDualSourceBlend ? "" : "DualSourceBlend ",
			g_ActiveConfig.backend_info.bSupportsPrimitiveRestart ? "" : "PrimitiveRestart ",
			g_ActiveConfig.backend_info.bSupportsEarlyZ ? "" : "EarlyZ ",
			g_ogl_config.bSupportsGLPinnedMemory ? "" : "PinnedMemory ",
			g_ogl_config.bSupportsGLSLCache ? "" : "ShaderCache ",
			g_ogl_config.bSupportsGLBaseVertex ? "" : "BaseVertex ",
			g_ogl_config.bSupportsGLBufferStorage ? "" : "BufferStorage ",
			g_ogl_config.bSupportsGLSync ? "" : "Sync ",
			g_ogl_config.bSupportsMSAA ? "" : "MSAA ",
			g_ogl_config.bSupportSampleShading ? "" : "SSAA "
			);

	s_LastMultisampleMode = g_ActiveConfig.iMultisampleMode;
	s_MSAASamples = GetNumMSAASamples(s_LastMultisampleMode);
	ApplySSAASettings();

	// Decide framebuffer size
	s_backbuffer_width = (int)GLInterface->GetBackBufferWidth();
	s_backbuffer_height = (int)GLInterface->GetBackBufferHeight();

	// Handle VSync on/off
	s_vsync = g_ActiveConfig.IsVSync();
	GLInterface->SwapInterval(s_vsync);

	// TODO: Move these somewhere else?
	FramebufferManagerBase::SetLastXfbWidth(MAX_XFB_WIDTH);
	FramebufferManagerBase::SetLastXfbHeight(MAX_XFB_HEIGHT);

	UpdateDrawRectangle(s_backbuffer_width, s_backbuffer_height);

	s_LastEFBScale = g_ActiveConfig.iEFBScale;
	CalculateTargetSize(s_backbuffer_width, s_backbuffer_height);

	// Because of the fixed framebuffer size we need to disable the resolution
	// options while running
	g_Config.bRunning = true;

	glStencilFunc(GL_ALWAYS, 0, 0);
	glBlendFunc(GL_ONE, GL_ONE);

	glViewport(0, 0, GetTargetWidth(), GetTargetHeight()); // Reset The Current Viewport

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);  // 4-byte pixel alignment

	glDisable(GL_STENCIL_TEST);
	glEnable(GL_SCISSOR_TEST);

	glScissor(0, 0, GetTargetWidth(), GetTargetHeight());
	glBlendColor(0, 0, 0, 0.5f);
	glClearDepthf(1.0f);

	if (g_ActiveConfig.backend_info.bSupportsPrimitiveRestart)
	{
		if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGLES3)
			glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		else
			if (g_ogl_config.bSupportOGL31)
			{
				glEnable(GL_PRIMITIVE_RESTART);
				glPrimitiveRestartIndex(65535);
			}
			else
			{
				glEnableClientState(GL_PRIMITIVE_RESTART_NV);
				glPrimitiveRestartIndexNV(65535);
			}
	}
	UpdateActiveConfig();
}

Renderer::~Renderer()
{

#if defined(HAVE_WX) && HAVE_WX
	if (scrshotThread.joinable())
		scrshotThread.join();
#endif
}

void Renderer::Shutdown()
{
	delete g_framebuffer_manager;

	g_Config.bRunning = false;
	UpdateActiveConfig();

	glDeleteBuffers(1, &s_ShowEFBCopyRegions_VBO);
	glDeleteVertexArrays(1, &s_ShowEFBCopyRegions_VAO);
	s_ShowEFBCopyRegions_VBO = 0;

	delete s_pfont;
	s_pfont = nullptr;
	s_ShowEFBCopyRegions.Destroy();
}

void Renderer::Init()
{
	// Initialize the FramebufferManager
	g_framebuffer_manager = new FramebufferManager(s_target_width, s_target_height);

	s_pfont = new RasterFont();

	ProgramShaderCache::CompileShader(s_ShowEFBCopyRegions,
		"in vec2 rawpos;\n"
		"in vec3 color0;\n"
		"out vec4 c;\n"
		"void main(void) {\n"
		"	gl_Position = vec4(rawpos, 0.0, 1.0);\n"
		"	c = vec4(color0, 1.0);\n"
		"}\n",
		"in vec4 c;\n"
		"out vec4 ocol0;\n"
		"void main(void) {\n"
		"	ocol0 = c;\n"
		"}\n");

	// creating buffers
	glGenBuffers(1, &s_ShowEFBCopyRegions_VBO);
	glGenVertexArrays(1, &s_ShowEFBCopyRegions_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, s_ShowEFBCopyRegions_VBO);
	glBindVertexArray( s_ShowEFBCopyRegions_VAO );
	glEnableVertexAttribArray(SHADER_POSITION_ATTRIB);
	glVertexAttribPointer(SHADER_POSITION_ATTRIB, 2, GL_FLOAT, 0, sizeof(GLfloat)*5, nullptr);
	glEnableVertexAttribArray(SHADER_COLOR0_ATTRIB);
	glVertexAttribPointer(SHADER_COLOR0_ATTRIB, 3, GL_FLOAT, 0, sizeof(GLfloat)*5, (GLfloat*)nullptr+2);
}

// Create On-Screen-Messages
void Renderer::DrawDebugInfo()
{
	// Reset viewport for drawing text
	glViewport(0, 0, GLInterface->GetBackBufferWidth(), GLInterface->GetBackBufferHeight());

	// Draw various messages on the screen, like FPS, statistics, etc.
	std::string debug_info;

	if (g_ActiveConfig.bShowFPS)
		debug_info += StringFromFormat("FPS: %d\n", s_fps);

	if (SConfig::GetInstance().m_ShowLag)
		debug_info += StringFromFormat("Lag: %" PRIu64 "\n", Movie::g_currentLagCount);

	if (g_ActiveConfig.bShowInputDisplay)
		debug_info += Movie::GetInputDisplay();

	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL && g_ActiveConfig.bShowEFBCopyRegions)
	{
		// Set Line Size
		glLineWidth(3.0f);

		// 2*Coords + 3*Color
		u32 length = stats.efb_regions.size() * sizeof(GLfloat) * (2+3)*2*6;
		glBindBuffer(GL_ARRAY_BUFFER, s_ShowEFBCopyRegions_VBO);
		glBufferData(GL_ARRAY_BUFFER, length, nullptr, GL_STREAM_DRAW);
		GLfloat *Vertices = (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, length, GL_MAP_WRITE_BIT);

		// Draw EFB copy regions rectangles
		int a = 0;
		GLfloat color[3] = {0.0f, 1.0f, 1.0f};

		for (const EFBRectangle& rect : stats.efb_regions)
		{
			GLfloat halfWidth = EFB_WIDTH / 2.0f;
			GLfloat halfHeight = EFB_HEIGHT / 2.0f;
			GLfloat x =  (GLfloat) -1.0f + ((GLfloat)rect.left / halfWidth);
			GLfloat y =  (GLfloat) 1.0f - ((GLfloat)rect.top / halfHeight);
			GLfloat x2 = (GLfloat) -1.0f + ((GLfloat)rect.right / halfWidth);
			GLfloat y2 = (GLfloat) 1.0f - ((GLfloat)rect.bottom / halfHeight);

			Vertices[a++] = x;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x2;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x2;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x2;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x2;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x2;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x2;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			// TO DO: build something nicer here
			GLfloat temp = color[0];
			color[0] = color[1];
			color[1] = color[2];
			color[2] = temp;
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);

		s_ShowEFBCopyRegions.Bind();
		glBindVertexArray( s_ShowEFBCopyRegions_VAO );
		glDrawArrays(GL_LINES, 0, stats.efb_regions.size() * 2*6);

		// Restore Line Size
		SetLineWidth();

		// Clear stored regions
		stats.efb_regions.clear();
	}

	if (g_ActiveConfig.bOverlayStats)
		debug_info += Statistics::ToString();

	if (g_ActiveConfig.bOverlayProjStats)
		debug_info += Statistics::ToStringProj();

	if (!debug_info.empty())
	{
		// Render a shadow, and then the text.
		Renderer::RenderText(debug_info, 21, 21, 0xDD000000);
		Renderer::RenderText(debug_info, 20, 20, 0xFF00FFFF);
	}
}

void Renderer::RenderText(const std::string& text, int left, int top, u32 color)
{
	const int nBackbufferWidth = (int)GLInterface->GetBackBufferWidth();
	const int nBackbufferHeight = (int)GLInterface->GetBackBufferHeight();

	s_pfont->printMultilineText(text,
		left * 2.0f / (float)nBackbufferWidth - 1,
		1 - top * 2.0f / (float)nBackbufferHeight,
		0, nBackbufferWidth, nBackbufferHeight, color);

	GL_REPORT_ERRORD();
}

TargetRectangle Renderer::ConvertEFBRectangle(const EFBRectangle& rc)
{
	TargetRectangle result;
	result.left   = EFBToScaledX(rc.left);
	result.top    = EFBToScaledY(EFB_HEIGHT - rc.top);
	result.right  = EFBToScaledX(rc.right);
	result.bottom = EFBToScaledY(EFB_HEIGHT - rc.bottom);
	return result;
}

// No video, no access to EFB.
// TODO: Set the option so Skip EFB Acesss is always enabled.
u32 Renderer::AccessEFB(EFBAccessType type, u32 x, u32 y, u32 poke_data)
{
	return 0;
}

void Renderer::ClearScreen(const EFBRectangle& rc, bool colorEnable, bool alphaEnable, bool zEnable, u32 color, u32 z)
{
	ResetAPIState();

	// color
	GLboolean const
		color_mask = colorEnable ? GL_TRUE : GL_FALSE,
		alpha_mask = alphaEnable ? GL_TRUE : GL_FALSE;
	glColorMask(color_mask,  color_mask,  color_mask,  alpha_mask);

	glClearColor(
		float((color >> 16) & 0xFF) / 255.0f,
		float((color >> 8) & 0xFF) / 255.0f,
		float((color >> 0) & 0xFF) / 255.0f,
		float((color >> 24) & 0xFF) / 255.0f);

	// depth
	glDepthMask(zEnable ? GL_TRUE : GL_FALSE);

	glClearDepthf(float(z & 0xFFFFFF) / float(0xFFFFFF));

	// Update rect for clearing the picture
	glEnable(GL_SCISSOR_TEST);

	TargetRectangle const targetRc = ConvertEFBRectangle(rc);
	glScissor(targetRc.left, targetRc.bottom, targetRc.GetWidth(), targetRc.GetHeight());

	// glColorMask/glDepthMask/glScissor affect glClear (glViewport does not)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RestoreAPIState();
}

void Renderer::ReinterpretPixelData(unsigned int convtype)
{

}

void DumpFrame(const std::vector<u8>& data, int w, int h)
{
#if defined(HAVE_LIBAV) || defined(_WIN32)
		if (g_ActiveConfig.bDumpFrames && !data.empty())
		{
			AVIDump::AddFrame(&data[0], w, h);
		}
#endif
}

// This function has the final picture. We adjust the aspect ratio here.
void Renderer::SwapImpl(u32 xfbAddr, u32 fbWidth, u32 fbHeight,const EFBRectangle& rc,float Gamma)
{
	if (XFBWrited)
		s_fps = UpdateFPSCounter();
}

// ALWAYS call RestoreAPIState for each ResetAPIState call you're doing
void Renderer::ResetAPIState()
{
	// Gets us to a reasonably sane state where it's possible to do things like
	// image copies with textured quads, etc.
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
		glDisable(GL_COLOR_LOGIC_OP);
	glDepthMask(GL_FALSE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Renderer::RestoreAPIState()
{
	// Gets us back into a more game-like state.
	glEnable(GL_SCISSOR_TEST);
	SetGenerationMode();
	BPFunctions::SetScissor();
	SetColorMask();
	SetDepthMode();
	SetBlendMode(true);
	SetLogicOpMode();
	SetViewport();

	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
		glPolygonMode(GL_FRONT_AND_BACK, g_ActiveConfig.bWireFrame ? GL_LINE : GL_FILL);
}

}

namespace NullVideo
{

bool Renderer::SaveScreenshot(const std::string &filename, const TargetRectangle &back_rc)
{
	return false; // TODO: This function is pure virtual in RenderBase, but is only called from inside this file.
}

}
