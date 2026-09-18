#pragma once
#include "imgui\imgui.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_impl_dx11.h"
#include "imgui\imgui_internal.h"
#include "Cheat.h"
#include <random>
#include <string>
#include "SDK/SDK.h"
#include "Cheat.h"
#include "Dump.h"

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(48, 122);

extern void LoadConfig();
void LoadConfig();
void SaveConfig();
std::wstring random_string(int length)
{
    std::wstring s;
    for (int i = 0; i < length; ++i)
    {
        int random_char = dis(gen);
        s += (wchar_t)random_char;
    }
    return s;
}
float CachedFramerates[1000];
int AimBone = 4; // Default chest, head is 8
int PossibleDinos = 1;
int PossibleAimKey = 6;
int PossibleMenuKey = 2;
const char* CurrentContainer = "Supply Crate";
static const char* SettingFilter[] = { "Setting 1", "Setting 2", "Setting 3", "Setting 4", "Setting 5" };
const char* Setting_Current = "Setting 1";
const char* GetCurrentSetting()
{
    return Setting_Current;
}
static int tabs;
static int subtabs;
ImFont* zzzz = nullptr;
ImFont* icons = nullptr;
ImVec2 pos;
ImDrawList* draw;
static float PlayerColor1[4] = { 1.0f, 0.1f, 0.1f, 255 };
static float CrosshairColor1[4] = { 0.0f, 1.0f, 0.0f, 255 };
static float CrosshairColorFOV1[4] = { 0.0f, 1.0f, 0.0f, 255 };
static float TamedDinoColor1[4] = { 9.0f, 1.0f, 0.0f, 255 };
static float WildDinoColor1[4] = { 0.0f, 1.0f, 1.0f, 255 };
static float ContainerColor1[4] = { 0.9f, 1.0f, 0.0f, 255 };
static float StructureColor1[4] = { 1.0f, 1.0f, 1.0f, 255 };
static float TurretColor1[4] = { 1.0f, 0.0f, 0.0f, 255 };
static float FilteredDinoColor1[4] = { 0.9f, 0.0f, 1.0f, 255 };
float MaxArr(float arr[], int n)
{
    int i;
    float max = arr[0];
    for (i = 1; i < n; i++)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }
        return max;
    }
}
float MinArr(float arr[], int n)
{
    int i;
    float min = arr[0];
    for (i = 1; i < n; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
        return min;
    }
}
typedef struct _D3DXMATRIX
{
    union
    {
        struct
        {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DXMATRIX;
D3DXMATRIX Matrix(FRotator Rotation, FVector Origin = FVector(0, 0, 0))
{
    float radPitch = (Rotation.Pitch * M_PI / 180.f);
    float radYaw = (Rotation.Yaw * M_PI / 180.f);
    float radRoll = (Rotation.Roll * M_PI / 180.f);
    float SP = sinf(radPitch);
    float CP = cosf(radPitch);
    float SY = sinf(radYaw);
    float CY = cosf(radYaw);
    float SR = sinf(radRoll);
    float CR = cosf(radRoll);
    D3DXMATRIX matrix;
    matrix.m[0][0] = CP * CY;
    matrix.m[0][1] = CP * SY;
    matrix.m[0][2] = SP;
    matrix.m[0][3] = 0.f;
    matrix.m[1][0] = SR * SP * CY - CR * SY;
    matrix.m[1][1] = SR * SP * SY + CR * CY;
    matrix.m[1][2] = -SR * CP;
    matrix.m[1][3] = 0.f;
    matrix.m[2][0] = -(CR * SP * CY + SR * SY);
    matrix.m[2][1] = CY * SR - CR * SP * SY;
    matrix.m[2][2] = CR * CP;
    matrix.m[2][3] = 0.f;
    matrix.m[3][0] = Origin.X;
    matrix.m[3][1] = Origin.Y;
    matrix.m[3][2] = Origin.Z;
    matrix.m[3][3] = 1.f;
    return matrix;
}
bool W2S(FVector WorldLocation, FVector2D& ScreenLocation)
{
    if (WorldLocation == 0.f) return false;
    auto Location = Cache.LPC->PlayerCameraManager->GetCameraLocation();
    auto Rotation = Cache.LPC->PlayerCameraManager->GetCameraRotation();
    D3DXMATRIX tempMatrix = Matrix(Rotation);
    FVector vAxisX, vAxisY, vAxisZ;
    vAxisX = FVector(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
    vAxisY = FVector(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
    vAxisZ = FVector(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);
    float w = tempMatrix.m[3][0] * WorldLocation.X + tempMatrix.m[3][1] * WorldLocation.Y + tempMatrix.m[3][2] * WorldLocation.Z + tempMatrix.m[3][3];
    if (w < 0.01) return false;
    FVector vDelta = WorldLocation - Location;
    FVector vTransformed = FVector(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));
    if (vTransformed.Z < 1.0f) vTransformed.Z = 1.f;
    float fovAngle = Cache.LPFOV;
    float screenCenterX = Cache.WindowSizeX / 2;
    float screenCenterY = Cache.WindowSizeY / 2;
    ScreenLocation.X = (screenCenterX + vTransformed.X * (screenCenterX / (float)tan(fovAngle * M_PI / 360)) / vTransformed.Z);
    ScreenLocation.Y = (screenCenterY - vTransformed.Y * (screenCenterX / (float)tan(fovAngle * M_PI / 360)) / vTransformed.Z);
    if (ScreenLocation.X < -50 || ScreenLocation.X >(Cache.WindowSizeX + 250)) return false;
    if (ScreenLocation.Y < -50 || ScreenLocation.Y >(Cache.WindowSizeY + 250)) return false;
    return true;
}
uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets) {
    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
        addr = *(uintptr_t*)addr;
        addr += offsets[i];
    }
    return addr;
}
void DumpClass()
{
    OffsetDumper::DumpRequestedOffsets();
}
float calcDistance(int x1, int y1, int x2, int y2)
{
    // Calculating distance
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) * 1.0);
}
char text_buffer[256] = ""; // buffer pour stocker le texte entré par l'utilisateur
void drawImGuiWindow()
{
    ImGui::Begin("Ma fenêtre ImGui");

    ImGui::InputText("Entrez du texte ici", text_buffer, sizeof(text_buffer));

    ImGui::End();
}
void CheatMenu()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;
    style->Colors[ImGuiCol_Text] = ImColor(50, 150, 255, 255);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.0f, 0.0263f, 0.0357f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImColor(25, 25, 25);
    style->Colors[ImGuiCol_ChildBg] = ImColor(20, 20, 20);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.080f, 0.080f, 0.080f, 0.940f);
    style->Colors[ImGuiCol_Border] = ImColor(57, 57, 57);
    style->Colors[ImGuiCol_BorderShadow] = ImColor(1, 1, 1);
    style->Colors[ImGuiCol_FrameBg] = ImColor(40, 40, 40);
    style->Colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 40);
    style->Colors[ImGuiCol_FrameBgActive] = ImColor(40, 40, 40);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.102f, 0.090f, 0.122f, 1.000f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.102f, 0.090f, 0.122f, 1.000f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.102f, 0.090f, 0.122f, 1.000f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.263f, 0.357f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_SliderGrab] = ImColor(111, 0, 255);
    style->Colors[ImGuiCol_SliderGrabActive] = ImColor(111, 0, 255);
    style->Colors[ImGuiCol_Button] = ImColor(0, 25, 255, 255);
    style->Colors[ImGuiCol_ButtonHovered] = ImColor(20, 20, 20);
    style->Colors[ImGuiCol_ButtonActive] = ImColor(20, 20, 20);
    style->Colors[ImGuiCol_Header] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
    style->Colors[ImGuiCol_Separator] = ImColor(105, 0, 255);
    style->WindowRounding = 50.f;
    static int tabb = 0;

    ImGui::SetNextWindowSize(ImVec2(450.000f, 550.000f), ImGuiCond_Once);

    ImGui::Begin(("Goldfish Cheat"), 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

    ImGui::BeginChild(("##backround"), ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
    {
        if (ImGui::Button("Aimbot", ImVec2(77, 20)))
        {
            tabb = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("ESP", ImVec2(77, 20)))
        {
            tabb = 2;
        }
        ImGui::SameLine();
        if (ImGui::Button("Misc", ImVec2(77, 20)))
        {
            tabb = 3;
        }
        ImGui::SameLine();
        if (ImGui::Button("Colors", ImVec2(77, 20)))
        {
            tabb = 4;
        }
        ImGui::SameLine();
        if (ImGui::Button("Settings", ImVec2(77, 20)))
        {
            tabb = 5;
        }
    }

    if (tabb == 1)
    {
        ImGui::BeginChild(("##tab1left"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("Enable Aimbot", &Settings.Aimbot.EnableAimbot);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Lock On To Enemies");
                ImGui::Checkbox2("Silent Aim", &Settings.Aimbot.SilentAim);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Teleports Bullets TO enemy In FOV");
                ImGui::Checkbox2("No Sway + Recoil", &Settings.Misc.NoSway);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("No Gun Movements");
                ImGui::Checkbox2("Rapid Fire", &Settings.Misc.RapidFire);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Shoots Guns Rapidly");
                ImGui::Checkbox2(("Aim FOV"), &Settings.Visuals.DrawAimFOV);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Aimbot FOV Circle");
                ImGui::Spacing();
                ImGui::Checkbox2("Crosshair", &Settings.Visuals.DrawCrosshair);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Enables a CrossHair");
                ImGui::Checkbox2(("Target Sleepers"), &Settings.Aimbot.TargetSleepers);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Target Sleeping Player");
                ImGui::Checkbox2(("Target Visible Only"), &Settings.Aimbot.VisibleOnly);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Visibility Check");
                ImGui::Checkbox2(("Ignore Teamates"), &Settings.Aimbot.TargetTribe);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Ignore Teamate");
                ImGui::BulletText("Aimbot Key");
                {
                    const char* Items2[] = { FKeyNames.Delete, FKeyNames.Insert, FKeyNames.LeftClick, FKeyNames.RightClick, FKeyNames.MouseButton1, FKeyNames.MouseButton2, FKeyNames.B, FKeyNames.C, FKeyNames.E, FKeyNames.F, FKeyNames.G, FKeyNames.H, FKeyNames.I, FKeyNames.J, FKeyNames.K, FKeyNames.L, FKeyNames.M, FKeyNames.N,
FKeyNames.O, FKeyNames.P, FKeyNames.Q, FKeyNames.T, FKeyNames.U, FKeyNames.V, FKeyNames.X, FKeyNames.Y, FKeyNames.Z, FKeyNames.Up, FKeyNames.Down, FKeyNames.Left, FKeyNames.Right }; // 28




                    static int Item2 = 3;
                    const char* Preview2 = Items2[Item2];

                    if (ImGui::BeginCombo("##Aim Bone Key", Preview2))
                    {
                        for (int i = 0; i < IM_ARRAYSIZE(Items2); i++)
                        {
                            const bool IsSelected = (Item2 == i);
                            if (ImGui::Selectable(Items2[i], &IsSelected)) Item2 = i;
                            if (IsSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    for (int i = 0; i < 32; i++)
                    {
                        if (Preview2 == Items2[i]) { Settings.Keybinds.AimbotKey = Items2[i]; }
                    }


                }
                ImGui::Spacing();
                ImGui::BulletText("Aimbone");
                {
                    static const char* items[] = { "Head", "Chest", "Pelvis", "Hands", "Legs", "Mix" };
                    static const char* current_item = "Head";

                    if (ImGui::BeginCombo("##combo", current_item))
                    {
                        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                        {
                            bool is_selected = (current_item == items[n]);
                            if (ImGui::Selectable(items[n], is_selected))
                                current_item = items[n];
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    if (current_item == "Head")
                        AimBone = 8;
                    if (current_item == "Chest")
                        AimBone = 4;
                    if (current_item == "Pelvis")
                        AimBone = 1;
                    if (current_item == "Hands")
                        AimBone = 38;
                    if (current_item == "Legs")
                        AimBone = 82;
                    if (current_item == "Mix")
                        AimBone = 1, 4, 8, 38, 82;
                }

            }
            ImGui::EndChild();
        }

    }
    if (tabb == 2)
    {
        ImGui::BeginChild(("##tab1left"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("Draw Player", &Settings.Visuals.DrawPlayers);
                ImGui::Checkbox2("Draw Dead Players", &Settings.Visuals.DrawDeadPlayers);
                ImGui::Checkbox2("Draw Box", &Settings.Visuals.BOX);
                ImGui::Checkbox2("Draw Player HP", &Settings.Visuals.DrawPlayerHP);
                ImGui::Checkbox2("Draw Skeleton", &Settings.Visuals.DrawPlayerBones);
                ImGui::Checkbox2("Hide Team Players", &Settings.Visuals.HideTeamPlayers);
                ImGui::Checkbox2("Draw Player Distance", &Settings.Visuals.DrawPlayerDistance);
                ImGui::Checkbox2("Draw Sleeping Players", &Settings.Visuals.DrawSleepingPlayers);
                ImGui::Checkbox2("Draw Player Name ", &Settings.Visuals.RenderPlayerName);
                ImGui::Checkbox2("Draw Head Dots", &Settings.Visuals.HeadDot);
                ImGui::Checkbox2("Draw Snap Lines", &Settings.Visuals.DrawLine);
                ImGui::Checkbox2("Draw Enemy Aim Lines", &Settings.Visuals.DrawAimingLine);
                ImGui::Checkbox2("Hide Self", &Settings.Visuals.HideSelf2);
            }
            if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("Turret ESP", &Settings.Visuals.DrawTurrets);
                ImGui::Checkbox2("Draw Turret Distance", &Settings.Visuals.DrawTurretsDistance);
                ImGui::Checkbox2("Draw Containers", &Settings.Visuals.DrawContainers);
                ImGui::Checkbox2("Draw Structure", &Settings.Visuals.DrawStructure);

            }
            ImGui::EndChild();
        }
        ImGui::SameLine();
        ImGui::BeginChild(("##tab2right"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            if (ImGui::CollapsingHeader("Dino", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("Draw Tamed Dino", &Settings.Visuals.DrawTamedCreatures);
                ImGui::Checkbox2("Tamed Dino Torp", &Settings.Visuals.TamedDinoTorp);
                ImGui::Checkbox2("Draw Dead Dino", &Settings.Visuals.DinoDead);
                ImGui::Checkbox2("Hide Team Dino", &Settings.Visuals.HideTeamDinos);
                ImGui::Checkbox2("Wild Dino Name", &Settings.Visuals.DrawWildCreatures);
                ImGui::Checkbox2("Wild HP+Torp", &Settings.Visuals.WildDinoTorp);
                ImGui::Checkbox2("Draw Dino Distance", &Settings.Visuals.DrawDinoDistance);
                ImGui::Checkbox2("Sleeping Dino ESP", &Settings.Visuals.DrawSleepingDinos);
                ImGui::Checkbox2("Hide Fish", &Settings.Visuals.HideFish);
            }
            if (ImGui::CollapsingHeader("Radar", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2(("Radar 2D"), &Settings.Visuals.Radar2D);
                if (Settings.Visuals.Radar2D)
                {
                    ImGui::Checkbox2(("Hide My Self"), &Settings.Visuals.Radar2DSelf);
                    ImGui::Checkbox2(("Enemy"), &Settings.Visuals.Radar2DEnemy);
                    ImGui::Checkbox2(("Ally"), &Settings.Visuals.Radar2DAlly);
                    ImGui::Checkbox2(("Dead"), &Settings.Visuals.Radar2DDead);
                    ImGui::Checkbox2(("Sleeper"), &Settings.Visuals.Radar2DSleeper);
                    ImGui::Checkbox2(("Ally Sleeper"), &Settings.Visuals.Radar2DASleeper);
                    ImGui::Checkbox2(("Draw Line Enemy"), &Settings.Visuals.DrawLineRadar2D);
                }
            }
            ImGui::EndChild();
        }

    }
    if (tabb == 3)
    {
        ImGui::BeginChild(("##tab1left"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("Explorer Note Hack", &Settings.Misc.explorernotehack);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Unlocks All ExplorerNote");
                ImGui::Checkbox2("Long Arm", &Settings.Misc.LongArms);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Increases Reach Distance");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Allows to Adjust FOV*** WARNING!!! HIGHER FOV ESP MOVES WITH IT");
                ImGui::Checkbox2("Infinite Orbit", &Settings.Misc.InfiniteOrbit);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Allows Infinite Orbit Radius");
                ImGui::Checkbox2("No Shake", &Settings.Misc.NoShake);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("No Shake");
                ImGui::Checkbox2("No Spread", &Settings.Misc.NoSpread);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("No Spread(ShotGuns)");
                ImGui::Checkbox2("No Sway", &Settings.Misc.NoSway);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("No Sway on Weapons");
                if (ImGui::ButtonEx("Generate GamerTag", { 200,20 }))
                {
                    int length = 8;
                    //Settings.Misc.wantedGT = (wchar_t*)L"123";
                    Settings.Misc.wantedGT = (wchar_t*)random_string(length).c_str();
                    wchar_t* gtAddy = (wchar_t*)FindDMAAddy(((uintptr_t)GetModuleHandleA("EraAdapter.dll") + 0x9f5e0), { 0x60, 0x0, 0x0, 0x28, 0x20, 0x128, 0x0 });
                    //FindDMAAddy(((uintptr_t)GetModuleHandleA("EraAdapter.dll") + 0x9f5e0), { 0x8B, 0x81, 0x68, 0x05, 0x00, 0x00 });
                    wcscpy_s(gtAddy, 16, Settings.Misc.wantedGT);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Sets To RANDOM GT");
            }
            if (ImGui::CollapsingHeader("Exploit", ImGuiTreeNodeFlags_DefaultOpen)) {
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();
        ImGui::BeginChild(("##tab2right"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            if (ImGui::CollapsingHeader("Dino", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("Instant Turn", &Settings.Misc.InstantDinoTurn);
            }
            if (ImGui::CollapsingHeader("Tek", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("Magic Punch", &Settings.Misc.TekPunch);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Allows Tek Punch Infinitly");
                if (Settings.Misc.TekPunch)
                {
                    {
                        const char* Items3[] = { FKeyNames.Delete, FKeyNames.Insert, FKeyNames.LeftClick, FKeyNames.RightClick, FKeyNames.MouseButton1, FKeyNames.MouseButton2, FKeyNames.B, FKeyNames.C, FKeyNames.E, FKeyNames.F, FKeyNames.G, FKeyNames.H, FKeyNames.I, FKeyNames.J, FKeyNames.K, FKeyNames.L, FKeyNames.M, FKeyNames.N,
    FKeyNames.O, FKeyNames.P, FKeyNames.Q, FKeyNames.T, FKeyNames.U, FKeyNames.V, FKeyNames.X, FKeyNames.Y, FKeyNames.Z, FKeyNames.Up, FKeyNames.Down, FKeyNames.Left, FKeyNames.Right }; // 28




                        static int Item3 = 10;
                        const char* Preview3 = Items3[Item3];

                        if (ImGui::BeginCombo("##TekPunch Key", Preview3))
                        {
                            for (int i = 0; i < IM_ARRAYSIZE(Items3); i++)
                            {
                                const bool IsSelected = (Item3 == i);
                                if (ImGui::Selectable(Items3[i], &IsSelected)) Item3 = i;
                                if (IsSelected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        for (int i = 0; i < 32; i++)
                        {
                            if (Preview3 == Items3[i]) { Settings.Keybinds.TekPunchKey = Items3[i]; }
                        }
                    }
                }
            }
            ImGui::EndChild();
        }
    }
    if (tabb == 4)
    {
        ImGui::BeginChild(("##tab1left"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::ColorEdit3("Player ESP Color", PlayerColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Tamed Dino ESP Color", TamedDinoColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Wild Dino ESP Color", WildDinoColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Wild Dino Filter ESP Color", FilteredDinoColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Turret ESP Color", TurretColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Container ESP Color", ContainerColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Structure ESP Color", StructureColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Crosshair FOV Color", CrosshairColor1, ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Crosshair Color", CrosshairColorFOV1, ImGuiColorEditFlags_NoInputs);
            ImGui::EndChild();
        }
    }
    if (tabb == 5)
    {
        ImGui::BeginChild(("##tab1left"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox2("HUD", &Settings.Misc.ExtraInfo);
                if (Settings.Misc.ExtraInfo)
                {
                    ImGui::Checkbox2("FPS", &Settings.Misc.ShowFPS);
                    ImGui::Checkbox2("Player Count", &Settings.Misc.ShowPlayers);
                    ImGui::Checkbox2("Dino Count", &Settings.Misc.NumTamedDinos);
                }
                ImGui::Separator();
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();
        ImGui::BeginChild(("##tab2right"), ImVec2(189, 455), false, ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::Text("CHANGELOG");
            ImGui::Separator();
            ImGui::Text("[*] ");
            ImGui::Text("[+] ");
            ImGui::Text("[+] ");
            ImGui::EndChild();
        }
    }
}
