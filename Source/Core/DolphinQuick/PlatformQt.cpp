// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQuick/PlatformQt.h"

#include "Common/Logging/Log.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/VideoBackendBase.h"

#include "Common/GL/GLContext.h"

#include <QAbstractEventDispatcher>
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QQuickGraphicsDevice>
#include <QQuickView>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QVulkanInstance>
#include <QWindow>

#include "Core/Config/MainSettings.h"
#include "Core/Core.h"

#ifndef _WIN32
#include <qpa/qplatformnativeinterface.h>
#endif

#include <private/qrhivulkan_p.h>

#include <memory>
#include <string>

PlatformQt::~PlatformQt()
{
  if (m_surface.use_count() > 1)
  {
    ERROR_LOG_FMT(VIDEO, "Surface still used at platform shutdown");
  }

  m_surface.reset();
  m_window.reset();
  if (m_app)
  {
    m_app->quit();
    delete m_app;
  }
}

static WindowSystemType GetWindowSystemType()
{
  // Determine WSI type based on Qt platform.
  QString platform_name = QGuiApplication::platformName();
  if (platform_name == QStringLiteral("windows"))
    return WindowSystemType::Windows;
  else if (platform_name == QStringLiteral("cocoa"))
    return WindowSystemType::MacOS;
  else if (platform_name == QStringLiteral("xcb"))
    return WindowSystemType::Xcb;
  else if (platform_name == QStringLiteral("wayland"))
    return WindowSystemType::Wayland;
  else if (platform_name == QStringLiteral("haiku"))
    return WindowSystemType::Haiku;
  return WindowSystemType::Headless;
}

static WindowSystemInfo NativeWindowSystemInfo(QWindow* window)
{
  WindowSystemInfo wsi;
  wsi.type = GetWindowSystemType();

  // Our Win32 Qt external doesn't have the private API.
#if defined(WIN32) || defined(__APPLE__) || defined(__HAIKU__)
  wsi.render_window = reinterpret_cast<void*>(window->winId());
  wsi.render_surface = wsi.render_window;
#else
  QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
  wsi.display_connection = pni->nativeResourceForWindow("display", window);
  if (wsi.type == WindowSystemType::Wayland)
    wsi.render_window = pni->nativeResourceForWindow("surface", window);
  else
    wsi.render_window = reinterpret_cast<void*>(window->winId());
  wsi.render_surface = wsi.render_window;
#endif
  wsi.render_surface_scale = static_cast<float>(window->devicePixelRatio());

  if (wsi.type == WindowSystemType::Headless)
    wsi.enable_surface = false;

  return wsi;
}

class PWindow : public QWindow
{
  Q_OBJECT

public:
  PWindow(PlatformQt* platform) : m_platform(platform) {}

protected:
  void closeEvent(QCloseEvent* ev) override
  {
    if (m_platform->IsRunning())
    {
      ev->ignore();
      m_platform->Stop();
    }
  }

private:
  PlatformQt* m_platform;
};

