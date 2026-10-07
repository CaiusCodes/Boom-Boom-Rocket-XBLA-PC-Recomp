#pragma once

#include <atomic>
#include <cstdint>
#include <string_view>

namespace bbr {
inline std::atomic<bool> title_screen_active{false};
inline std::atomic<bool> gameplay_screen_active{false};
inline std::atomic<bool> song_select_active{false};
inline std::atomic<bool> physical_controller_connected{false};
inline constexpr char build_version[] = "v0.9.1";
inline std::atomic<bool> overlay_captures_input{false};

inline bool IsTitleScreen(std::string_view screen) {
  return screen == "Main\\TitleScreen.lua";
}
// Keep the original prompt ID on the widget. Resolve only the rendered text,
// so connecting/disconnecting a pad updates it without reloading the screen.
inline uint32_t TitlePromptId(uint32_t id, bool title, bool controller) {
  return id == 0x2B && title && !controller ? 0x16B : id;
}
inline bool IsTitleStartKey(uint16_t key) {
  // Ordinary keys, including unmapped letters and function keys. Modifier /
  // system keys alone must not turn a desktop shortcut into a game action.
  return key >= 8 && key < 256 && key != 0x10 && key != 0x11 &&
         key != 0x12 && key != 0x5B && key != 0x5C && key != 0x5D &&
         (key < 0xA0 || key > 0xA5);
}
inline bool IsSongSelectScreen(std::string_view screen) {
  return screen == "Main\\ChooseSong.lua" || screen == "Main\\MpChooseSong.lua" ||
         screen == "Main\\LbChooseSong.lua";
}
inline bool IsControllerIcon(std::u16string_view token) {
  return token == u"{A}" || token == u"{B}" || token == u"{X}" || token == u"{Y}" ||
         token == u"{DP}" || token == u"{LT}" || token == u"{RT}" ||
         token == u"{LB}" || token == u"{RB}" || token == u"{LS}" || token == u"{RS}" ||
         token == u"{START}" || token == u"{BACK}" || token == u"{J}";
}
inline bool IsGameplayScreen(std::string_view screen) {
  return screen == "Ingame\\Hud.lua" || screen == "Ingame\\Hud_2p.lua" ||
         screen == "Ingame\\Hud_Casual.lua" || screen == "Ingame\\Hud_Visualiser.lua" ||
         screen == "Ingame\\GetReady.lua" || screen == "Ingame\\GetReady_2p.lua";
}
}  // namespace bbr
