// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Common/GL/GLInterface/EGLKMS.h"

void GLContextEGL_KMS::Swap()
{
  if (m_egl_surface != EGL_NO_SURFACE)
    eglSwapBuffers(m_egl_display, m_egl_surface);
    m_wsi.swap_function(0);
}
void GLContextEGL_KMS::SwapInterval(int interval)
{
  eglSwapBuffers(m_egl_display, m_egl_surface);
  m_wsi.swap_function(interval);
}

EGLDisplay GLContextEGL_KMS::OpenEGLDisplay()
{
  return eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, m_wsi.display_connection, NULL);
}

EGLNativeWindowType GLContextEGL_KMS::GetEGLNativeWindow(EGLConfig config)
{
  return reinterpret_cast<EGLNativeWindowType>(m_wsi.render_surface);
}

bool GLContextEGL_KMS::ValidateConfig(EGLConfig config) {
    EGLint gbm_format = 0;

    int ret = eglGetConfigAttrib(m_egl_display, config, EGL_NATIVE_VISUAL_ID, &gbm_format);

    //printf("config %i %x %x\n", ret, gbm_format, config);

    // We are currently hardcoding KMS to GBM_FORMAT_XRGB8888, which as the 4CC code of XR24
    return gbm_format == ('X' | 'R' << 8 | '2' << 16 | '4' << 24);
}