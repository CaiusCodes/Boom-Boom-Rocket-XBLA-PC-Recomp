#pragma once

#include "title_screen_state.h"
#include <algorithm>
#include <chrono>
#include <SDL3/SDL_gamepad.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <rex/ui/imgui_dialog.h>

// A noninteractive title-only label. The guest screen transition publishes a
// boolean; the UI thread never follows guest pointers or reads mutable strings.
class BbrTitleVersion final : public rex::ui::ImGuiDialog {
 public:
  BbrTitleVersion(rex::ui::ImGuiDrawer* drawer, ImFont* font) : ImGuiDialog(drawer), font_(font) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    // Query real SDL devices, not the merged input system (the keyboard is
    // deliberately exposed to the guest as an always-connected virtual pad).
    const auto now = std::chrono::steady_clock::now();
    if (now >= next_controller_check_) {
      int count=0;
      auto* pads=SDL_GetGamepads(&count);
      bbr::physical_controller_connected.store(pads && count>0);
      SDL_free(pads);
      next_controller_check_=now+std::chrono::milliseconds(250);
    }
    // WantCapture alone can be false when a non-modal settings window is open
    // but the mouse is outside it. Detect visible interactive UI on this UI
    // thread, then publish only an atomic boolean to the guest input thread.
    bool capture = io.WantCaptureKeyboard || io.WantCaptureMouse;
    for (const auto* window : ImGui::GetCurrentContext()->Windows) {
      if ((window->Active || window->WasActive) && !window->Hidden &&
          !window->IsFallbackWindow &&
          (window->Flags & ImGuiWindowFlags_NoInputs) != ImGuiWindowFlags_NoInputs) {
        capture = true;
        break;
      }
    }
    bbr::overlay_captures_input.store(capture, std::memory_order_relaxed);
    if (!bbr::title_screen_active.load(std::memory_order_relaxed)) return;
    if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0) return;
    const float scale = std::min(io.DisplaySize.x / 1280.f, io.DisplaySize.y / 720.f);
    const float left = (io.DisplaySize.x - 1280.f * scale) * .5f;
    const float bottom = (io.DisplaySize.y + 720.f * scale) * .5f;
    const ImVec2 position(left + 24.f * scale, bottom - 40.f * scale);
    auto* draw = ImGui::GetForegroundDrawList();
    auto* font = font_ ? font_ : ImGui::GetFont();
    draw->AddText(font, 18.f * scale,
                  ImVec2(position.x + scale, position.y + scale),
                  IM_COL32(0, 0, 0, 210), bbr::build_version);
    draw->AddText(font, 18.f * scale, position,
                  IM_COL32(188, 233, 243, 240), bbr::build_version);
  }
 private:
  ImFont* font_ = nullptr; // Owned by the ImGui font atlas.
  std::chrono::steady_clock::time_point next_controller_check_{};
};
