// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The goal of this API is to meet the basic usecase of "I'm a script and I want to
// throw some debug text and lines on the screen"

#include "Common/CommonTypes.h"
#include "VideoCommon/BasicGuiAPI.h"

#include <imgui.h>
#include <fmt/format.h>

#include <BasicTypes.h>
#include <export.h>

#include <cstdlib>


namespace BasicGuiAPI {

constexpr float WINDOW_PADDING = 4.0f;

static ImVec4 ARGBToImVec4(const u32 argb)
{
  return ImVec4(static_cast<float>((argb >> 16) & 0xFF) / 255.0f,
                static_cast<float>((argb >> 8) & 0xFF) / 255.0f,
                static_cast<float>((argb >> 0) & 0xFF) / 255.0f,
                static_cast<float>((argb >> 24) & 0xFF) / 255.0f);
}

static std::vector<std::function<void (BasicGuiHandle)>> Hooks;
static uint32_t WindowId = 0;
static BasicGuiHandle CurrentHandle;

std::vector<std::function<void (BasicGuiHandle)>>& getCallbacks() {
  return Hooks;
}

BasicGuiHandle StartDraw() {
  WindowId = 0;
  CurrentHandle = std::rand();
  return CurrentHandle;
}

void EndDraw() {
  CurrentHandle = 0;
}

extern "C" {

EXPORTED u32 BasicGui_DrawText(BasicGuiHandle handle, String* text, u32 posX, u32 posY, u32 color)
{
  if (handle != CurrentHandle) {
    // TODO: raise error
    return 0;
  }

   // We have to provide a window name, and these shouldn't be duplicated.
  // So instead, we generate a name based on the number of messages drawn.
  const std::string window_name = fmt::format("bgui_{}", WindowId++);

  const float x = posX * ImGui::GetIO().DisplayFramebufferScale.x;
  const float y = posY * ImGui::GetIO().DisplayFramebufferScale.y;

  // The size must be reset, otherwise the length of old messages could influence new ones.
  ImGui::SetNextWindowPos(ImVec2(x, y));
  ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f));

  float window_height = 0.0f;
  if (ImGui::Begin(window_name.c_str(), nullptr,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav |
                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing))
  {
    ImGui::TextColored(ARGBToImVec4(color), "%-.*s", text->Len, text->Data);
    window_height = ImGui::GetWindowSize().y + (WINDOW_PADDING * ImGui::GetIO().DisplayFramebufferScale.y);
  }

  ImGui::End();
  return window_height / ImGui::GetIO().DisplayFramebufferScale.y;
}

EXPORTED u64 BasicGui_RegisterDrawHook(void (*Callback)(BasicGuiHandle)) {
  // TODO: add some tracking so we can later remove this hook
  Hooks.push_back(Callback);
  return 0;
}

}

} // BasicGuiAPI