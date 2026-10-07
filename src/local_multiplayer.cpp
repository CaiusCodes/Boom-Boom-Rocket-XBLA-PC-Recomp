#include "local_multiplayer.h"
#include "player_names.h"
#include <rex/ppc.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/user_profile.h>
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <mutex>
#include <vector>

REXCVAR_DEFINE_STRING(bbr_player_one_name,"User","BBR/Players","Player one display name");
REXCVAR_DEFINE_STRING(bbr_player_two_name,"User 2","BBR/Players","Player two display name");

// Guest API buffers use Xbox big endian words; do not apply host byte order.
#define REX_LOAD_U32(address) static_cast<uint32_t>(*reinterpret_cast<rex::be<uint32_t>*>(base + (address)))
#define REX_STORE_U32(address, value) (*reinterpret_cast<rex::be<uint32_t>*>(base + (address)) = (value))
#define REX_STORE_U64(address, value) (*reinterpret_cast<rex::be<uint64_t>*>(base + (address)) = (value))

// Keep original imported APIs for player zero and unsupported user indices.
REX_EXTERN(__imp__XamUserGetSigninState);
REX_EXTERN(__imp__XamUserGetName);
REX_EXTERN(__imp__XamUserGetXUID);
REX_EXTERN(__imp__XamUserReadProfileSettings);
REX_EXTERN(__imp__XamUserWriteProfileSettings);
REX_EXTERN(__imp__XamUserCheckPrivilege);
REX_EXTERN(__imp__XamShowSigninUI);
REX_EXTERN(__imp__XamInputGetState);

namespace {
using Setting = rex::system::xam::X_USER_PROFILE_SETTING;
using Profile = rex::system::xam::UserProfile;
std::filesystem::path second_save_root;
std::function<bool()> second_connected;
std::function<void(uint32_t, uint32_t)> complete_overlapped;
std::mutex profile_mutex;
std::atomic<bool> last_signed_in{false};
bool SignedIn() { return second_connected && second_connected(); }
bool PollSignin() {
  const bool signed_in = SignedIn();
  if (last_signed_in.exchange(signed_in) != signed_in) {
    if (auto kernel = rex::system::KernelState::shared())
      kernel->BroadcastNotification(0xA, 2); // user-one sign-in changed
  }
  return signed_in;
}

void Complete(PPCContext& ctx, uint32_t overlapped, uint32_t result) {
  if (overlapped) {
    if (complete_overlapped) complete_overlapped(overlapped, result);
    else rex::system::KernelState::shared()->CompleteOverlappedImmediate(overlapped, result);
    ctx.r3.u64 = 997; // ERROR_IO_PENDING; completion/event handled by runtime.
  } else ctx.r3.u64 = result;
}

bool IsTitleBinary(uint32_t id) {
  return ((id >> 28) == 6) && ((id & 0x3F00) == 0x3F00);
}
std::filesystem::path SettingPath(uint32_t id) {
  char name[9]{}; snprintf(name, sizeof(name), "%08X", id);
  return second_save_root / name;
}
Profile::Setting* DefaultSetting(uint32_t id) {
  if ((id & 0x3F00) == 0x3F00) return nullptr;
  const auto kernel = rex::system::KernelState::shared();
  return kernel ? kernel->user_profile()->GetSetting(id) : nullptr;
}
} // namespace

void bbr::ConfigureLocalMultiplayer(std::filesystem::path user_data,
                                   std::function<bool()> second_pad_connected,
                                   std::function<void(uint32_t, uint32_t)> completion) {
  std::lock_guard lock(profile_mutex);
  second_save_root = std::move(user_data) / "5841086A" / "profile" / "Player2";
  second_connected = std::move(second_pad_connected);
  complete_overlapped = std::move(completion);
  last_signed_in.store(false);
}

REX_EXTERN(BBR_XamUserGetSigninState) {
  if (ctx.r3.u32 != 1) { __imp__XamUserGetSigninState(ctx, base); return; }
  const bool signed_in = PollSignin();
  ctx.r3.u64 = signed_in ? 1 : 0; // Local only.
}

REX_EXTERN(BBR_XamInputGetState) {
  // The title caches profiles after loading. Detect hot-plug changes during
  // normal pad polling rather than waiting for a future sign-in API request.
  __imp__XamInputGetState(ctx, base);
  PollSignin();
}

REX_EXTERN(BBR_XamUserGetName) {
  const unsigned player=ctx.r3.u32;
  if (player > 1) { __imp__XamUserGetName(ctx, base); return; }
  const uint32_t pointer = ctx.r4.u32, size = ctx.r5.u32;
  if (!pointer || !size) { ctx.r3.u64 = 0x80070057; return; }
  if (player==1 && !SignedIn()) { ctx.r3.u64 = 0x80070525; return; }
  const auto name=bbr::PlayerName(player);
  const size_t count = std::min<size_t>(name.size(), std::min(size, 16u) - 1);
  std::memcpy(base + pointer, name.data(), count); base[pointer + count] = 0;
  ctx.r3.u64 = 0;
}

REX_EXTERN(BBR_XamUserGetXUID) {
  if (ctx.r3.u32 != 1) { __imp__XamUserGetXUID(ctx, base); return; }
  const uint32_t pointer = ctx.r5.u32;
  if (!pointer) { ctx.r3.u64 = 0x80070057; return; }
  const bool available = SignedIn() && (ctx.r4.u32 & 1u);
  REX_STORE_U64(pointer, available ? bbr::player_two_xuid : 0);
  ctx.r3.u64 = available ? 0 : 0x80070525;
}

