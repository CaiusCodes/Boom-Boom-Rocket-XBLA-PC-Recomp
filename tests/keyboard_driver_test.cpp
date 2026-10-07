#include "src/keyboard_driver.h"
#include "src/runtime_shortcuts.h"
#include "src/rocket_labels.h"
#include <rex/input/input_system.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
using X_RESULT = rex::X_RESULT;

void Check(bool ok, const char* name) {
  if (!ok) throw std::runtime_error(name);
  std::cout << "PASS: " << name << std::endl;
}
auto Key(uint16_t key, bool repeat = false, bool alt = false) {
  return rex::ui::KeyEvent(nullptr, static_cast<rex::ui::VirtualKey>(key), 1, repeat, false, false, alt, false);
}
uint16_t Poll(BbrKeyboardDriver& driver) {
  rex::input::X_INPUT_STATE state{};
  driver.GetState(0, &state); return state.gamepad.buttons;
}
class PhysicalPad final : public rex::input::InputDriver {
 public:
  using X_STATUS = rex::X_STATUS; using X_RESULT = rex::X_RESULT;
  PhysicalPad() : InputDriver(nullptr, 0) {}
  X_STATUS Setup() override { return X_STATUS_SUCCESS; }
  X_RESULT GetCapabilities(uint32_t, uint32_t, rex::input::X_INPUT_CAPABILITIES*) override { return X_ERROR_SUCCESS; }
  X_RESULT GetState(uint32_t, rex::input::X_INPUT_STATE* state) override {
    *state = {}; state->gamepad.buttons = 0x8000; return X_ERROR_SUCCESS;
  }
  X_RESULT SetState(uint32_t, rex::input::X_INPUT_VIBRATION*) override { return X_ERROR_SUCCESS; }
  X_RESULT GetKeystroke(uint32_t, uint32_t, rex::input::X_INPUT_KEYSTROKE*) override { return X_ERROR_EMPTY; }
};
int main() {
 try {
  Check(bbr::TitlePromptId(0x2B,true,false)==0x16B &&
        bbr::TitlePromptId(0x2B,true,true)==0x2B,
        "title prompt follows physical controller presence");
  Check(bbr::TitlePromptId(0x2B,false,false)==0x2B &&
        bbr::TitlePromptId(0x62,true,false)==0x62,
        "any-key prompt does not replace other screens or text");
  Check(bbr::IsTitleStartKey('Q') && bbr::IsTitleStartKey(0x1B) &&
        bbr::IsTitleStartKey(0x0D) && bbr::IsTitleStartKey(0x70),
        "any-key title accepts ordinary, unmapped and function keys");
  Check(!bbr::IsTitleStartKey(1) && !bbr::IsTitleStartKey(2) &&
        !bbr::IsTitleStartKey(0x10) && !bbr::IsTitleStartKey(0x11) &&
        !bbr::IsTitleStartKey(0x5B) && !bbr::IsTitleStartKey(256),
        "any-key policy excludes mouse, modifiers and invalid key codes");
  for(uint32_t count : {2u,3u,4u,11u}) {
    uint32_t choice=0;
    for(uint32_t step=0;step<count;++step) {
      choice=bbr::NextSpinnerChoice(choice,count);
      Check(choice==(step+1)%count,"every spinner choice advances and wraps exactly once");
    }
  }
  Check(bbr::NextSpinnerChoice(0,0)==0 && bbr::NextSpinnerChoice(0,1)==0 &&
        bbr::NextSpinnerChoice(100,3)==0,
        "empty, action and invalid spinner selections cannot overflow");
  bbr::SpinnerClickCycle cycle;
  Check(!cycle.Take(123),"keyboard-only spinner events have no mouse wrapping token");
  cycle.Arm(123);
  Check(!cycle.Take(456) && cycle.Take(123) && !cycle.Take(123),
        "mouse cycle token belongs to one widget and is consumed once");
  cycle.Arm(123);cycle.Clear();
  Check(!cycle.Take(123),"screen and popup transitions discard old cycle tokens");
  Check(bbr::ResolveRocketLabels(-1,false)==1 && bbr::ResolveRocketLabels(-1,true)==2,
        "automatic rocket labels follow physical controller presence");
  for(int preference=0;preference<3;++preference)
    Check(bbr::ResolveRocketLabels(preference,false)==preference &&
          bbr::ResolveRocketLabels(preference,true)==preference,
          "manual rocket-label choice wins over controller hot-plug");
  Check(bbr::ResolveRocketLabels(9,false)==1 && bbr::ResolveRocketLabels(-7,true)==2,
        "invalid rocket-label preferences safely use automatic mode");
  bbr::RocketLabelsEdit labels;
  Check(!labels.Save(),"saving unrelated settings does not disable automatic labels");
  labels.Change(2);labels.Cancel();
  Check(!labels.Save(),"cancelled label edits do not create a persistent override");
  labels.Change(2);labels.Change(0);
  Check(labels.Save()==0 && !labels.Save(),"saved manual label choice commits exactly once, including None");
  Check(bbr::IsModalUiScript("Popups\\Generic.lua") &&
        bbr::IsModalUiScript("Popups\\ResetOptions.lua"),
        "modal script loads preserve the underlying Settings selection bridge");
  Check(!bbr::IsModalUiScript("Options\\Options.lua") &&
        !bbr::IsModalUiScript("Options\\Main.lua") && !bbr::IsModalUiScript(""),
        "real screen changes clear old Settings widget references");
  Check(bbr::IsSongSelectScreen("Main\\ChooseSong.lua") &&
        bbr::IsSongSelectScreen("Main\\MpChooseSong.lua") &&
        !bbr::IsSongSelectScreen("Main\\MainMenu.lua"),"wheel routing is restricted to song lists");
  Check(bbr::IsControllerIcon(u"{A}") && bbr::IsControllerIcon(u"{DP}") &&
        bbr::IsControllerIcon(u"{RT}") && !bbr::IsControllerIcon(u"{STAR}") &&
        !bbr::IsControllerIcon(u"nn"),"only controller tokens are hidden, not art or text commands");
  bbr::SongWheel wheel;
  wheel.Add(120,1000);
  Check(wheel.Next(1000)==1 && wheel.Next(1200)==0,"one wheel notch produces exactly one Up action");
  wheel.Add(-120,1300);
  Check(wheel.Next(1300)==-1,"reverse wheel produces Down");
  wheel.Add(3600,1600);
  Check(wheel.Next(1600)==1 && wheel.Next(1601)==0 && wheel.Next(1750)==1 &&
        wheel.Next(1900)==1 && wheel.Next(2050)==0,"fast wheel bursts are bounded and paced");
  wheel.Add(360,2200);
  Check(wheel.Next(2700)==0,"old wheel motion expires instead of scrolling later");
  wheel.Add(60,3000);Check(wheel.Next(3000)==0,"partial wheel notch waits");
  wheel.Add(60,3010);Check(wheel.Next(3010)==1,"two half notches produce one action");
  bbr::PublishMenuWheel(120);bbr::ClearMenuMouse();
  Check(bbr::TakeMenuWheel()==0,"screen and focus changes discard queued wheel input");
  for(float scale : {1.f,1.25f,1.5f,2.f,3.f}) {
    auto p=bbr::ToMenuPoint(640*scale,360*scale,1280*scale,720*scale);
    Check(p.valid && p.x==320 && p.y==240,"mouse mapping is invariant across physical DPI/resolution scales");
  }
  auto letterbox=bbr::ToMenuPoint(640,400,1280,800);
  Check(letterbox.valid && letterbox.x==320 && letterbox.y==240,"16:10 letterbox maps UI centre");
  Check(!bbr::ToMenuPoint(100,20,1280,800).valid,"letterbox bars reject mouse hits");
  auto ultrawide=bbr::ToMenuPoint(1720,720,3440,1440);
  Check(ultrawide.valid && ultrawide.x==320 && ultrawide.y==240,"ultrawide pillarbox maps UI centre");
  Check(!bbr::ToMenuPoint(20,720,3440,1440).valid,"pillarbox bars reject mouse hits");
  bbr::PublishMenuMouse({10,20,true},true);
  bbr::PublishMenuMouse({11,21,true},false);
  auto mouse=bbr::TakeMenuMouse();
  Check(mouse.click && mouse.point.x==11 && !bbr::TakeMenuMouse().dirty,"mouse click survives movement and is consumed once");
  bbr::PublishMenuMouse({10,20,true},true);bbr::ClearMenuMouse();
  Check(!bbr::TakeMenuMouse().click && !bbr::mouse_menu_active.load(),"screen/focus change clears pending menu clicks");
  for (const auto* shortcut : {"F3", "F4", "F7", "Backtick"}) {
    int invoked = 0;
    const auto bind_name = std::string("bbr_test_runtime_shortcut_") + shortcut;
    rex::ui::RegisterBind(bind_name, shortcut, "Test only", [&] { ++invoked; });
    auto disabled = Key(static_cast<uint16_t>(rex::ui::ParseVirtualKey(shortcut)));
    Check(!bbr::DispatchRuntimeShortcut(false, disabled) && invoked == 0,
          "release gate blocks runtime shortcut callback");
    auto enabled = Key(static_cast<uint16_t>(rex::ui::ParseVirtualKey(shortcut)));
    Check(bbr::DispatchRuntimeShortcut(true, enabled) && invoked == 1,
          "diagnostic switch restores runtime shortcut callback");
    rex::ui::UnregisterBind(bind_name);
  }
  bbr::KeyboardState state;
  for (auto bind : bbr::keyboard_bindings) {
    state.Clear(); state.Set(bind.key, true);
    Check(state.Poll() == bind.button, "requested key/button mapping");
    state.Set(bind.key, false); Check(state.Poll() == 0, "mapped button releases");
  }
  state.Clear(); state.Set('E', true); state.Set(1, true); state.Poll(); state.Set('E', false);
  Check(state.Poll() == 0x1000, "releasing E keeps held left-click A");
  state.Set(1, false); Check(state.Poll() == 0, "last A alias releases");
  state.Clear(); state.Set('X', true); state.Set('X', false);
  Check(state.Poll() == 0x4000 && state.Poll() == 0, "short tap survives one guest poll only");
  state.Set('W', true); state.Set('E', true);
  Check(state.Poll() == 0x1001, "direction plus face-button chord");
  state.Clear(); Check(state.Poll() == 0, "clear drops held and pending input");
  for (auto screen : {"Ingame\\Hud.lua", "Ingame\\Hud_2p.lua", "Ingame\\Hud_Casual.lua",
                      "Ingame\\Hud_Visualiser.lua", "Ingame\\GetReady.lua", "Ingame\\GetReady_2p.lua"})
    Check(bbr::IsGameplayScreen(screen), "playing/countdown screen enables Esc Start");
  for (auto screen : {"Main\\MainMenu.lua", "Ingame\\Pause.lua", "Ingame\\GameOver.lua",
                      "Ingame\\RetryMenu_2p.lua", "Options\\Options.lua", "Popups\\TheCustomPopup.lua"})
    Check(!bbr::IsGameplayScreen(screen), "menus/results/popups keep Esc Back");
  state.Set(0x1B, true, true);
  Check(state.Poll() == 0x10, "Esc sends only Start during gameplay");
  state.Set(0x1B, true, false);
  Check(state.Poll() == 0x10, "held Esc does not change meaning after pause opens");
  state.Set(0x1B, false, false);
  Check(state.Poll() == 0, "Esc releases original Start after screen transition");
  state.Set(0x1B, true, false);
  Check(state.Poll() == 0x2000, "next Esc press sends Back in the pause menu");
  state.Set(0x1B, false, true);
  Check(state.Poll() == 0, "Back releases even when gameplay resumes before key-up");
  state.Set(0x1B, true, true); state.Set(0x0D, true); state.Poll(); state.Set(0x1B, false);
  Check(state.Poll() == 0x10, "Enter remains held after gameplay Esc releases");
  state.Clear(); state.Set('W', true); state.Set(0x26, true); state.Poll(); state.Set('W', false);
  Check(state.Poll() == 1, "arrow alias remains held after W releases");
  state.Set(0x26, false); Check(state.Poll() == 0, "final direction alias releases");

  BbrKeyboardDriver driver; rex::ui::UISetupEvent focus;
  bool allowed = true;
  driver.set_is_active_callback([&] { return allowed; }); driver.OnGotFocus(focus);
  auto e = Key('E'); driver.OnKeyDown(e); Check(Poll(driver) == 0x1000, "driver sends A");
  driver.OnLostFocus(focus); Check(Poll(driver) == 0, "focus loss releases input");
  driver.OnGotFocus(focus); Check(Poll(driver) == 0, "focus regain has no stale input");
  driver.OnKeyDown(e); allowed = false; Check(Poll(driver) == 0, "overlay capture blocks input");
  allowed = true; Check(Poll(driver) == 0, "overlay close has no stale input");
  auto repeat = Key('E', true); driver.OnKeyDown(repeat);
  Check(Poll(driver) == 0, "OS repeat after capture does not retrigger");
  auto alt_enter = Key(0x0D, false, true); driver.OnKeyDown(alt_enter);
  Check(Poll(driver) == 0, "system shortcut does not press Start");
  rex::input::X_INPUT_KEYSTROKE stroke{};
  auto escape = Key(0x1B);
  bbr::gameplay_screen_active.store(true); driver.OnKeyDown(escape);
  Check(Poll(driver) == 0x10 && driver.GetKeystroke(0, 0, &stroke) == 0 && stroke.virtual_key == 0x5814,
        "driver emits Xbox Start for gameplay Esc");
  bbr::gameplay_screen_active.store(false); driver.OnKeyUp(escape);
  Check(Poll(driver) == 0 && driver.GetKeystroke(0, 0, &stroke) == 0 && stroke.virtual_key == 0x5814 && stroke.flags == 2,
        "driver releases Start after entering pause menu");
  driver.OnKeyDown(escape);
  Check(Poll(driver) == 0x2000 && driver.GetKeystroke(0, 0, &stroke) == 0 && stroke.virtual_key == 0x5801,
        "driver emits Xbox Back for menu Esc");
  driver.OnKeyUp(escape); Poll(driver); driver.GetKeystroke(0, 0, &stroke);
  for (auto bind : bbr::keyboard_bindings) {
    if (bind.key < 0x25 || bind.key > 0x28) continue;
    auto arrow = Key(bind.key);
    driver.OnKeyDown(arrow);
    Check(Poll(driver) == bind.button && driver.GetKeystroke(0, 0, &stroke) == 0 &&
          stroke.virtual_key == bind.pad_key && stroke.flags == 1,
          "driver emits direction state and Xbox keystroke for arrow key");
    driver.OnKeyUp(arrow);
    Check(Poll(driver) == 0 && driver.GetKeystroke(0, 0, &stroke) == 0 &&
          stroke.virtual_key == bind.pad_key && stroke.flags == 2,
          "driver releases arrow state and Xbox keystroke");
  }
  for(uint16_t key : {uint16_t('Q'),uint16_t('E'),uint16_t('W'),uint16_t(0x1B),
                     uint16_t(0x26),uint16_t(0xBA),uint16_t(0x70),uint16_t(0x20)}) {
    bbr::title_screen_active.store(true);
    auto title_key=Key(key);driver.OnKeyDown(title_key);
    Check(Poll(driver)==0x10 && driver.GetKeystroke(0,0,&stroke)==0 &&
          stroke.virtual_key==0x5814 && stroke.flags==1,
          "ordinary or mapped title key emits only Xbox Start");
    bbr::title_screen_active.store(false);driver.OnKeyDown(title_key);
    Check(Poll(driver)==0x10 && driver.GetKeystroke(0,0,&stroke)==X_ERROR_EMPTY,
          "held title key keeps Start without selecting or backing out of main menu");
    driver.OnKeyUp(title_key);
    Check(Poll(driver)==0 && driver.GetKeystroke(0,0,&stroke)==0 &&
          stroke.virtual_key==0x5814 && stroke.flags==2,
          "title key releases original Start after screen transition");
  }
  auto q=Key('Q');driver.OnKeyDown(q);
  Check(Poll(driver)==0 && driver.GetKeystroke(0,0,&stroke)==X_ERROR_EMPTY,
        "unmapped keys do not become Start outside the title screen");
  driver.OnKeyUp(q);
  bbr::title_screen_active.store(true);bbr::physical_controller_connected.store(true);
  driver.OnKeyDown(q);
  Check(Poll(driver)==0,"connected controller retains original title input policy");
  driver.OnKeyUp(q);
  bbr::physical_controller_connected.store(false);
  auto shift=Key(0x10);driver.OnKeyDown(shift);driver.OnKeyDown(alt_enter);
  Check(Poll(driver)==0,"title any-key mode does not hijack modifier/system shortcuts");
  driver.OnKeyUp(shift);
  allowed=false;driver.OnKeyDown(q);allowed=true;
  Check(Poll(driver)==0,"title any-key mode respects overlay/input capture");
  bbr::title_screen_active.store(false);
  driver.OnKeyDown(e); driver.OnKeyUp(e);
  Check(driver.GetKeystroke(0, 0, &stroke) == 0 && stroke.virtual_key == 0x5800 && stroke.flags == 1,
        "Xbox A key-down event");
  Check(driver.GetKeystroke(0, 0, &stroke) == 0 && stroke.flags == 2, "Xbox A key-up event");
  rex::ui::MouseEvent right(nullptr, rex::ui::MouseEvent::Button::kRight, 10, 10);
  driver.OnMouseDown(right); Check((Poll(driver) & 0x2000) != 0, "right-click sends B");
  driver.OnMouseUp(right); Check(Poll(driver) == 0, "right-click releases");

  rex::input::InputSystem system(nullptr);
  system.AddDriver(std::make_unique<PhysicalPad>());
  auto keyboard = std::make_unique<BbrKeyboardDriver>();
  keyboard->OnGotFocus(focus); keyboard->OnKeyDown(e);
  system.AddDriver(std::move(keyboard));
  rex::input::X_INPUT_STATE merged{}; system.GetState(0, &merged);
  Check(merged.gamepad.buttons == 0x9000, "installed runtime merges controller and keyboard");
  Check(system.GetKeystroke(0, 0, &stroke) == 0 && stroke.virtual_key == 0x5800,
        "idle controller does not swallow keyboard keystrokes");
  rex::input::InputSystem two_players(nullptr);
  auto first = std::make_unique<BbrKeyboardDriver>();
  auto second = std::make_unique<BbrKeyboardDriver>(1, true);
  first->OnGotFocus(focus); second->OnGotFocus(focus);
  first->OnKeyDown(e); second->OnKeyDown(e);
  auto n = Key('N'); first->OnKeyDown(n); second->OnKeyDown(n);
  auto* second_pad = second.get();
  two_players.AddDriver(std::move(first)); two_players.AddDriver(std::move(second));
  two_players.GetState(0, &merged);
  Check(merged.gamepad.buttons == 0x1000, "player-one state excludes player-two buttons");
  two_players.GetState(1, &merged);
  Check(merged.gamepad.buttons == 0x4000, "player-two state excludes player-one buttons");
  Check(two_players.GetKeystroke(1, 0, &stroke) == 0 && stroke.user_index == 1 && stroke.virtual_key == 0x5802,
        "player-two keystroke carries independent user index");
  auto unplug = Key('P'); second_pad->OnKeyDown(unplug);
  rex::input::X_INPUT_CAPABILITIES caps{};
  Check(two_players.GetCapabilities(1, 0, &caps) == X_ERROR_DEVICE_NOT_CONNECTED &&
        two_players.GetState(1, &merged) == X_ERROR_DEVICE_NOT_CONNECTED,
        "diagnostic second-pad disconnect removes its capabilities and input");
  second_pad->OnKeyDown(unplug);
  two_players.GetState(1, &merged);
  Check(merged.gamepad.buttons == 0, "second-pad reconnect has no stale held buttons");
  std::cout << "SUCCESS: keyboard/mouse state and runtime integration checks passed" << std::endl;
  std::ofstream("bbr-input-tests.passed") << "All mapping, focus, overlay and runtime merge checks passed.\n";
 } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
