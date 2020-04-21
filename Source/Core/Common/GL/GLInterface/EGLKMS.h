// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once


#include "Common/GL/GLInterface/EGL.h"

class GLContextEGL_KMS final : public GLContextEGL
{
public:
  //~GLContextEGL_KMS() override;

  //void Update() override;

  void Swap() override;
  void SwapInterval(int interval) override;

protected:
  bool ValidateConfig(EGLConfig config) override;
  EGLDisplay OpenEGLDisplay() override;
  EGLNativeWindowType GetEGLNativeWindow(EGLConfig config) override;


};
