#include "Menu.h"

MenuTab Menu::CurrentTab = MenuTab::ESP;

void Menu::InitializeStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 8);
    style.WindowBorderSize = 1.0f;
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    
    colors[ImGuiCol_WindowBg] = ImColor(10, 10, 15, 255);
    colors[ImGuiCol_ChildBg] = ImColor(18, 18, 26, 255);
    colors[ImGuiCol_Border] = ImColor(45, 45, 61, 255);
    colors[ImGuiCol_FrameBg] = ImColor(30, 30, 40, 255);
    colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 55, 255);
    colors[ImGuiCol_FrameBgActive] = ImColor(50, 50, 70, 255);
    colors[ImGuiCol_TitleBg] = ImColor(10, 10, 15, 255);
    colors[ImGuiCol_TitleBgActive] = ImColor(10, 10, 15, 255);
    colors[ImGuiCol_CheckMark] = ImColor(139, 92, 246, 255);
    colors[ImGuiCol_SliderGrab] = ImColor(139, 92, 246, 255);
    colors[ImGuiCol_SliderGrabActive] = ImColor(167, 139, 250, 255);
    colors[ImGuiCol_Button] = ImColor(139, 92, 246, 255);
    colors[ImGuiCol_ButtonHovered] = ImColor(167, 139, 250, 255);
    colors[ImGuiCol_ButtonActive] = ImColor(124, 58, 237, 255);
    colors[ImGuiCol_Header] = ImColor(30, 30, 40, 255);
    colors[ImGuiCol_HeaderHovered] = ImColor(139, 92, 246, 100);
    colors[ImGuiCol_HeaderActive] = ImColor(139, 92, 246, 150);
    colors[ImGuiCol_Text] = ImColor(255, 255, 255, 255);
    colors[ImGuiCol_TextDisabled] = ImColor(100, 100, 120, 255);
}

void Menu::Render() {
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 windowPos((displaySize.x - MenuStyle::Width) * 0.5f, 
                     (displaySize.y - MenuStyle::Height) * 0.5f);
    
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(MenuStyle::Width, MenuStyle::Height));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                            ImGuiWindowFlags_NoCollapse | 
                            ImGuiWindowFlags_NoResize | 
                            ImGuiWindowFlags_NoScrollbar;
    
    ImGui::Begin("ARK Cheat Menu", &Settings.IsMenuOpen, flags);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    
    // Title bar
    drawList->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + 35),
                           MenuStyle::Colors::Panel, 8.0f, ImDrawFlags_RoundCornersTop);
    drawList->AddRectFilled(ImVec2(winPos.x, winPos.y + 35),
                           ImVec2(winPos.x + winSize.x, winPos.y + 37), MenuStyle::Colors::Primary);
    
    ImGui::SetCursorPos(ImVec2(15, 8));
    ImGui::TextColored(ImColor(139, 92, 246), "ARK");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(255, 255, 255), "CHEAT");
    
    ImGui::SetCursorPos(ImVec2(winSize.x - 80, 8));
    ImGui::TextColored(ImColor(100, 100, 120), "v2.0");
    
    ImGui::SetCursorPos(ImVec2(0, 37));
    RenderTabs();
    
    ImGui::SetCursorPos(ImVec2(15, MenuStyle::TabHeight + 45));
    ImGui::BeginChild("Content", ImVec2(winSize.x - 30, winSize.y - MenuStyle::TabHeight - 60), false);
    
    switch (CurrentTab) {
        case MenuTab::ESP: RenderESPPage(); break;
        case MenuTab::Aimbot: RenderAimbotPage(); break;
        case MenuTab::Misc: RenderMiscPage(); break;
        case MenuTab::Colors: RenderColorsPage(); break;
        case MenuTab::Config: RenderConfigPage(); break;
        default: break;
    }
    
    ImGui::EndChild();
    ImGui::End();
}

void Menu::RenderTabs() {
    const char* tabNames[] = {"ESP", "Aimbot", "Misc", "Colors", "Config"};
    const int tabCount = static_cast<int>(MenuTab::Count);
    ImVec2 winSize = ImGui::GetWindowSize();
    float tabWidth = (winSize.x - 30) / tabCount;
    
    ImGui::SetCursorPos(ImVec2(15, 45));
    
    for (int i = 0; i < tabCount; i++) {
        MenuTab tab = static_cast<MenuTab>(i);
        bool isActive = (CurrentTab == tab);
        
        ImGui::PushStyleColor(ImGuiCol_Button, isActive ? MenuStyle::Colors::Primary : MenuStyle::Colors::Panel);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, MenuStyle::Colors::PrimaryHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, MenuStyle::Colors::Primary);
        ImGui::PushStyleColor(ImGuiCol_Text, isActive ? IM_COL32(255, 255, 255, 255) : MenuStyle::Colors::TextDim);
        
        if (ImGui::Button(tabNames[i], ImVec2(tabWidth - 5, MenuStyle::TabHeight - 10))) {
            CurrentTab = tab;
        }
        
        ImGui::PopStyleColor(4);
        if (i < tabCount - 1) ImGui::SameLine();
    }
}

