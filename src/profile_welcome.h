#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace bbr {
// Patch only the known embedded Profile\Load.lua, while its bytecode is read.
// Keep Profile.Setup, BeginSave, the saving wait, and corrupt-save recovery.
// The complete compressed stream is fingerprinted BEFORE any instruction changes.
inline uint32_t ProfileScriptCrc(std::span<const uint8_t> bytes) {
  uint32_t crc = 0xFFFFFFFFu;
  for (uint8_t byte : bytes) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i)
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

struct ProfileWelcomeReader {
  struct Edit { uint32_t offset, expected, replacement; };
  static constexpr uint32_t Jump(int distance) {
    return (uint32_t(distance + 131071) << 14) | 22u;
  }
  static constexpr std::array<Edit, 3> edits{{
      {2966, 0x000580C5u, (131090u << 14) | 22u}, // Message[48] -> [68]: Setup
      {3066, 0x801A8016u, (131111u << 14) | 22u}, // Message[73] -> [114]: BeginSave
      {3306, 0x0009C105u, (131072u << 14) | 22u}, // [133] -> [135]: no button sound
  }};
  bool active = false;
  uint32_t position = 0;

  void Begin(std::string_view name, std::span<const uint8_t> compressed) {
    position = 0;
    active = name == "Profile\\Load.lua" && compressed.size() == 2000 &&
             ProfileScriptCrc(compressed) == 0x45D8495Bu;
  }
  void Patch(std::span<uint8_t> chunk) {
    if (!active) return;
    // The native callback reads 512 bytes per call. Patch bytewise as well so
    // instruction words remain correct even if a read is split at a boundary.
    for (const auto& edit : edits)
      for (uint32_t byte = 0; byte < 4; ++byte) {
        const uint32_t offset = edit.offset + byte;
        if (offset >= position && offset - position < chunk.size())
          chunk[offset - position] = uint8_t(edit.replacement >> (byte * 8));
      }
    position += uint32_t(chunk.size());
  }
};
inline thread_local ProfileWelcomeReader profile_welcome_reader;
} // namespace bbr
