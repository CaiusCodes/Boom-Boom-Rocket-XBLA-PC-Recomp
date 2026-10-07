#pragma once

#include "profile_welcome.h"
#include <algorithm>
#include <cstring>

namespace bbr {
// Authored PC UI, evaluated by the game's own Lua 5.1/menu engine. No retail
// bytecode is redistributed here. Existing screens and numeric menu values stay
// unchanged; the added keyboard page borrows only the stock loader's stream.
inline constexpr std::string_view help_menu_source = R"lua(
GUI = {menu_ID=-1, finished=0}
function Init()
  SetupBackground()
  GUI.logo_ID = SetupBoomBoomLogo()
  SetDefaultScreenTimers()
  SetupNextAndBack(true)
  GUI.menu_ID = SetupMenu({
    {text=UIText.IDS_HOW_TO_PLAY, value=1},
    {text=UIText.IDS_CONTROLS, value=2},
    {text=UIText.IDS_PC_KEYBOARD_CONTROLS, value=5},
    {text=UIText.IDS_PC_TIMING_CALIBRATION, value=6},
    {text=UIText.IDS_OPTIONS, value=3},
    {text=UIText.IDS_CREDITS, value=4}
  }, UIEnums.Justify.JUSTIFY_RIGHT, UIText.IDS_HELP_N_OPTIONS)
  RestoreMenuSelection(GUI.menu_ID)
end
function Update()
  ContinueXBLSessionCreation()
  ShowSavePopup()
end
function Message(message, pad, data)
  CheckSaveMessage(message, pad)
  if GUI.finished == 1 or PadMessageAcceptable(pad) == 0 then return end
  if message == "BUTTON_NEXT" then
    local selection = Buttons.GetSelection(GUI.menu_ID)
    local destination = nil
    if selection == 1 then
      UIGlobals.instructions_offset = 0
      destination = "Options\\HowToPlay.lua"
    elseif selection == 2 then destination = "Options\\Controls.lua"
    elseif selection == 5 then destination = "Options\\Keyboard.lua"
    elseif selection == 6 then destination = "Options\\Calibration.lua"
    elseif selection == 3 then
      UIGlobals.options_pad = pad
      destination = "Options\\Options.lua"
    elseif selection == 4 then destination = "Options\\Credits.lua" end
    if destination ~= nil then
      GUI.finished = 1
      PlayNextSound()
      Buttons.AddEffect(GUI.logo_ID, GUIBank.fade_out_effect)
      StoreMenuSelection(GUI.menu_ID)
      Screen.SetNextScreen(destination)
    end
  elseif message == "BUTTON_BACK" then
    PlayBackSound()
    GUI.finished = 1
    Screen.SetNextScreen(UIGlobals.screen_store.options_main_back)
  end
end
function Exit() end
)lua";

inline constexpr std::string_view keyboard_help_source = R"lua(
GUI = {finished=0}
function Init()
  SetupBackground()
  SetDefaultScreenTimers()
  SetupNextAndBack(false, -1)
  SetupTitleText(UIText.IDS_PC_KEYBOARD_CONTROLS)
  local lines = {
    UIText.IDS_PC_KEYS_MOVE, UIText.IDS_PC_KEYS_START,
    UIText.IDS_PC_KEYS_PAUSE, UIText.IDS_PC_KEYS_A,
    UIText.IDS_PC_KEYS_B, UIText.IDS_PC_KEYS_X, UIText.IDS_PC_KEYS_Y,
    UIText.IDS_PC_KEYS_MENU_ESC, UIText.IDS_PC_KEYS_PLAYER
  }
  for i=1,9 do
    Buttons.AddButton({
      type=UIEnums.ButtonTypes.TEXT, x=48, y=130+(i-1)*28,
      width=544, height=UIGlobals.font_size.small,
      justify=UIEnums.Justify.JUSTIFY_LEFT, colour="255 255 255 255",
      text=lines[i], render_state=UIGlobals.font_render_state.new_filled_blue,
      effect_mode=UIEnums.TextEffectMode.ScaleUp,
      effects={GUIBank.fade_out_effect}
    })
  end
end
function Update() ContinueXBLSessionCreation() end
function Message(message, pad, data)
  if GUI.finished == 1 or PadMessageAcceptable(pad) == 0 then return end
  if message == "BUTTON_BACK" then
    PlayBackSound()
    GUI.finished = 1
    Screen.SetNextScreen("Options\\Main.lua")
  end
end
function Exit() end
)lua";

