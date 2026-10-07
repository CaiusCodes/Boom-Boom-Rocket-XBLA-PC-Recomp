#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>

namespace bbr {
// Signed hit-window adjustment, not an input delay or audio buffer setting.
// Positive values accept later hits; negative values accept earlier hits.
// Latch at the native judgement manager's reset so pause-menu edits cannot
// abruptly move the clock of a match already in progress.
struct TimingCalibration {
  static constexpr int minimum_ms = -200, maximum_ms = 200, step_ms = 10;
  static constexpr int zero_index = 20, item_count = 41;
  int session_ms = 0; // Accessed only by the guest game thread.
  static int Normalize(int value) {
    value = std::clamp(value, minimum_ms, maximum_ms);
    return int(std::round(value / double(step_ms))) * step_ms;
  }
  static int Index(int value) { return (Normalize(value)-minimum_ms)/step_ms; }
  static int Value(int index) {
    return minimum_ms + std::clamp(index,0,item_count-1)*step_ms;
  }
  void Begin(int configured_ms) { session_ms = Normalize(configured_ms); }
  double Adjust(double seconds) const {
    // At zero preserve the original register exactly, including signed zero.
    if (session_ms == 0 || !std::isfinite(seconds)) return seconds;
    return double(float(seconds - session_ms / 1000.0));
  }
};
inline TimingCalibration timing_calibration;
} // namespace bbr
