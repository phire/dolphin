// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinNoGUI/Gui.h"
#include "Core/Core.h"
#include "Core/DolphinAnalytics.h"
#include "Core/BootManager.h"


#ifdef USE_DISCORD_PRESENCE
#include "UICommon/DiscordPresence.h"
#endif


bool Gui::Init(Platform* platform)
{
    m_platform = platform;
    return true;
}

void Gui::SetBootParameters(std::unique_ptr<BootParameters> boot) {
  m_boot = std::move(boot);
}

int Gui::Run() {
  Core::AddOnStateChangedCallback([this](Core::State state) {
    if (state == Core::State::Uninitialized)
      m_platform->Stop();
  });

  DolphinAnalytics::Instance().ReportDolphinStart("nogui");

  if (!Boot())
    return 1;

  m_platform->MainLoop();

  Core::Stop();

  Core::Shutdown();

  return 0;
}

bool Gui::Boot() {
  if (!BootManager::BootCore(std::move(m_boot), m_platform->GetWindowSystemInfo()))
  {
    fprintf(stderr, "Could not boot the specified file\n");
    return false;
  }

#ifdef USE_DISCORD_PRESENCE
  Discord::UpdateDiscordPresence();
#endif

  return true;
}


Gui::~Gui() {

}

std::unique_ptr<Gui> Gui::CreateNullGui() {
    return std::make_unique<Gui>();
}