inline constexpr std::string_view timing_help_source = R"lua(
GUI = {finished=0}
function Init()
  SetupBackground()
  SetDefaultScreenTimers()
  SetupTitleText(UIText.IDS_PC_TIMING_CALIBRATION)
  SetupNextAndBack(false,UIText.IDS_A_SAVE,UIText.IDS_B_CANCEL)
  local lines = {
    UIText.IDS_PC_TIMING_INFO, UIText.IDS_PC_TIMING_LATE,
    UIText.IDS_PC_TIMING_EARLY, UIText.IDS_PC_TIMING_KEYS,
    UIText.IDS_PC_TIMING_NEXT_SONG
  }
  for i=1,5 do
    Buttons.AddButton({type=UIEnums.ButtonTypes.TEXT,
      x=48, y=124+(i-1)*28, width=544, height=UIGlobals.font_size.small,
      text=lines[i], justify=UIEnums.Justify.JUSTIFY_LEFT,
      colour="255 255 255 255",
      render_state=UIGlobals.font_render_state.new_filled_blue,
      effects={GUIBank.fade_out_effect}})
  end
  local items={}
  for i=0,40 do
    items[i+1]={text=UIText["IDS_PC_TIMING_VALUE_"..i], value=i*10-200}
  end
  GUI.spinner = Buttons.AddButton({
    type=UIEnums.ButtonTypes.VARIABLE_SPINNER,
    x=320, y=304, width=480, height=32, num_items=41, items=items,
    player=-1, text=UIText.IDS_PC_TIMING_OFFSET, font=UIGlobals.font_size.small,
    render_state={blend_mode=UIEnums.BlendMode.Additive,shader=UIEnums.Shader.MultiTexture},
    justify=UIEnums.Justify.JUSTIFY_CENTRE,
    text_info={font=0, height=UIGlobals.font_size.small, offset=-4,
      colour="255 255 255 255", text=UIText.IDS_PC_TIMING_OFFSET},
    title_selected={colour="255 255 255 255",render_state=UIGlobals.font_render_state.new_filled_orange},
    title_deselected={colour="255 255 255 255",render_state=UIGlobals.font_render_state.new_filled_blue},
    variable_selected={colour="255 255 255 255",render_state=UIGlobals.font_render_state.new_filled_orange},
    variable_deselected={colour="255 255 255 255",render_state=UIGlobals.font_render_state.new_filled_blue},
    end_size=50, menu_end_l=UIGlobals.gfx.menu_end_l,
    menu_end_r=UIGlobals.gfx.menu_end_r, menu_line=UIGlobals.gfx.menu_line,
    menu_double_line=UIGlobals.gfx.menu_double_line,
    selected_colour=UIGlobals.colour.orange, colour=UIGlobals.colour.blue,
    effects={GUIBank.fade_out_effect}
  })
  Buttons.SetSelected(GUI.spinner,true)
end
function Update() ContinueXBLSessionCreation() end
function Message(message,pad,data)
  if GUI.finished==1 or PadMessageAcceptable(pad)==0 then return end
  if message=="BUTTON_NEXT" or message=="BUTTON_BACK" then
    GUI.finished=1
    if message=="BUTTON_NEXT" then
      Buttons.GetSelection(GUI.spinner) -- commit this page's accepted Save action
      PlayNextSound()
    else PlayBackSound() end
    Screen.SetNextScreen("Options\\Main.lua")
  end
end
function Exit() end
)lua";

struct NativeHelpReader {
  std::string_view source;
  size_t position = 0;
  // Both fingerprints are required before adding a route to the new page.
  // Return the real stock filename's guest address for the virtual keyboard
  // screen, so no external script or asset needs to exist on disk.
  uint32_t Begin(std::string_view name, std::span<const uint8_t> main_stream,
                 std::span<const uint8_t> controls_stream) {
    source = {};
    position = 0;
    if (main_stream.size() != 1115 || controls_stream.size() != 1479 ||
        ProfileScriptCrc(main_stream) != 0xFC54F2C1u ||
        ProfileScriptCrc(controls_stream) != 0x5C335632u) return 0;
    if (name == "Options\\Main.lua") source = help_menu_source;
    if (name == "Options\\Keyboard.lua") {
      source = keyboard_help_source;
      return 0x822F0611u;
    }
    if (name == "Options\\Calibration.lua") {
      source = timing_help_source;
      return 0x822F0611u;
    }
    return 0;
  }
  bool Read(std::span<uint8_t> buffer, uint32_t& size) {
    if (source.empty()) return false;
    size = uint32_t(std::min(buffer.size(), source.size()-position));
    std::memcpy(buffer.data(), source.data()+position, size);
    position += size;
    return true; // Includes explicit EOF, even when replacement is longer.
  }
  void End() { source = {}; position = 0; }
};
inline thread_local NativeHelpReader native_help_reader;
} // namespace bbr
