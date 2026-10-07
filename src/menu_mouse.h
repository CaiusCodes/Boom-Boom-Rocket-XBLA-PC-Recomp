#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string_view>

namespace bbr {
// Popup scripts use a separate native object list; the underlying screen and
// its selection remain alive until the actual non-modal screen changes.
inline bool IsModalUiScript(std::string_view name) {
  return name.starts_with("Popups\\");
}
inline uint32_t NextSpinnerChoice(uint32_t index, uint32_t count) {
  return count > 1 && index < count - 1 ? index + 1 : 0;
}
// One mouse-originated native Right event may wrap its specific Settings row.
// Keyboard/controller Right events and unrelated widgets keep native limits.
struct SpinnerClickCycle {
  uint32_t object = 0;
  void Arm(uint32_t target) { object = target; }
  bool Take(uint32_t target) {
    if (!target || target != object) return false;
    object = 0; return true;
  }
  void Clear() { object = 0; }
};
struct MenuPoint { float x=0,y=0; bool valid=false; };
// MouseEvent coordinates and client dimensions are both physical pixels.
// The guest draws a centred 640x480 UI inside its 16:9 output viewport.
inline MenuPoint ToMenuPoint(float x,float y,float width,float height) {
  if(width<=0 || height<=0) return {};
  const float scale=std::min(width/1280.f,height/720.f);
  const float left=(width-1280.f*scale)*.5f,top=(height-720.f*scale)*.5f;
  if(x<left || y<top || x>=width-left || y>=height-top) return {};
  return {(x-width*.5f)/(1.5f*scale)+320.f,(y-top)/(1.5f*scale),true};
}
struct MenuMouseEvent { MenuPoint point; bool dirty=false,click=false; };
inline std::atomic<bool> mouse_menu_active{false};
inline std::mutex menu_mouse_mutex;
inline MenuMouseEvent menu_mouse_event;
inline int menu_wheel_delta=0;
inline void PublishMenuWheel(int delta) {
  std::lock_guard lock(menu_mouse_mutex);
  menu_wheel_delta=int(std::clamp(int64_t(menu_wheel_delta)+delta,int64_t(-360),int64_t(360)));
}
inline int TakeMenuWheel() {
  std::lock_guard lock(menu_mouse_mutex);
  const int delta=menu_wheel_delta;menu_wheel_delta=0;return delta;
}
// A wheel notch is a single native menu action, never a held D-pad button.
// Limit high-resolution/fast-wheel bursts and discard old inertial movement.
struct SongWheel {
  int remainder=0,steps=0;
  int64_t next_ms=0,expires_ms=0;
  void Add(int delta,int64_t now) {
    if(!delta) return;
    if(now>expires_ms || (steps && ((steps>0)!=(delta>0)))) {steps=0;remainder=0;}
    remainder+=std::clamp(delta,-360,360);
    steps=std::clamp(steps+remainder/120,-3,3);remainder%=120;
    expires_ms=now+450;
  }
  int Next(int64_t now) {
    if(now>expires_ms) {steps=0;remainder=0;}
    if(!steps || now<next_ms) return 0;
    const int direction=steps>0?1:-1;steps-=direction;next_ms=now+150;
    return direction;
  }
};
inline void PublishMenuMouse(MenuPoint point,bool click) {
  std::lock_guard lock(menu_mouse_mutex);
  menu_mouse_event={point,true,click || menu_mouse_event.click};
}
inline MenuMouseEvent TakeMenuMouse() {
  std::lock_guard lock(menu_mouse_mutex);
  auto event=menu_mouse_event; menu_mouse_event={}; return event;
}
inline void ClearMenuMouse() { TakeMenuMouse(); TakeMenuWheel(); mouse_menu_active.store(false); }
} // namespace bbr
