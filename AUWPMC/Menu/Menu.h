#pragma once
#include "../Cheat.h"

namespace MenuStyle {
    constexpr float Width = 850.0f;
    constexpr float Height = 600.0f;
    constexpr float TabHeight = 45.0f;
    
    namespace Colors {
        constexpr ImU32 Background = IM_COL32(10, 10, 15, 255);
        constexpr ImU32 Panel = IM_COL32(18, 18, 26, 255);
        constexpr ImU32 PanelHover = IM_COL32(25, 25, 35, 255);
        constexpr ImU32 Primary = IM_COL32(139, 92, 246, 255);
        constexpr ImU32 PrimaryHover = IM_COL32(167, 139, 250, 255);
        constexpr ImU32 Text = IM_COL32(255, 255, 255, 255);
        constexpr ImU32 TextDim = IM_COL32(180, 180, 200, 255);
        constexpr ImU32 Border = IM_COL32(45, 45, 61, 255);
    }
}

enum class MenuTab { ESP, Aimbot, Misc, Colors, Config, Count };

class Menu {
public:
    static void InitializeStyle();
    static void Render();
    
private:
    static void RenderTabs();
    static void RenderESPPage();
    static void RenderAimbotPage();
    static void RenderMiscPage();
    static void RenderColorsPage();
    static void RenderConfigPage();
    
    static void PurpleCheckbox(const char* label, bool* value);
    static void PurpleSliderFloat(const char* label, float* value, float min, float max, const char* format = "%.1f");
    static void PurpleSliderInt(const char* label, int* value, int min, int max);
    static void SectionHeader(const char* title);
    static void BeginPanel(const char* title);
    static void EndPanel();
    
    static MenuTab CurrentTab;
};

extern Menu g_Menu;