void Menu::RenderESPPage() {
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, ImGui::GetWindowWidth() * 0.5f);
    
    BeginPanel("Player ESP");
    PurpleCheckbox("Enable Player ESP", &Settings.Visuals.DrawPlayers);
    PurpleCheckbox("Draw Names", &Settings.Visuals.RenderPlayerName);
    PurpleCheckbox("Draw Distance", &Settings.Visuals.DrawPlayerDistance);
    PurpleCheckbox("Draw Health Bar", &Settings.Visuals.DrawPlayerHP);
    PurpleCheckbox("Draw Skeleton", &Settings.Visuals.DrawPlayerBones);
    PurpleCheckbox("Draw Box", &Settings.Visuals.BOX);
    PurpleCheckbox("Draw Head Dot", &Settings.Visuals.HeadDot);
    PurpleCheckbox("Hide Team", &Settings.Visuals.HideTeamPlayers);
    EndPanel();
    
    ImGui::Spacing();
    
    BeginPanel("Dino ESP");
    PurpleCheckbox("Enable Dino ESP", &Settings.Visuals.DrawWildCreatures);
    PurpleCheckbox("Draw Tamed", &Settings.Visuals.DrawTamedCreatures);
    PurpleCheckbox("Draw Distance", &Settings.Visuals.DrawDinoDistance);
    PurpleCheckbox("Draw Health", &Settings.Visuals.DrawDinoHP);
    PurpleCheckbox("Hide Fish", &Settings.Visuals.HideFish);
    EndPanel();
    
    ImGui::NextColumn();
    
    BeginPanel("Structure ESP");
    PurpleCheckbox("Draw Turrets", &Settings.Visuals.DrawTurrets);
    PurpleCheckbox("Show Bullet Count", &Settings.Visuals.ShowBulletCount);
    PurpleCheckbox("Draw Containers", &Settings.Visuals.DrawContainers);
    PurpleCheckbox("Draw Structures", &Settings.Visuals.DrawStructure);
    EndPanel();
    
    ImGui::Spacing();
    
    BeginPanel("Radar");
    PurpleCheckbox("Enable 2D Radar", &Settings.Visuals.Radar2D);
    PurpleCheckbox("Draw Lines", &Settings.Visuals.DrawLineRadar2D);
    PurpleCheckbox("Show Enemies", &Settings.Visuals.Radar2DEnemy);
    PurpleCheckbox("Show Allies", &Settings.Visuals.Radar2DAlly);
    EndPanel();
    
    ImGui::Columns(1);
}

void Menu::RenderAimbotPage() {
    ImGui::Columns(2, nullptr, false);
    
    BeginPanel("Aimbot Settings");
    PurpleCheckbox("Enable Aimbot", &Settings.Aimbot.EnableAimbot);
    PurpleCheckbox("Silent Aim", &Settings.Aimbot.SilentAim);
    PurpleCheckbox("Visibility Check", &Settings.Aimbot.VisibleOnly);
    PurpleSliderFloat("Aimbot FOV", &Settings.Visuals.FOVSize, 1.0f, 500.0f);
    PurpleCheckbox("Draw FOV Circle", &Settings.Visuals.DrawAimFOV);
    EndPanel();
    
    ImGui::NextColumn();
    
    BeginPanel("Targeting");
    PurpleCheckbox("Target Sleepers", &Settings.Aimbot.TargetSleepers);
    PurpleCheckbox("Target Tribe", &Settings.Aimbot.TargetTribe);
    PurpleSliderFloat("Max Distance", &Settings.Aimbot.MaxDistance, 100.0f, 10000.0f, "%.0f");
    PurpleSliderFloat("Smoothing", &Settings.Aimbot.Smoothing, 0.0f, 1.0f, "%.2f");
    EndPanel();
    
    ImGui::Columns(1);
}

void Menu::RenderMiscPage() {
    ImGui::Columns(2, nullptr, false);
    
    BeginPanel("Weapon");
    PurpleCheckbox("No Sway", &Settings.Misc.NoSway);
    PurpleCheckbox("No Spread", &Settings.Misc.NoSpread);
    PurpleCheckbox("No Shake", &Settings.Misc.NoShake);
    PurpleCheckbox("Rapid Fire", &Settings.Misc.RapidFire);
    EndPanel();
    
    ImGui::Spacing();
    
    BeginPanel("Movement");
    PurpleCheckbox("Infinite Orbit Cam", &Settings.Misc.InfiniteOrbit);
    PurpleCheckbox("Long Arms", &Settings.Misc.LongArms);
    PurpleCheckbox("Instant Dino Turn", &Settings.Misc.InstantDinoTurn);
    EndPanel();
    
    ImGui::NextColumn();
    
    BeginPanel("Visual");
    PurpleCheckbox("Crosshair", &Settings.Visuals.DrawCrosshair);
    PurpleSliderFloat("Crosshair Size", &Settings.Visuals.CrosshairSize, 5.0f, 50.0f);
    PurpleCheckbox("Show FPS", &Settings.Misc.ShowFPS);
    PurpleCheckbox("Show Coordinates", &Settings.Misc.ShowXYZ);
    EndPanel();
    
    ImGui::Columns(1);
}

void Menu::RenderColorsPage() {
    ImGui::Text("Color configuration would go here...");
}

void Menu::RenderConfigPage() {
    ImGui::Text("Save/Load configuration would go here...");
}

void Menu::PurpleCheckbox(const char* label, bool* value) {
    ImGui::PushStyleColor(ImGuiCol_CheckMark, MenuStyle::Colors::Primary);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(30, 30, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImColor(40, 40
