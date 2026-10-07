#pragma once
#include <cstdint>
#include <optional>

namespace bbr {
// Native FireworkIcons values: None=0, Arrows=1, Letters (ABXY)=2.
// -1 is a PC-only policy, never passed to the game's spinner/profile.
inline int32_t ResolveRocketLabels(int32_t preference,bool controller) {
  return preference>=0 && preference<=2?preference:controller?2:1;
}
struct RocketLabelsEdit {
  std::optional<int32_t> pending;
  void Change(int32_t value) {if(value>=0 && value<=2) pending=value;}
  std::optional<int32_t> Save() {auto value=pending;pending.reset();return value;}
  void Cancel() {pending.reset();}
};
}
