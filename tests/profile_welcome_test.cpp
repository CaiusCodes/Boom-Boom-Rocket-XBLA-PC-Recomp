#include "src/profile_welcome.h"
#include "src/native_help.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

static void Check(bool result, const char* label) {
  if (!result) throw std::runtime_error(label);
  std::cout << "PASS: " << label << '\n';
}
int main(int argc, char** argv) {
  try {
    const std::string crc_fixture = "123456789";
    Check(bbr::ProfileScriptCrc({reinterpret_cast<const uint8_t*>(crc_fixture.data()), crc_fixture.size()}) == 0xCBF43926,
          "standard CRC32 fixture");
    bbr::NativeHelpReader public_help;
    for(auto source : {bbr::help_menu_source,bbr::keyboard_help_source,bbr::timing_help_source}) {
      public_help.source=source; public_help.position=0;
      std::string loaded; std::array<uint8_t,512> buffer{}; uint32_t size=0;
      while(public_help.Read(buffer,size) && size)
        loaded.append(reinterpret_cast<char*>(buffer.data()),size);
      Check(loaded==source && size==0,"authored help reader supports variable lengths and EOF");
    }
    public_help.End();
    std::array<uint8_t,512> help_buffer{}; uint32_t help_size=456;
    Check(!public_help.Read(help_buffer,help_size) && help_size==456,
          "original reader retained after end of help load");
    bbr::ProfileWelcomeReader reader;
    std::vector<uint8_t> invalid(2000);
    reader.Begin("Profile\\Load.lua", invalid);
    Check(!reader.active, "unknown compressed scripts are never patched");
    for (size_t chunk_size : {1u, 3u, 511u, 512u, 513u, 4381u}) {
      std::vector<uint8_t> script(4381, 0xAA), expected = script;
      for (auto edit : reader.edits)
        for (uint32_t b = 0; b < 4; ++b) {
          script[edit.offset + b] = uint8_t(edit.expected >> (b * 8));
          expected[edit.offset + b] = uint8_t(edit.replacement >> (b * 8));
        }
      reader.active = true; reader.position = 0;
      for (size_t pos = 0; pos < script.size(); pos += chunk_size)
        reader.Patch({script.data() + pos, std::min(chunk_size, script.size() - pos)});
      Check(script == expected, "all chunk boundaries preserve every non-patch byte");
    }
    Check(reader.edits[0].replacement == reader.Jump(19) &&
          reader.edits[1].replacement == reader.Jump(40) &&
          reader.edits[2].replacement == reader.Jump(1), "jumps target original setup/save/wait flow");
    if (argc > 1) {
      // Optional PRIVATE integration fixture: never packaged or required by public tests.
      std::ifstream in(argv[1], std::ios::binary);
      std::vector<uint8_t> image((std::istreambuf_iterator<char>(in)), {});
      Check(image.size() >= 0x307007 + 2000, "private guest image loaded");
      bbr::NativeHelpReader help;
      auto main_stream=std::span<const uint8_t>(image).subspan(0x305099,1115);
      auto controls_stream=std::span<const uint8_t>(image).subspan(0x3042A2,1479);
      Check(help.Begin("Options\\Main.lua",main_stream,controls_stream)==0 &&
            help.source==bbr::help_menu_source,"supported help menu replacement selected");
      Check(help.Begin("Options\\Keyboard.lua",main_stream,controls_stream)==0x822F0611 &&
            help.source==bbr::keyboard_help_source,"virtual keyboard route uses supported stock loader");
      Check(help.Begin("Options\\Calibration.lua",main_stream,controls_stream)==0x822F0611 &&
            help.source==bbr::timing_help_source,"virtual calibration route uses supported stock loader");
      for (auto source : {bbr::help_menu_source,bbr::keyboard_help_source,bbr::timing_help_source}) {
        help.source=source; help.position=0;
        std::string loaded; std::array<uint8_t,512> buffer{}; uint32_t size=0;
        while(help.Read(buffer,size) && size) loaded.append(reinterpret_cast<char*>(buffer.data()),size);
        Check(loaded==source && size==0,"replacement streams all chunks and explicit EOF without truncation");
      }
      help.End(); uint32_t size=123; std::array<uint8_t,512> buffer{};
      Check(!help.Read(buffer,size) && size==123,"unselected scripts retain original reader");
      image[0x3042A2+900]^=1;
      help.Begin("Options\\Main.lua",main_stream,controls_stream);
      Check(help.source.empty(),"altered controls stream disables entire added keyboard route");
      auto compressed = std::span<const uint8_t>(image).subspan(0x307007, 2000);
      reader.Begin("Profile\\Load.lua", compressed);
      Check(reader.active, "supported embedded script passes full-stream fingerprint");
      reader.Begin("Profile\\Other.lua", compressed);
      Check(!reader.active, "other screens are excluded");
      image[0x307007 + 1500] ^= 1;
      reader.Begin("Profile\\Load.lua", compressed);
      Check(!reader.active, "single changed byte prevents all patches");
    }
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
