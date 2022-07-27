// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later


#include "DolphinNoGUI/Gui.h"
#include "DolphinQuick/PlatformQt.h"
#include "DolphinQuick/Emulator.h"

#include "Common/Assert.h"
#include "VideoCommon/VideoBackendBase.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickView>

#include <QTimer>

Emulator* Emulator::instance = nullptr;
int Emulator::typeId;

class QuickGui : public Gui
{
public:
  virtual ~QuickGui();

  virtual bool Init(Platform* platform) override;
  virtual int Run() override;
  virtual void OnBoot() override;
  virtual void OnShutdown() override;

private:
  Emulator m_emulator;
  QQuickView* m_view = nullptr;
  bool m_exiting = false;
};

QuickGui::~QuickGui()
{
  g_video_backend->ReleaseContext();
}

bool QuickGui::Init(Platform* platform)
{
  m_platform = platform;
  PlatformQt* qt_platform = dynamic_cast<PlatformQt*>(platform);

  if (!qt_platform)
  {
    // Technically we can make QtQuick work with any platform, just inject a window/surface
    // a 3d device, and hookup input.
    // But it's not worth it, Qt already has plugins for more platforms that we do.

    PanicAlertFmt("QuickGui requires Qt Platform.");
    return false;
  }

  Emulator::instance = &m_emulator;
  Emulator::typeId = qmlRegisterSingletonInstance("DolphinQuick", 1, 0, "Emulator", &m_emulator);

  m_view = qt_platform->GetQuickView();
  if (!m_view)
  {
    PanicAlertFmt("Failed to create QQuickView.");
    return false;
  }

  m_view->setResizeMode(QQuickView::SizeRootObjectToView);
  m_view->setSource(QUrl("qrc:/DolphinQuick/main.qml"));

  QObject::connect(m_view, &QWindow::close, [this]() {
    m_platform->RequestShutdown();
    m_exiting = true;
    return false;
  });

  QTimer::singleShot(0, m_view, [this]() {
    qWarning("QQuickView ready");
    Boot();
  });

  return true;
}

int QuickGui::Run()
{
  m_platform->MainLoop();
  return 0;
}

void QuickGui::OnBoot() {
  emit m_emulator.dolphinBooted();
}

void QuickGui::OnShutdown() {
  if (!m_exiting)
    m_platform->Stop();
}

// std::unique_ptr<Gui> Gui::CreateQuickGui()
// {
//   return std::make_unique<QuickGui>();
// }

