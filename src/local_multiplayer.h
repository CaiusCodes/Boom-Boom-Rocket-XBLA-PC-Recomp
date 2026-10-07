#pragma once
#include <rex/ppc/context.h>
#include <filesystem>
#include <functional>

namespace bbr {
inline constexpr uint64_t player_two_xuid = 0xE000000000000002ull;
void ConfigureLocalMultiplayer(std::filesystem::path user_data, std::function<bool()> second_pad_connected,
                              std::function<void(uint32_t, uint32_t)> completion = {});
}

REX_EXTERN(BBR_XamUserGetSigninState);
REX_EXTERN(BBR_XamUserGetName);
REX_EXTERN(BBR_XamUserGetXUID);
REX_EXTERN(BBR_XamUserReadProfileSettings);
REX_EXTERN(BBR_XamUserWriteProfileSettings);
REX_EXTERN(BBR_XamUserCheckPrivilege);
REX_EXTERN(BBR_XamShowSigninUI);
REX_EXTERN(BBR_XamInputGetState);
