#pragma once
#include <rex/ui/keybinds.h>

namespace bbr {
// Gate the runtime dispatcher, not the game's independent input listener.
// This also blocks old config files that rebound an overlay onto another key.
inline bool DispatchRuntimeShortcut(bool enabled, rex::ui::KeyEvent& event) {
  return enabled && rex::ui::ProcessKeyEvent(event);
}
}  // namespace bbr
