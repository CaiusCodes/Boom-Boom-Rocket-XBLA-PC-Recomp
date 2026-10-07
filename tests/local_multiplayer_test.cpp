#include "local_multiplayer.h"
#include "player_names.h"
#include <rex/ppc.h>
#include <rex/system/xam/user_profile.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

int forwarded = 0;
#define ORIGINAL(name) REX_EXTERN(__imp__##name) { ++forwarded; ctx.r3.u64 = 0x1234; }
ORIGINAL(XamUserGetSigninState)
ORIGINAL(XamUserGetName)
ORIGINAL(XamUserGetXUID)
ORIGINAL(XamUserReadProfileSettings)
ORIGINAL(XamUserWriteProfileSettings)
ORIGINAL(XamUserCheckPrivilege)
ORIGINAL(XamShowSigninUI)
ORIGINAL(XamInputGetState)
static void Check(bool good, const char* label) {
  if (!good) throw std::runtime_error(label);
  std::cout << "PASS: " << label << '\n';
}
int main(int argc, char** argv) {
  try {
    if (argc != 2) throw std::runtime_error("Pass a NEW isolated fixture directory");
    const auto root = std::filesystem::absolute(argv[1]);
    if (std::filesystem::exists(root)) throw std::runtime_error("Existing fixture preserved");
    bool connected = true; uint32_t completed_address = 0, completed_result = ~0u;
    bbr::ConfigureLocalMultiplayer(root, [&] { return connected; }, [&](uint32_t a, uint32_t r) {
      completed_address = a; completed_result = r;
    });
    std::vector<uint8_t> memory(16384); auto* base = memory.data(); PPCContext ctx{};
    auto word = [&](uint32_t a, uint32_t v) { *reinterpret_cast<rex::be<uint32_t>*>(base + a) = v; };
    auto read = [&](uint32_t a) { return uint32_t(*reinterpret_cast<rex::be<uint32_t>*>(base + a)); };
    ctx.r3.u32 = 0; BBR_XamUserGetSigninState(ctx, base);
    Check(ctx.r3.u32 == 0x1234 && forwarded == 1, "player one delegates unchanged to runtime");
    ctx.r3.u32 = 0; BBR_XamInputGetState(ctx, base);
    Check(ctx.r3.u32 == 0x1234 && forwarded == 2, "pad polling preserves original input results while checking sign-in changes");
    ctx.r3.u32 = 1; BBR_XamUserGetSigninState(ctx, base);
    Check(ctx.r3.u32 == 1, "second pad supplies a local signed-in identity");
    ctx.r3.u32 = 1; ctx.r4.u32 = 256; ctx.r5.u32 = 16; BBR_XamUserGetName(ctx, base);
    Check(ctx.r3.u32 == 0 && std::string(reinterpret_cast<char*>(base + 256)) == "User 2", "default second name");
    rex::cvar::SetFlagByName("bbr_player_two_name","Rocket Two");
    ctx.r3.u32=1; ctx.r4.u32=256; ctx.r5.u32=16; BBR_XamUserGetName(ctx,base);
    Check(ctx.r3.u32==0 && std::string(reinterpret_cast<char*>(base+256))=="Rocket Two","custom display name");
    ctx.r3.u32=0; ctx.r4.u32=256; ctx.r5.u32=16; BBR_XamUserGetName(ctx,base);
    Check(ctx.r3.u32==0 && std::string(reinterpret_cast<char*>(base+256))=="User","default first name");
    Check(bbr::NormalizePlayerName("   ",1)=="User 2" && bbr::NormalizePlayerName("  Rocket!  ",0)=="Rocket","safe native name characters and empty fallback");
    ctx.r3.u32 = 1; ctx.r4.u32 = 3; ctx.r5.u32 = 288; BBR_XamUserGetXUID(ctx, base);
    Check(ctx.r3.u32 == 0 && uint64_t(*reinterpret_cast<rex::be<uint64_t>*>(base + 288)) == bbr::player_two_xuid,
          "stable distinct offline XUID uses Xbox byte order");

    // The ninth API argument is at r1+84, not in r11.
    ctx.r1.u32 = 128; word(212, 0); word(320, 0x63E83FFF); word(324, 0);
    auto read_request = [&](uint32_t buffer) {
      ctx.r4.u32 = 1; ctx.r5.u32 = ctx.r6.u32 = 0; ctx.r7.u32 = 1;
      ctx.r8.u32 = 320; ctx.r9.u32 = 324; ctx.r10.u32 = buffer;
      BBR_XamUserReadProfileSettings(ctx, base);
    };
    read_request(0); Check(ctx.r3.u32 == 122 && read(324) == 1048, "profile size negotiation");
    read_request(512); Check(ctx.r3.u32 == 0 && read(520) == 0, "missing player-two save is unset");
    Check(read(512) == 1 && read(516) == 520 && read(528) == 1 && read(536) == 0x63E83FFF,
          "profile descriptor layout and guest pointers");
    auto* setting = reinterpret_cast<rex::system::xam::X_USER_PROFILE_SETTING*>(base + 2048);
    setting->setting_id = 0x63E83FFF; setting->data.type = 6;
    setting->data.binary.size = 5; setting->data.binary.ptr = 3000;
    const uint8_t payload[]{1, 3, 5, 7, 9}; std::memcpy(base + 3000, payload, 5);
    ctx.r4.u32 = 1; ctx.r5.u32 = 1; ctx.r6.u32 = 2048; ctx.r7.u32 = 0;
    BBR_XamUserWriteProfileSettings(ctx, base);
    Check(ctx.r3.u32 == 0 && std::filesystem::file_size(root / "5841086A/profile/Player2/63E83FFF") == 5,
          "player-two save created in separate portable directory");
    Check(!std::filesystem::exists(root / "5841086A/profile/User"), "player-one data untouched");
    read_request(512); const auto data = read(556);
    Check(ctx.r3.u32 == 0 && read(520) == 2 && read(552) == 5 && !std::memcmp(base + data, payload, 5),
          "player-two binary save roundtrip");
    bbr::ConfigureLocalMultiplayer(root, [&] { return connected; }, [&](uint32_t a, uint32_t r) {
      completed_address = a; completed_result = r;
    });
    read_request(512); Check(ctx.r3.u32 == 0 && read(552) == 5, "saved data reloads after reconfiguration");
    setting->data.binary.size = 1001;
    ctx.r4.u32 = 1; ctx.r5.u32 = 1; ctx.r6.u32 = 2048; ctx.r7.u32 = 0;
    BBR_XamUserWriteProfileSettings(ctx, base);
    Check(ctx.r3.u32 == 87 && std::filesystem::file_size(root / "5841086A/profile/Player2/63E83FFF") == 5,
          "oversized data rejected without replacing save");
    setting->data.binary.size = 5; ctx.r4.u32 = 1; ctx.r5.u32 = 1; ctx.r6.u32 = 2048; ctx.r7.u32 = 4096;
    BBR_XamUserWriteProfileSettings(ctx, base);
    Check(ctx.r3.u32 == 997 && completed_address == 4096 && completed_result == 0,
          "asynchronous save completes and reports IO_PENDING");
    word(212, 4096); read_request(512);
    Check(ctx.r3.u32 == 997 && completed_result == 0, "ninth-argument async load completion");
    connected = false; ctx.r3.u32 = 1; BBR_XamUserGetSigninState(ctx, base);
    Check(ctx.r3.u32 == 0, "disconnected second controller signs out");
    ctx.r3.u32 = 1; ctx.r4.u32 = 3; ctx.r5.u32 = 288; BBR_XamUserGetXUID(ctx, base);
    Check(ctx.r3.u32 != 0 && uint64_t(*reinterpret_cast<rex::be<uint64_t>*>(base + 288)) == 0,
          "signed-out identity cannot be reused");
    read_request(512); Check(ctx.r3.u32 == 997 && completed_result == 1317, "disconnected async load completes with no-user");
    std::cout << "SUCCESS: local identity and independent profile API checks passed\n";
  } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
