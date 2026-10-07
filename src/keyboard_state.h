#pragma once
#include <array>
#include <cstdint>
#include <deque>

namespace bbr {
// Host virtual-key codes -> Xbox button bits. Mouse buttons share this table.
struct KeyBinding { uint16_t key, button, pad_key; };
inline constexpr KeyBinding keyboard_bindings[] = {
    {'W', 1, 0x5810}, {'S', 2, 0x5811}, {'A', 4, 0x5812}, {'D', 8, 0x5813},
    {0x26, 1, 0x5810}, {0x28, 2, 0x5811}, // Up / Down arrows
    {0x25, 4, 0x5812}, {0x27, 8, 0x5813}, // Left / Right arrows
    {0x0D, 0x10, 0x5814}, {0x20, 0x10, 0x5814}, // Enter / Space = Start
    {'E', 0x1000, 0x5800}, {1, 0x1000, 0x5800}, // E / left mouse = A
    {'B', 0x2000, 0x5801}, {0x1B, 0x2000, 0x5801},
    {8, 0x2000, 0x5801}, {2, 0x2000, 0x5801},   // Esc / Backspace / right mouse = B
    {'X', 0x4000, 0x5802}, {'Y', 0x8000, 0x5803}};

struct PadKeyEvent { uint16_t key; bool down; };
// Owned under the driver's mutex. OS repeats don't manufacture rhythm hits;
// overlapping aliases are held until the final associated key is released.
class KeyboardState {
 public:
  void Set(uint16_t key, bool down, bool gameplay = false, bool title_start = false) {
    if (key >= keys_.size() || keys_[key] == down) return;
    const uint16_t before = Buttons();
    // Latch Esc's meaning at key-down. A pause/menu transition before key-up
    // must release the original button, not manufacture Back or leave Start held.
    if (key == 0x1B && down) escape_button_ = gameplay ? 0x10 : 0x2000;
    keys_[key] = down;
    // Any-key Start is latched until key-up, even after the main menu opens.
    // The same held key must not become A, Back or a direction mid-press.
    title_keys_[key] = down && title_start;
    const uint16_t after = Buttons();
    pending_ |= after & ~before;
    uint16_t changed = before ^ after;
    for (auto bind : keyboard_bindings) {
      if (!(changed & bind.button)) continue;
      changed &= ~bind.button;
      if (events_.size() == 128) events_.pop_front();
      events_.push_back({bind.pad_key, (after & bind.button) != 0});
    }
  }
  uint16_t Buttons() const {
    uint16_t buttons = 0;
    for (bool held : title_keys_) if (held) { buttons |= 0x10; break; }
    for (auto bind : keyboard_bindings)
      if (keys_[bind.key] && !title_keys_[bind.key])
        buttons |= bind.key == 0x1B ? escape_button_ : bind.button;
    return buttons;
  }
  uint16_t Poll() {
    const uint16_t buttons = Buttons() | pending_;
    pending_ = 0; // Preserve a tap that began and ended between guest polls.
    return buttons;
  }
  bool Pop(PadKeyEvent& event) {
    if (events_.empty()) return false;
    event = events_.front(); events_.pop_front(); return true;
  }
  void Clear() { keys_.fill(false); title_keys_.fill(false); pending_ = 0; events_.clear(); escape_button_ = 0x2000; }
 private:
  std::array<bool, 256> keys_{};
  std::array<bool, 256> title_keys_{};
  uint16_t pending_ = 0;
  uint16_t escape_button_ = 0x2000;
  std::deque<PadKeyEvent> events_;
};
} // namespace bbr
