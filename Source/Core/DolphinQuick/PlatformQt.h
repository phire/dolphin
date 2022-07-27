
// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Core/Config/MainSettings.h"
#include "DolphinNoGUI/Platform.h"

#include <QtGui/QSurfaceFormat>

class QGuiApplication;
class QOffscreenSurface;
class QSurfaceFormat;
class QThread;
class QWindow;
class QVulkanInstance;
class QSurface;
class QQuickView;

class PlatformQt : public Platform
{
public:
  ~PlatformQt() override;

  bool Init(int& argc, char** argv) override;
  void SetTitle(const std::string& string) override;
  void MainLoop() override;
  void Stop() override;

  void ReleaseContext();
  QQuickView* GetQuickView() const;

  WindowSystemInfo GetWindowSystemInfo() const override { return m_wsi; };

protected:
  friend class QtGLContext;

  bool m_opengl = false;
  bool m_vulkan = false;
  bool m_quick = false;
  bool m_have_context = false;

  bool ReadConfig();

  QGuiApplication* m_app = nullptr;
  std::shared_ptr<QSurface> m_surface;
  std::shared_ptr<QWindow> m_window;
  QSurfaceFormat m_format;
  QOpenGLContext* m_ui_context = nullptr;
  QVulkanInstance* m_vk_instance = nullptr;

  int m_window_x = Config::Get(Config::MAIN_RENDER_WINDOW_XPOS);
  int m_window_y = Config::Get(Config::MAIN_RENDER_WINDOW_YPOS);
  int m_window_width = Config::Get(Config::MAIN_RENDER_WINDOW_WIDTH);
  int m_window_height = Config::Get(Config::MAIN_RENDER_WINDOW_HEIGHT);

  WindowSystemInfo m_wsi;
};

std::unique_ptr<GLContext> makeQtGLContext(std::shared_ptr<QSurface> surface,
                                           QOpenGLContext* shareContext, bool egl);
