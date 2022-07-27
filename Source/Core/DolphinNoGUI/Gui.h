// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Core/Boot/Boot.h"
#include "DolphinNoGUI/Platform.h"

class Gui
{
public:
  Gui() {}
  virtual ~Gui();

  virtual bool Init(Platform* platform);
  virtual void SetBootParameters(std::unique_ptr<BootParameters> boot);
  virtual int Run();
  virtual void OnBoot() {}
  virtual void OnShutdown() { m_platform->Stop(); }
  virtual bool HasMenu() const { return false; }

  Platform* m_platform = nullptr;

protected:
  virtual bool Boot();

  std::unique_ptr<BootParameters> m_boot;

public:
#if QUICK_GUI
  static std::unique_ptr<Gui> CreateQuickGui();
#endif

  static std::unique_ptr<Gui> CreateNullGui();
};
