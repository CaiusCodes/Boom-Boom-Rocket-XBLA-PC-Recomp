#pragma once
#include "keyboard_state.h"
#include "title_screen_state.h"
#include "menu_mouse.h"
#include <mutex>
#include <rex/input/input_driver.h>
#include <rex/ui/window_listener.h>

// A title-local player-one keyboard/mouse pad. InputSystem merges its state
// with the physical controller; no shared SDK changes or cursor capture.
class BbrKeyboardDriver final : public rex::input::InputDriver,
                                public rex::ui::WindowInputListener,
                                public rex::ui::WindowListener {
 public:
  using X_STATUS = rex::X_STATUS;
  using X_RESULT = rex::X_RESULT;
  explicit BbrKeyboardDriver(uint32_t user = 0, bool alternate = false)
      : InputDriver(nullptr, 0), user_(user), alternate_(alternate) {}
  ~BbrKeyboardDriver() override { Detach(); }
  X_STATUS Setup() override { return X_STATUS_SUCCESS; }
  void OnWindowAvailable(rex::ui::Window* window) override {
    Detach();
    attached_ = window;
    if (!window) return;
    focused_ = window->HasFocus();
    window->AddInputListener(this, 0);
    window->AddListener(this);
  }
  X_RESULT GetCapabilities(uint32_t user, uint32_t, rex::input::X_INPUT_CAPABILITIES* out) override {
    std::lock_guard lock(mutex_);
    if (!connected_) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (user != user_) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (out) {
      *out = {}; out->type = 1; out->sub_type = 1;
      out->gamepad.buttons = 0xF01F;
    }
    return X_ERROR_SUCCESS;
  }
  X_RESULT GetState(uint32_t user, rex::input::X_INPUT_STATE* out) override {
    if (user != user_) return X_ERROR_DEVICE_NOT_CONNECTED;
    const bool active = is_active();
    std::lock_guard lock(mutex_);
    if (!connected_) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (!focused_ || !active) state_.Clear();
    const uint16_t buttons = state_.Poll();
    if (buttons != last_buttons_) { ++packet_; last_buttons_ = buttons; }
    if (out) { *out = {}; out->packet_number = packet_; out->gamepad.buttons = buttons; }
    return X_ERROR_SUCCESS;
  }
  X_RESULT SetState(uint32_t user, rex::input::X_INPUT_VIBRATION*) override {
    std::lock_guard lock(mutex_);
    return connected_ && user == user_ ? X_ERROR_SUCCESS : X_ERROR_DEVICE_NOT_CONNECTED;
  }
  X_RESULT GetKeystroke(uint32_t user, uint32_t, rex::input::X_INPUT_KEYSTROKE* out) override {
    if (user != user_ && user != 255) return X_ERROR_DEVICE_NOT_CONNECTED;
    const bool active = is_active();
    std::lock_guard lock(mutex_);
    if (!connected_) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (!focused_ || !active) state_.Clear();
    bbr::PadKeyEvent event;
    if (!state_.Pop(event)) return X_ERROR_EMPTY;
    if (out) {
      *out = {}; out->virtual_key = event.key;
      out->user_index = uint8_t(user_);
      out->flags = event.down ? rex::input::X_INPUT_KEYSTROKE_KEYDOWN : rex::input::X_INPUT_KEYSTROKE_KEYUP;
    }
    return X_ERROR_SUCCESS;
  }
  void OnKeyDown(rex::ui::KeyEvent& e) override {
    // Don't turn system shortcuts such as Alt+Enter into a Start press.
    if (e.is_alt_pressed() || e.is_ctrl_pressed() || e.is_super_pressed() || e.prev_state()) return;
    // P plugs/unplugs only the isolated diagnostic pad, never player one.
    // Avoid F7, which the runtime already uses for its achievement overlay.
    if (alternate_ && static_cast<uint16_t>(e.virtual_key()) == 'P') {
      const bool active = is_active();
      std::lock_guard lock(mutex_);
      if (focused_ && active) { connected_ = !connected_; state_.Clear(); }
      return;
    }
    const auto key = TranslateKey(static_cast<uint16_t>(e.virtual_key()));
    const bool title_start = !alternate_ && bbr::IsTitleStartKey(key) &&
        bbr::title_screen_active.load(std::memory_order_relaxed) &&
        !bbr::physical_controller_connected.load(std::memory_order_relaxed);
    Set(key, true, title_start);
  }
  void OnKeyUp(rex::ui::KeyEvent& e) override { Set(TranslateKey(static_cast<uint16_t>(e.virtual_key())), false); }
  void OnMouseMove(rex::ui::MouseEvent& e) override {
    if(!alternate_ && is_active() && attached_ && bbr::mouse_menu_active.load() &&
       !bbr::song_select_active.load())
      bbr::PublishMenuMouse(bbr::ToMenuPoint(float(e.x()),float(e.y()),
        float(attached_->GetActualPhysicalWidth()),float(attached_->GetActualPhysicalHeight())),false);
  }
  void OnMouseWheel(rex::ui::MouseEvent& e) override {
    if(!alternate_ && is_active() && attached_ && bbr::song_select_active.load() &&
       bbr::mouse_menu_active.load() &&
       bbr::ToMenuPoint(float(e.x()),float(e.y()),float(attached_->GetActualPhysicalWidth()),
                        float(attached_->GetActualPhysicalHeight())).valid)
      bbr::PublishMenuWheel(e.scroll_y());
  }
  void OnMouseDown(rex::ui::MouseEvent& e) override {
    if(alternate_) return;
    if(MouseKey(e)==1 && bbr::mouse_menu_active.load() &&
       !bbr::gameplay_screen_active.load()) {
      if(is_active() && attached_) bbr::PublishMenuMouse(bbr::ToMenuPoint(float(e.x()),float(e.y()),
        float(attached_->GetActualPhysicalWidth()),float(attached_->GetActualPhysicalHeight())),true);
      return; // Select the pointed row on the guest thread BEFORE confirming.
    }
    Set(MouseKey(e), true);
  }
  void OnMouseUp(rex::ui::MouseEvent& e) override { if (!alternate_) Set(MouseKey(e), false); }
  void OnLostFocus(rex::ui::UISetupEvent&) override {
    if(!alternate_) bbr::ClearMenuMouse();
    std::lock_guard lock(mutex_); focused_ = false; state_.Clear();
  }
  void OnGotFocus(rex::ui::UISetupEvent&) override {
    std::lock_guard lock(mutex_); focused_ = true; state_.Clear();
  }
  void OnClosing(rex::ui::UIEvent&) override { Detach(); }
 private:
  uint16_t TranslateKey(uint16_t key) const {
    if (!alternate_) return key;
    // Separate diagnostic controller in the EXCLUDE_FROM_ALL multiplayer probe.
    switch (key) {
      case 'I': return 'W'; case 'K': return 'S'; case 'J': return 'A'; case 'L': return 'D';
      case 0x75: return 0x0D; // F6 = Start
      case 'U': return 'E'; case 'O': return 'B'; case 'N': return 'X'; case 'M': return 'Y';
      default: return 0;
    }
  }
  static uint16_t MouseKey(const rex::ui::MouseEvent& e) {
    return e.button() == rex::ui::MouseEvent::Button::kLeft ? 1 :
           e.button() == rex::ui::MouseEvent::Button::kRight ? 2 : 0;
  }
  void Set(uint16_t key, bool down, bool title_start = false) {
    const bool active = is_active();
    std::lock_guard lock(mutex_);
    if (!connected_ || !focused_ || !active) { state_.Clear(); return; }
    if (key) state_.Set(key, down, bbr::gameplay_screen_active.load(std::memory_order_relaxed), title_start);
  }
  void Detach() {
    if (!attached_) return;
    attached_->RemoveInputListener(this); attached_->RemoveListener(this); attached_ = nullptr;
  }
  rex::ui::Window* attached_ = nullptr;
  uint32_t user_ = 0;
  bool alternate_ = false;
  bool connected_ = true;
  bool focused_ = false;
  std::mutex mutex_;
  bbr::KeyboardState state_;
  uint16_t last_buttons_ = 0;
  uint32_t packet_ = 0;
};
