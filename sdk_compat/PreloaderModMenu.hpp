#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pl::modmenu {
enum class ConfigType { Toggle, SliderInt, SliderFloat, Radio, Color };
enum class ButtonBehavior { Click, Hold, Toggle };
enum class ButtonEvent { Click, Down, Up, StateChanged, Scroll };
enum class ButtonStylePreset { Keycap, Accent };
enum class ButtonIconFormat { Auto, Png, Webp, Svg, Resource };
enum class DrawCommandType { Text, Rect, Line, RectFilled, CircleFilled, TriangleFilled, Image };
struct ConfigEntry {
    std::string key, displayName;
    ConfigType type{};
    std::string defaultValue, minValue, maxValue, dependsOn;
};
struct ModuleInfo {
    std::string moduleId, displayName, description, modId;
    bool defaultEnabled{}, hideInHudEditor{};
    std::vector<ConfigEntry> configs;
    std::function<void(std::string_view,bool)> onToggle;
    std::function<void(std::string_view,std::string_view,std::string_view)> onConfigChanged;
    // Added in the preloader SDK: required for the runtime ModuleInfo ABI.
    std::function<void(std::string_view,std::string_view,bool)> onKeybind;
};
struct ButtonInfo {
    std::string buttonId, moduleId, displayName, modId, label;
    int androidKeyCode{};
    ButtonBehavior behavior{ButtonBehavior::Click};
    bool defaultVisible{true};
    ButtonStylePreset stylePreset{ButtonStylePreset::Keycap};
    uint32_t normalBgColor{},activeBgColor{},borderColor{},textColor{},activeTextColor{};
    float widthScale{},heightScale{1.f};
    ButtonIconFormat iconFormat{ButtonIconFormat::Auto};
    bool hideLabelWhenIconPresent{true};
    std::vector<unsigned char> iconData;
    // Added in the preloader SDK: omission shifts the onEvent callback offset.
    std::vector<unsigned char> activeIconData;
    std::function<void(std::string_view,ButtonEvent,float)> onEvent;
};
struct DrawCommand {
    DrawCommandType type{};
    float x{},y{},w{},h{},x3{},y3{};
    uint32_t color{};
    float size{};
    std::string text,fontId,imageId;
};
bool registerModule(const ModuleInfo&);
void unregisterModule(std::string_view);
void setModuleEnabled(std::string_view,bool);
void submitDrawCommands(std::string_view,std::span<const DrawCommand>);
bool registerButton(const ButtonInfo&);
void unregisterButton(std::string_view);
}

// Match the public Levi preloader-android ModuleInfo and ButtonInfo layout.
// Removing any of these fields changes the ABI and corrupts STL callback data.
static_assert(offsetof(pl::modmenu::ModuleInfo, onKeybind) >
              offsetof(pl::modmenu::ModuleInfo, onConfigChanged));
static_assert(offsetof(pl::modmenu::ButtonInfo, onEvent) >
              offsetof(pl::modmenu::ButtonInfo, activeIconData));