REX_EXTERN(BBR_XamUserCheckPrivilege) {
  if (ctx.r3.u32 != 1) { __imp__XamUserCheckPrivilege(ctx, base); return; }
  if (!ctx.r5.u32) { ctx.r3.u64 = 87; return; }
  REX_STORE_U32(ctx.r5.u32, 0); // No Xbox Live privileges needed for couch play.
  ctx.r3.u64 = SignedIn() ? 0 : 1317;
}

REX_EXTERN(BBR_XamShowSigninUI) {
  __imp__XamShowSigninUI(ctx, base);
  if (SignedIn()) {
    if (auto kernel = rex::system::KernelState::shared())
      kernel->BroadcastNotification(0xA, 3);
  }
}

REX_EXTERN(BBR_XamUserReadProfileSettings) {
  if (ctx.r4.u32 != 1) { __imp__XamUserReadProfileSettings(ctx, base); return; }
  const uint32_t count = ctx.r7.u32, ids = ctx.r8.u32, size_ptr = ctx.r9.u32;
  const uint32_t buffer = ctx.r10.u32, overlapped = REX_LOAD_U32(ctx.r1.u32 + 84);
  if (ctx.r5.u32 || ctx.r6.u32 || !count || count > 32 || !ids || !size_ptr) {
    Complete(ctx, overlapped, 87); return;
  }
  uint32_t needed = 8 + 40 * count;
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t id = REX_LOAD_U32(ids + 4 * i), type = id >> 28;
    if (type == 4 || type == 6) needed += (id >> 16) & 0xFFF;
  }
  const uint32_t supplied = REX_LOAD_U32(size_ptr);
  if (!buffer || supplied < needed) {
    REX_STORE_U32(size_ptr, needed); ctx.r3.u64 = 122; return;
  }
  if (!SignedIn()) { Complete(ctx, overlapped, 1317); return; }
  std::lock_guard lock(profile_mutex);
  try {
    std::memset(base + buffer, 0, needed);
    REX_STORE_U32(buffer, count); REX_STORE_U32(buffer + 4, buffer + 8);
    Profile::SettingByteStream stream(buffer, base + buffer, supplied, 8 + 40 * count);
    for (uint32_t i = 0; i < count; ++i) {
      const uint32_t id = REX_LOAD_U32(ids + 4 * i);
      auto* out = reinterpret_cast<Setting*>(base + buffer + 8 + 40 * i);
      out->user_index = 1; out->setting_id = id;
      if (IsTitleBinary(id)) {
        const auto path = SettingPath(id);
        if (!std::filesystem::exists(path)) continue; // unset -> game creates profile
        const auto size = std::filesystem::file_size(path);
        if (size > ((id >> 16) & 0xFFF)) { Complete(ctx, overlapped, 13); return; }
        std::ifstream in(path, std::ios::binary);
        std::vector<uint8_t> bytes(size);
        if (!in.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
          Complete(ctx, overlapped, 13); return;
        }
        Profile::BinarySetting setting(id, bytes);
        out->from = 2; setting.Append(&out->data, &stream);
      } else {
        auto* setting = DefaultSetting(id);
        if (!setting) { Complete(ctx, overlapped, 87); return; }
        out->from = setting->is_set ? 1 : 0;
        if (setting->is_set) setting->Append(&out->data, &stream);
      }
    }
    Complete(ctx, overlapped, 0);
  } catch (const std::exception&) { Complete(ctx, overlapped, 5); }
}

REX_EXTERN(BBR_XamUserWriteProfileSettings) {
  if (ctx.r4.u32 != 1) { __imp__XamUserWriteProfileSettings(ctx, base); return; }
  const uint32_t count = ctx.r5.u32, pointer = ctx.r6.u32, overlapped = ctx.r7.u32;
  if (!count || count > 32 || !pointer) { Complete(ctx, overlapped, 87); return; }
  if (!SignedIn()) { Complete(ctx, overlapped, 1317); return; }
  std::lock_guard lock(profile_mutex);
  try {
    // Validate the entire request before writing anything.
    for (uint32_t i = 0; i < count; ++i) {
      const auto& setting = *reinterpret_cast<const Setting*>(base + pointer + 40 * i);
      if (setting.data.type == 0xFF) continue;
      const uint32_t id = setting.setting_id, size = setting.data.binary.size;
      if (!IsTitleBinary(id) || setting.data.type != 6 || size > ((id >> 16) & 0xFFF)) {
        Complete(ctx, overlapped, 87); return;
      }
    }
    std::filesystem::create_directories(second_save_root);
    for (uint32_t i = 0; i < count; ++i) {
      const auto& setting = *reinterpret_cast<const Setting*>(base + pointer + 40 * i);
      if (setting.data.type == 0xFF) continue;
      const auto path = SettingPath(setting.setting_id);
      auto temporary = path; temporary += ".tmp";
      const uint32_t size = setting.data.binary.size, source = setting.data.binary.ptr;
      std::vector<uint8_t> bytes(size, 0);
      if (source) std::memcpy(bytes.data(), base + source, size);
      { std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); file.flush();
        if (!file) { Complete(ctx, overlapped, 5); return; }
      }
      if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        Complete(ctx, overlapped, 5); return;
      }
    }
    Complete(ctx, overlapped, 0);
  } catch (const std::exception&) { Complete(ctx, overlapped, 5); }
}
