#pragma once
// Proven unique in uploaded 1.26.62.1 ELF with BuildID c60fa32908019eecae39ebfe7bac5755dc35cde2.
// NO memory patches or hooks are installed by this UI library.
namespace badlion::profile {
inline constexpr const char* kMinecraftBuildId = "c60fa32908019eecae39ebfe7bac5755dc35cde2";
inline constexpr const char* kPauseMenuOpenIdaPattern = "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 57 D0 3B D5 F3 03 00 AA ? ? ? 52 ? ? ? F9 ? ? ? F8 ? ? ? F9";
inline constexpr unsigned int kExpectedMatches = 1;
}
