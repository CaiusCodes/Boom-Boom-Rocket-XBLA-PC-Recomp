// boom_boom_rocket - ReXGlue Recompiled Project

#include "generated/default/boom_boom_rocket_init.h"

#include "boom_boom_rocket_app.h"
#include "profile_welcome.h"
#include "native_help.h"
#include "timing_calibration.h"
#include "rocket_labels.h"

REXCVAR_DEFINE_INT32(bbr_rocket_labels, -1, "BBR/Gameplay",
                    "Player 1 rocket labels: -1 automatic, 0 none, 1 arrows, 2 ABXY");
REXCVAR_DEFINE_INT32(bbr_player_two_rocket_labels, -1, "BBR/Gameplay",
                    "Player 2 rocket labels: -1 automatic, 0 none, 1 arrows, 2 ABXY");
int32_t BBR_GetRocketLabels(uint32_t slot) {
  const auto text=rex::cvar::GetFlagByName(slot==1?"bbr_player_two_rocket_labels":"bbr_rocket_labels");
  char* end=nullptr;
  const long value=std::strtol(text.c_str(),&end,10);
  const int32_t preference=end!=text.c_str() && *end=='\0' && value>=-1 && value<=2?int32_t(value):-1;
  return bbr::ResolveRocketLabels(preference,bbr::physical_controller_connected.load());
}
void BBR_SaveRocketLabels(uint32_t slot,int32_t value) {
  if(value<0 || value>2) return;
  rex::cvar::SetFlagByName(slot==1?"bbr_player_two_rocket_labels":"bbr_rocket_labels",std::to_string(value));
  BoomBoomRocketApp::SavePcSettings();
}

REXCVAR_DEFINE_INT32(bbr_timing_offset_ms, 0, "BBR/Gameplay",
                    "Hit timing offset in milliseconds; positive accepts later hits");

int32_t BBR_GetTimingOffset() {
  const auto value = rex::cvar::GetFlagByName("bbr_timing_offset_ms");
  return bbr::TimingCalibration::Normalize(std::atoi(value.c_str()));
}
void BBR_SetTimingOffset(int32_t value) {
  rex::cvar::SetFlagByName("bbr_timing_offset_ms",
      std::to_string(bbr::TimingCalibration::Normalize(value)));
  BoomBoomRocketApp::SavePcSettings();
}
void BBR_BeginJudgementSession() {
  bbr::timing_calibration.Begin(BBR_GetTimingOffset());
}
double BBR_AdjustJudgementTime(double seconds) {
  return bbr::timing_calibration.Adjust(seconds);
}
void BBR_ClearCustomMenuState(const char* name);
void BBR_ResetDisplaySettings() {
  BoomBoomRocketApp::SetPcFullscreen(false);
  BoomBoomRocketApp::SetPcResolution(1280,720,1);
  BoomBoomRocketApp::SetPcVSync(true);
  BoomBoomRocketApp::SetPcShowFps(false);
}

REXCVAR_DEFINE_BOOL(bbr_runtime_shortcuts, false, "BBR/Diagnostics",
                   "Enable ReXGlue overlay/console shortcuts for troubleshooting")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

uint32_t BBR_BeginUiScript(const char* name, uint8_t* base) {
  auto& reader = bbr::profile_welcome_reader;
  reader.active = false;
  bbr::native_help_reader.End();
  BBR_ClearCustomMenuState(name);
  if (!name) return 0;
  if (std::string_view(name) == "Options\\Main.lua" ||
      std::string_view(name) == "Options\\Keyboard.lua" ||
      std::string_view(name) == "Options\\Calibration.lua") {
    return bbr::native_help_reader.Begin(name, {base+0x82305099u,1115},
                                       {base+0x823042A2u,1479});
  }
  if (std::string_view(name) != "Profile\\Load.lua") return 0;
  // This embedded stream belongs to the supported recompiled XEX. A different
  // stream fails the fingerprint and retains the original working save flow.
  reader.Begin(name, {base + 0x82307007u, 2000});
  return 0;
}

bool BBR_ReadNativeHelp(uint8_t* data, uint32_t& size) {
  return bbr::native_help_reader.Read({data,512},size);
}

void BBR_PatchUiScriptChunk(uint8_t* data, uint32_t size) {
  bbr::profile_welcome_reader.Patch({data, size});
}

void BBR_EndUiScript() {
  bbr::profile_welcome_reader.active = false;
  bbr::native_help_reader.End();
}

bool BBR_IsTitleScreen(const char* screen) {
  return screen && bbr::IsTitleScreen(screen);
}

void BBR_SetTitleScreenActive(bool active) {
  bbr::title_screen_active.store(active, std::memory_order_relaxed);
}
bool BBR_IsGameplayScreen(const char* screen) {
  return screen && bbr::IsGameplayScreen(screen);
}
void BBR_SetGameplayScreenActive(bool active) {
  bbr::gameplay_screen_active.store(active, std::memory_order_relaxed);
}

int32_t BBR_GetPcResolutionIndex() {
  return BoomBoomRocketApp::GetPcResolutionIndex();
}
bool BBR_GetPcFullscreen() {
  return BoomBoomRocketApp::GetPcFullscreen();
}

bool BBR_GetPcVSync() {
  return BoomBoomRocketApp::GetPcVSync();
}

void BBR_SetPcVSync(bool enabled) {
  BoomBoomRocketApp::SetPcVSync(enabled);
}

bool BBR_GetPcShowFps() {
  return BoomBoomRocketApp::GetPcShowFps();
}

void BBR_SetPcShowFps(bool enabled) {
  BoomBoomRocketApp::SetPcShowFps(enabled);
}

void BBR_SetPcFullscreen(bool fullscreen) {
  BoomBoomRocketApp::SetPcFullscreen(fullscreen);
}

void BBR_SetPcResolution(
    int32_t width,
    int32_t height,
    int32_t scale) {
  BoomBoomRocketApp::SetPcResolution(
      width,
      height,
      scale);
}

REX_DEFINE_APP(boom_boom_rocket, BoomBoomRocketApp::Create)