bool PlatformQt::Init(int& argc, char** argv)
{
  if (!ReadConfig())
    return false;

  m_app = new QGuiApplication(argc, argv);

  // This seems to be a flaw with Qt6, I can't work out how to move the opengl context to the
  // render thread, which doesn't exist yet.
  // So instead, we just disable context thread checking.
  m_app->setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

  std::shared_ptr<QQuickView> quick_view;
  if (m_quick)
  {
    if (m_opengl)
      QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    if (m_vulkan)
      QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

    m_window = quick_view = std::make_shared<QQuickView>(QUrl());
  }
  else
  {
    m_window = std::make_shared<PWindow>(this);

    if (m_opengl)
    {
      m_window->setSurfaceType(QSurface::OpenGLSurface);
    }
    if (m_vulkan)
    {
      m_window->setSurfaceType(QSurface::VulkanSurface);
    }
  }

  m_wsi = NativeWindowSystemInfo(m_window.get());

  m_window->resize(m_window_width, m_window_height);
  m_window->setPosition(m_window_x, m_window_y);

  // Show the window as soon as possible, so the user gets some feedback
  m_window->show();

  if (m_opengl)
  {
    m_format = QSurfaceFormat::defaultFormat();
    m_format.setDepthBufferSize(24);

    m_format.setRenderableType(QSurfaceFormat::OpenGL);
    m_format.setProfile(QSurfaceFormat::CoreProfile);

    if (m_quick)
    {
      // We need to create a surface
      auto offscreen_surface = std::make_shared<QOffscreenSurface>(m_app->primaryScreen());
      m_surface = offscreen_surface;

      offscreen_surface->size() = QSize(m_window_width, m_window_height);
      offscreen_surface->setFormat(m_format);

      offscreen_surface->create();
      m_surface = std::move(offscreen_surface);

      m_ui_context = new QOpenGLContext();
      m_ui_context->setFormat(m_format);
      m_ui_context->create();
      quick_view->setGraphicsDevice(QQuickGraphicsDevice::fromOpenGLContext(m_ui_context));
    }
    else
    {
      m_surface = m_window;
      m_window->setFormat(m_format);
    }

    m_wsi.gl_context_factory = [this](bool prefer_egl) -> std::unique_ptr<GLContext> {
      return makeQtGLContext(m_surface, m_ui_context, prefer_egl);
    };
  }

  if (m_vulkan)
  {
    m_wsi.vk_set_instance = [this](VkInstance instance) {
      m_vk_instance = new QVulkanInstance();
      m_vk_instance->setVkInstance(instance);
      if (!m_vk_instance->create())
      {
        qWarning("Vulkan instance creation failed");
      }

      m_window->setVulkanInstance(m_vk_instance);
      qWarning("Instance set");
    };

    if (m_quick)
    {
      m_wsi.vk_set_device = [this, quick_view](VkPhysicalDevice physicalDevice, VkDevice device,
                                               int queueFamilyIndex, int queueIndex) {
        auto dev = QQuickGraphicsDevice::fromDeviceObjects(physicalDevice, device, queueFamilyIndex,
                                                           queueIndex);
        quick_view->setGraphicsDevice(dev);
        // m_window->show();

        if (!m_vk_instance->supportsPresent(physicalDevice, queueFamilyIndex, m_window.get()))
        {
          qWarning("Vulkan presentation not supported");
        }

        auto surface = m_vk_instance->surfaceForWindow(m_window.get());
        if (!surface)
        {
          qWarning("No vulkan surface created");
        }

        qWarning("Device set");
      };
    }
    else
    {
      m_wsi.vk_get_surface = [this]() {
        auto surface = m_vk_instance->surfaceForWindow(m_window.get());
        if (!surface)
        {
          qWarning("No vulkan surface created");
        }
        return surface;
      };
      m_wsi.vk_surface_done = [this]() { m_window->destroy(); };
    }
  }

  if (m_quick)
  {
    m_wsi.enable_surface = false;

    // We get these preferred extentions from a private Qt API
    m_wsi.vk_get_instance_extensions = []() {
      std::vector<std::string> extensions = {"VK_EXT_debug_report"};
      for (const auto& extension : QRhiVulkanInitParams::preferredInstanceExtensions())
      {
        extensions.push_back(extension.toStdString());
      }
      return extensions;
    };
    m_wsi.vk_get_device_extensions = []() {
      std::vector<std::string> extensions;
      for (const auto& extension : QRhiVulkanInitParams::preferredExtensionsForImportedDevice())
      {
        extensions.push_back(extension.toStdString());
      }
      return extensions;
    };
  }

  auto resize_lambda = []() {
    if (g_renderer)
      g_renderer->ResizeSurface();
  };

  QObject::connect(m_window.get(), &QWindow::widthChanged, resize_lambda);
  QObject::connect(m_window.get(), &QWindow::heightChanged, resize_lambda);

  if (m_vulkan || m_quick)
  {
    VideoBackendBase::PopulateBackendInfo();
    g_video_backend->AcquireContext(GetWindowSystemInfo());
    m_have_context = true;
  }

  return true;
}

QQuickView* PlatformQt::GetQuickView() const
{
  return qobject_cast<QQuickView*>(m_window.get());
}

bool PlatformQt::ReadConfig()
{
  std::string backend = Config::Get(Config::MAIN_GFX_BACKEND);
  m_quick = Config::Get(Config::MAIN_QUICK_UI);

  m_opengl = false;
  m_vulkan = false;

  if (backend == "Software Renderer" || backend == "OGL")
  {
    m_opengl = true;
  }
  else if (backend == "Vulkan")
  {
    m_vulkan = true;
  }
  else
  {
    // TODO: Dx11/Dx12
    fprintf(stderr, "Unknown video backend: %s\n", g_video_backend->GetName().c_str());
    return false;
  }
  return true;
}

void PlatformQt::SetTitle(const std::string& string)
{
  QGuiApplication::setApplicationDisplayName(QString::fromStdString(string));
}

void PlatformQt::MainLoop()
{
  // Whenever the event loop is about to go to sleep, dispatch the jobs queued in the Core first.
  QObject::connect(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::aboutToBlock,
                   m_app, [this]() {
                     UpdateRunningFlag();
                     Core::HostDispatchJobs();
                     if (!m_running.IsSet())
                     {
                       m_app->quit();
                     }
                   });

  m_app->exec();
}

void PlatformQt::ReleaseContext()
{
  if (m_have_context)
    g_video_backend->ReleaseContext();
}

void PlatformQt::Stop()
{
  // Because our window is intertwined with dolphin's vulkan instance, we need to shut
  // it down before we close our window.
  Core::Stop();
  Core::Shutdown();
  ReleaseContext();
  m_running.Clear();
}

std::unique_ptr<Platform> Platform::CreateQtPlatform()
{
  return std::make_unique<PlatformQt>();
}

#include "PlatformQt.moc"
