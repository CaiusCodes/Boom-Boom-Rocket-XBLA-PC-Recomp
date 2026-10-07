#pragma once
#include <string>
#include <string_view>
#include <rex/cvar.h>

namespace bbr {
inline std::string NormalizePlayerName(std::string_view input, unsigned player) {
  std::string result;
  for (unsigned char c : input) {
    if ((c>='A'&&c<='Z') || (c>='a'&&c<='z') || (c>='0'&&c<='9') || c==' ' || c=='_' || c=='-')
      result+=char(c);
    if(result.size()==15) break;
  }
  const auto first=result.find_first_not_of(' ');
  if(first==std::string::npos) return player==0?"User":"User 2";
  return result.substr(first,result.find_last_not_of(' ')-first+1);
}
inline std::string PlayerName(unsigned player) {
  return NormalizePlayerName(rex::cvar::GetFlagByName(player==0?"bbr_player_one_name":"bbr_player_two_name"),player);
}
} // namespace bbr
