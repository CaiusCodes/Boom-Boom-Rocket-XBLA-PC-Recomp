#include "src/title_screen_state.h"
#include <cassert>
#include <iostream>

int main() {
  assert(!bbr::title_screen_active.load());
  assert(bbr::IsTitleScreen("Main\\TitleScreen.lua"));
  for (const auto screen : {"", "Main\\StartScreen.lua", "Main\\MainMenu.lua",
       "Main\\Loading.lua", "Ingame\\Hud.lua", "Ingame\\Pause.lua",
       "Options\\Options.lua", "Main\\TitleScreen.lua.extra"}) {
    assert(!bbr::IsTitleScreen(screen));
  }
  bbr::title_screen_active.store(bbr::IsTitleScreen("Main\\TitleScreen.lua"));
  assert(bbr::title_screen_active.load());
  bbr::title_screen_active.store(bbr::IsTitleScreen("Ingame\\Hud.lua"));
  assert(!bbr::title_screen_active.load());
  std::cout << "PASS: title identity, non-title screens and published transitions\n";
}
