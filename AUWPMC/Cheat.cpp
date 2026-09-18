#include <d3d11.h>
#include "imgui\imgui.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_impl_dx11.h"
#include "imgui\imgui_internal.h"
#include "Cheat.h"
#include "Logger.h"
#include "Menu.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <thread>                      
#include <fstream>                           
#include <string>                       
#include <windows.h>    
#include "byte.h"
#include <Shlobj.h>
#include <sstream>
#include <tchar.h>
#include <iomanip>
#include <tlhelp32.h>
#include <filesystem>
#include <intrin.h>
#include "xorstr.hpp"
#include <lm.h>
#include <vector>
#include "Dump.h"
#include <stdio.h>
#include <cmath>

#include "SDK\UE4\Vector2D.h"

int UWorldOnline;
int ObjOnline;
int NamesOnline;
int ProcessEventOnline;

bool NearbyNoglin = false;
std::string CompareName;

void Renderer::RemoveInput()
{
    if (D3D.WndProcOriginal)
    {
        SetWindowLongPtrA(D3D.GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(D3D.WndProcOriginal));
        D3D.WndProcOriginal = nullptr;
    }
}
void Renderer::HookInput()
{
    Renderer::RemoveInput();
    D3D.WndProcOriginal = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(D3D.GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

    //Logger::Log("[WndProcOriginal] = %p\n", D3D.WndProcOriginal);
}
bool Renderer::Remove()
{
    Renderer::RemoveInput();
    if (!RemoveHook(D3D.OriginalPresent) || !RemoveHook(D3D.SetCursorPosOriginal) || !RemoveHook(D3D.SetCursorOriginal))
    {
        return false;
    }
    if (D3D.RenderTargetView)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext();
        D3D.RenderTargetView->Release();
        D3D.RenderTargetView = nullptr;
    }
    if (D3D.Ctx)
    {
        D3D.Ctx->Release();
        D3D.Ctx = nullptr;
    }
    if (D3D.Device)
    {
        D3D.Device->Release();
        D3D.Device = nullptr;
    }
    return true;
}
BOOL WINAPI Renderer::SetCursorPosHook(int X, int Y)
{
    if (Settings.IsMenuOpen) return FALSE;
    return D3D.SetCursorPosOriginal(X, Y);
}
HCURSOR WINAPI Renderer::SetCursorHook(HCURSOR hCursor)
{
    if (Settings.IsMenuOpen) return 0;
    return D3D.SetCursorOriginal(hCursor);
}
bool Renderer::CreateView()
{
    ID3D11Texture2D* Buffer = nullptr;
    if (FAILED(D3D.SwapChain->GetBuffer(0, __uuidof(Buffer), reinterpret_cast<PVOID*>(&Buffer)))) return false;
    if (FAILED(D3D.Device->CreateRenderTargetView(Buffer, nullptr, &D3D.RenderTargetView))) return false;
    Buffer->Release();
    return true;
}
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI Renderer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }
    switch (msg)
    {
    case WM_SIZE:
        if (D3D.Device != nullptr && wParam != SIZE_MINIMIZED)
        {
            if (D3D.RenderTargetView)
            {
                D3D.RenderTargetView->Release();
                D3D.RenderTargetView = nullptr;
            }
            D3D.SwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
            CreateView();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
void Utils::PE_HOOK(void* obj, UFunction* fn, void* params)
{
    if (GetAsyncKeyState(VK_DELETE) & 0x0001) Settings.IsMenuOpen = !Settings.IsMenuOpen, Renderer::RemoveInput();
    return Settings.OriginalPE(obj, fn, params);
}
FVector* Utils::GetAdjustedAim(AShooterWeapon* Weapon, FVector* Result)   // AShooterWeapon
{
    if (Cache.AimbotTarget && Settings.Aimbot.SilentAim)
    {
        FVector BoneLocation;
        Settings.GetBoneLocation(Cache.AimbotTarget->MeshComponent, &BoneLocation, Cache.AimbotTarget->MeshComponent->GetBoneName(AimBone), 0);
        FVector AimDirection = Cache.LocalPlayer->GetDirectionVector(Cache.LPC->PlayerCameraManager->GetCameraLocation(), BoneLocation);
        *Result = AimDirection;
        //if (Result->X == 0 || Result->Y == 0 || Result->Z == 0) return Cache.OriginalGetAdjustedAim(Weapon, Result);
        return Result;
    }
    return Cache.OriginalGetAdjustedAim(Weapon, Result);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
ULocalPlayer* GetLocalPlayer()
{
    TArray<ULocalPlayer*>& lPlayers = (UWorld::GWorld)->OwningGameInstance->LocalPlayers;
    return lPlayers[0];
}
void SetPlayerPosition(float x, float y, float z)
{
    ULocalPlayer* localPlayer = GetLocalPlayer();
    if (!localPlayer)
    {
        Logger::Log("localPlayer is nullptr");
        return;
    }
    Logger::Log("LocalPlayer: %p", localPlayer);

    //UObject* local;
}//-----------------------------------------------------------------------------------------------------------------------------------------
bool Once = false;
//----------------------------------------- ProcessEvent ----------------------------------------------------------------
typedef void(__thiscall* ProcessEventType)(UObject*, UFunction*, void*);
ProcessEventType OProcessEvent = nullptr;
inline bool Renderer::Init()
{
    D3D.PresentFunc = GetD3D11PresentFunction();
    if (!SetHook(D3D.PresentFunc, D3D_HOOK, reinterpret_cast<void**>(&D3D.OriginalPresent)))
    {
        Logger::Log("[HOOK]: Failed Present Function Hook!\n");
        return false;
    }
    if (!SetHook(SetCursorPos, SetCursorPosHook, reinterpret_cast<void**>(&D3D.SetCursorPosOriginal)))
    {
        Logger::Log("[HOOK}: Failed SetCursorPos Hook!\n");
        return false;
    };
    if (!SetHook(SetCursor, SetCursorHook, reinterpret_cast<void**>(&D3D.SetCursorOriginal)))
    {
        Logger::Log("[HOOK]: Failed SetCursor Hook!\n");
        return false;
    }; 
    ULocalPlayer* localPlayer = GetLocalPlayer();
    void** localPlayerVmt = *reinterpret_cast<void***>(localPlayer);
    OProcessEvent = reinterpret_cast<ProcessEventType>(localPlayerVmt[PROCESS_EVENT_INDEX]);
    uintptr_t hookAddress = reinterpret_cast<uintptr_t>(OProcessEvent);
    if (!SetHook(reinterpret_cast<void*>(hookAddress), Utils::PE_HOOK, reinterpret_cast<PVOID*>(&Settings.OriginalPE)))
    {
        Logger::Log("[HOOK]: Failed ProcessEvent Hook!\n");
    }
    return true;
}
inline void Renderer::Drawing::RenderText(ImVec2 ScreenPosition, ImColor Color, const char* Text, int WidthText)
{
    bool Center = true;
    std::stringstream Stream(Text);
    std::string Line;
    float Y = 0.0f;
    int Index = 0;
    auto FontSize = ImGui::GetFontSize();
    auto Font = ImGui::GetFont();
    while (std::getline(Stream, Line))
    {
        ImVec2 TextSize = ImVec2(0, 0);
        TextSize = ImGui::GetFont()->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Line.c_str());
        if (Font) TextSize = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Line.c_str());
        if (Center)
        {
            ImGui::GetBackgroundDrawList()->AddText(Font, FontSize, ImVec2((ScreenPosition.x - TextSize.x / 2.0f) + 1, (ScreenPosition.y + TextSize.y * Index) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), Line.c_str());
            ImGui::GetBackgroundDrawList()->AddText(Font, FontSize, ImVec2((ScreenPosition.x - TextSize.x / 2.0f) - 1, (ScreenPosition.y + TextSize.y * Index) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), Line.c_str());
            ImGui::GetBackgroundDrawList()->AddText(Font, FontSize, ImVec2(ScreenPosition.x - TextSize.x / 2.0f, ScreenPosition.y + TextSize.y * Index), Color, Line.c_str());
        }
        else
        {
            ImGui::GetBackgroundDrawList()->AddText(Font, FontSize, ImVec2((ScreenPosition.x) + 1, (ScreenPosition.y + TextSize.y * Index) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), Line.c_str());
            ImGui::GetBackgroundDrawList()->AddText(Font, FontSize, ImVec2((ScreenPosition.x) - 1, (ScreenPosition.y + TextSize.y * Index) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), Line.c_str());
            ImGui::GetBackgroundDrawList()->AddText(Font, FontSize, ImVec2(ScreenPosition.x, ScreenPosition.y + TextSize.y * Index), Color, Line.c_str());
        }
        Y = ScreenPosition.y + TextSize.y * (Index + 1);
        Index++;
    }
}
inline void Renderer::Drawing::RenderText2(ImVec2 ScreenPosition, ImColor Color, const char* Text, int WidthText)
{
    if (!Text) return;
    auto ImScreen = *reinterpret_cast<const ImVec2*>(&ScreenPosition);
    if (ScreenPosition.x > 0 && ScreenPosition.y > 0 && Cache.WindowSizeX > ScreenPosition.x && Cache.WindowSizeY > ScreenPosition.y)
    {
        auto Size = ImGui::CalcTextSize(Text);
        ImScreen.x -= Size.x * 0.5f;
        ImScreen.y -= Size.y;
        ImGui::GetBackgroundDrawList()->AddText(ImGui::GetFont(), 20, ImScreen, Color, Text, Text + strlen(Text), WidthText);
    }
}
inline void Renderer::Drawing::RenderCollapseFriendlyDisplayList(ImVec2 ScreenStartPosition, ImColor Color, std::vector<std::string> DisplayArray, float HeightFactor)
{
    auto StartPosition = *reinterpret_cast<const ImVec2*>(&ScreenStartPosition);
    for (size_t s = 0; s < DisplayArray.size(); s++)
    {
        auto& DisplayString = DisplayArray[s];
        if (DisplayString.length() < 1) continue;
        auto CurrentPosition = ImVec2(StartPosition.x, StartPosition.y);
        auto StringSize = ImGui::CalcTextSize(DisplayString.c_str());
        CurrentPosition.y += StringSize.y;
        ImGui::GetBackgroundDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(CurrentPosition.x, StartPosition.y), Color, DisplayString.c_str(), DisplayString.c_str() + strlen(DisplayString.c_str()), 500);
        StartPosition.y += HeightFactor;
    }
}
inline bool Renderer::Drawing::HeadDot(USkeletalMeshComponent* Mesh, int Gender, ImColor Color)
{
    FVector MyHeadWorldLocation;
    FVector BoneWorldLocation;
    FVector2D MyHead;
    FVector2D EnemyHead;
    ImColor RedColor(255, 0, 0, 255);

    Settings.GetBoneLocation(Mesh, &MyHeadWorldLocation, Mesh->GetBoneName(8), 0);
    if (!W2S(MyHeadWorldLocation, MyHead)) return false;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(8), 0);
    if (!W2S(BoneWorldLocation, EnemyHead)) return false;


    auto DrawList = ImGui::GetBackgroundDrawList();

    //DrawList->AddLine(ImVec2(MyHead.X, MyHead.Y), ImVec2(EnemyHead.X, EnemyHead.Y), RedColor, 2.0f);

    DrawList->AddCircleFilled(ImVec2(MyHead.X, MyHead.Y), 3.0f, Color);
    DrawList->AddCircleFilled(ImVec2(EnemyHead.X, EnemyHead.Y), 3.0f, Color);

    return true;
}
inline bool Renderer::Drawing::BOX(USkeletalMeshComponent* Mesh, int Gender, ImColor Color)
{
    
    FVector BoneWorldLocation;
    FVector2D Head;
    FVector2D Neck;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(8), 0);
    if (!W2S(BoneWorldLocation, Head)) return false;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(6), 0);
    if (!W2S(BoneWorldLocation, Neck)) return false;
    auto DrawList = ImGui::GetBackgroundDrawList();
    DrawList->AddRect(ImVec2(Neck.X + 10, Neck.Y - 20), ImVec2(Head.X - 10, Head.Y + 15), Color);
    //                               Droit         Down               Gauche          Hauteur
    return true;
}
inline bool Renderer::Drawing::HP(USkeletalMeshComponent* Mesh, int Gender, ImColor Color)
{

}
inline void Renderer::Drawing::RenderCrosshair(ImColor Color, int Thickness)
{
    auto DrawList = ImGui::GetBackgroundDrawList();
    auto WinSizeX = ImGui::GetWindowSize().x;
    auto WinSizeY = ImGui::GetWindowSize().y;
    DrawList->AddLine(ImVec2(WinSizeX / 2, WinSizeY / 2), ImVec2(WinSizeX / 2, WinSizeY / 2 - Settings.Visuals.CrosshairSize), Color, Thickness);
    DrawList->AddLine(ImVec2(WinSizeX / 2, WinSizeY / 2), ImVec2(WinSizeX / 2, WinSizeY / 2 + Settings.Visuals.CrosshairSize), Color, Thickness);
    DrawList->AddLine(ImVec2(WinSizeX / 2, WinSizeY / 2), ImVec2(WinSizeX / 2 - Settings.Visuals.CrosshairSize, WinSizeY / 2), Color, Thickness);
    DrawList->AddLine(ImVec2(WinSizeX / 2, WinSizeY / 2), ImVec2(WinSizeX / 2 + Settings.Visuals.CrosshairSize, WinSizeY / 2), Color, Thickness);
}
inline void Renderer::Drawing::RenderAimFOV(ImColor Color)
{
    ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(Cache.WindowSizeX / 2, Cache.WindowSizeY / 2), Settings.Visuals.FOVSize, Color, 100, 1.0f);
}
inline int Renderer::Drawing::ReturnDistance(int X1, int Y1, int X2, int Y2) 
{ 
    return sqrt(pow(X2 - X1, 2) + pow(Y2 - Y1, 2) * 1); 
}
inline bool Renderer::Drawing::WithinAimFOV(int CircleX, int CircleY, int R, int X, int Y)
{
    int Dist = (X - CircleX) * (X - CircleX) + (Y - CircleY) * (Y - CircleY);
    if (Dist <= R * R) return true;
    else return false;
}
inline bool Renderer::Drawing::RenderPlayerSkeleton(USkeletalMeshComponent* Mesh, int Gender, ImColor Color)
{
    FVector BoneWorldLocation;
    // Spine
    FVector2D Head;
    FVector2D Neck;
    FVector2D Spine;
    FVector2D Pelvis;
    // Left Arm
    FVector2D LeftShoulder;
    FVector2D LeftElbow;
    FVector2D LeftWrist;
    // Right Arm
    FVector2D RightShoulder;
    FVector2D RightElbow;
    FVector2D RightWrist;
    // Left Leg
    FVector2D LeftKnee;
    FVector2D LeftAnkle;
    // Right Leg
    FVector2D RightKnee;
    FVector2D RightAnkle;

    // Spine
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(8), 0);
    if (!W2S(BoneWorldLocation, Head)) return false;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(6), 0);
    if (!W2S(BoneWorldLocation, Neck)) return false;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(4), 0);
    if (!W2S(BoneWorldLocation, Spine)) return false;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(1), 0);
    if (!W2S(BoneWorldLocation, Pelvis)) return false;
    // Left Arm
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(33), 0);
    if (!W2S(BoneWorldLocation, LeftShoulder)) return false;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(36), 0);
    if (!W2S(BoneWorldLocation, LeftElbow)) return false;
    Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(38), 0);
    if (!W2S(BoneWorldLocation, LeftWrist)) return false;
    // Right Arm
    if (Gender == 1) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(57), 0); } // Male
    else if (Gender == 2) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(59), 0); } // Female
    if (!W2S(BoneWorldLocation, RightShoulder)) return false;
    if (Gender == 1) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(60), 0); }
    else if (Gender == 2) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(62), 0); }
    if (!W2S(BoneWorldLocation, RightElbow)) return false;
    if (Gender == 1) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(62), 0); }
    else if (Gender == 2) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(64), 0); }
    if (!W2S(BoneWorldLocation, RightWrist)) return false;
    // Left Leg
    if (Gender == 1) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(82), 0); } // Male
    else if (Gender == 2) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(84), 0); } // Female
    if (!W2S(BoneWorldLocation, LeftKnee)) return false;
    if (Gender == 1) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(84), 0); } // Male
    else if (Gender == 2) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(86), 0); } // Female
    if (!W2S(BoneWorldLocation, LeftAnkle)) return false;
    // Right Leg
    if (Gender == 1) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(88), 0); } // Male
    else if (Gender == 2) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(90), 0); } // Female
    if (!W2S(BoneWorldLocation, RightKnee)) return false;
    if (Gender == 1) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(90), 0); } // Male
    else if (Gender == 2) { Settings.GetBoneLocation(Mesh, &BoneWorldLocation, Mesh->GetBoneName(92), 0); } // Female
    if (!W2S(BoneWorldLocation, RightAnkle)) return false;

    auto DrawList = ImGui::GetBackgroundDrawList();
    DrawList->AddLine(ImVec2(LeftAnkle.X, LeftAnkle.Y), ImVec2(LeftKnee.X, LeftKnee.Y), Color);
    DrawList->AddLine(ImVec2(RightAnkle.X, RightAnkle.Y), ImVec2(RightKnee.X, RightKnee.Y), Color);
    DrawList->AddLine(ImVec2(LeftKnee.X, LeftKnee.Y), ImVec2(Pelvis.X, Pelvis.Y), Color);
    DrawList->AddLine(ImVec2(RightKnee.X, RightKnee.Y), ImVec2(Pelvis.X, Pelvis.Y), Color);
    DrawList->AddLine(ImVec2(Pelvis.X, Pelvis.Y), ImVec2(Spine.X, Spine.Y), Color);
    DrawList->AddLine(ImVec2(Spine.X, Spine.Y), ImVec2(Neck.X, Neck.Y), Color);
    DrawList->AddLine(ImVec2(Neck.X, Neck.Y), ImVec2(Head.X, Head.Y), Color);
    DrawList->AddLine(ImVec2(LeftWrist.X, LeftWrist.Y), ImVec2(LeftElbow.X, LeftElbow.Y), Color);
    DrawList->AddLine(ImVec2(LeftElbow.X, LeftElbow.Y), ImVec2(LeftShoulder.X, LeftShoulder.Y), Color);
    DrawList->AddLine(ImVec2(LeftShoulder.X, LeftShoulder.Y), ImVec2(Neck.X, Neck.Y), Color);
    DrawList->AddLine(ImVec2(RightWrist.X, RightWrist.Y), ImVec2(RightElbow.X, RightElbow.Y), Color);
    DrawList->AddLine(ImVec2(RightElbow.X, RightElbow.Y), ImVec2(RightShoulder.X, RightShoulder.Y), Color);
    DrawList->AddLine(ImVec2(RightShoulder.X, RightShoulder.Y), ImVec2(Neck.X, Neck.Y), Color);
    return true;
}
void RenderTextWithBackground(ImDrawList* drawList, const ImVec2& pos, const ImColor& color, const char* text, const ImColor& backgroundColor, const ImColor& textColor, int wrap_width = -1)
{
    // Calculer la taille du texte
    ImVec2 textSize = ImGui::CalcTextSize(text);

    // Dessiner un rectangle derrière le texte
   // ImVec2 rectMin(pos.x, pos.y);
    //ImVec2 rectMax(pos.x + textSize.x, pos.y + textSize.y);
   // drawList->AddRectFilled(rectMin, rectMax, backgroundColor);

    // Dessiner le texte
    drawList->AddText(pos, textColor, text);
}
HRESULT Renderer::D3D_HOOK(IDXGISwapChain* SwapChain, UINT SyncInterval, UINT Flags)
{
    if (!D3D.Device)
    {
        ID3D11Texture2D * Surface = nullptr;
        goto Init;
    Cleanup:
        Logger::Log("[IMGUI]: Initializing ImGui Cleanup!\n");
        if (Surface) Surface->Release();
        return D3D.PresentFunc(SwapChain, SyncInterval, Flags);
    Init:
        if (FAILED(SwapChain->GetBuffer(0, _uuidof(Surface), reinterpret_cast<PVOID*>(&Surface)))) { goto Cleanup; }
        Logger::Log("[ID3D11Texture2D]: 0x%llX\n", Surface);
        if (FAILED(SwapChain->GetDevice(__uuidof(D3D.Device), reinterpret_cast<PVOID*>(&D3D.Device)))) goto Cleanup;
        Logger::Log("[ID3D11Device]: 0x%llX\n", D3D.Device);
        if (FAILED(D3D.Device->CreateRenderTargetView(Surface, nullptr, &D3D.RenderTargetView))) goto Cleanup;
        Logger::Log("[ID3D11RenderTargetView]: 0x%llX\n", D3D.RenderTargetView);
        Surface->Release();
        Surface = nullptr;
        D3D.Device->GetImmediateContext(&D3D.Ctx);
        Logger::Log("[ID3D11DeviceContext]: 0x%llX\n", D3D.Ctx);
        IMGUI_CHECKVERSION();
        auto Window = FindWindowA("Windows.UI.Core.CoreWindow", "ARK: Survival Evolved");
        if (!Window)
        {
            HWND ParentWindow = FindWindowA("ApplicationFrameWindow", "ARK: Survival Evolved");
            HWND ChildWindow = FindWindowExA(ParentWindow, NULL, "Windows.UI.Core.CoreWindow", "ARK: Survival Evolved");
            D3D.GameWindow = ChildWindow;
        }
        else { D3D.GameWindow = Window; }
        ImGui::CreateContext();
        {
            ImGuiIO& IO = ImGui::GetIO();
            ImFontConfig Config;
            ImGuiStyle* Style = &ImGui::GetStyle();
            bool show_demo_window = true, loader_window = false;
            bool show_another_window = false;
            ImGui::StyleColorsDark();
            Style->Alpha = 1.f;
            Style->WindowRounding = 12.f;
            Style->FramePadding = ImVec2(4, 3);
            Style->WindowPadding = ImVec2(8, 8);
            Style->ItemInnerSpacing = ImVec2(4, 4);
            Style->ItemSpacing = ImVec2(8, 5);
            Style->FrameRounding = 4.f;
            Style->ScrollbarSize = 2.f;
            Style->ScrollbarRounding = 12.f;
            Style->PopupRounding = 5.f;
            ImVec4* colors = ImGui::GetStyle().Colors;



            colors[ImGuiCol_ChildBg] = ImColor(24, 29, 59, 0);
            colors[ImGuiCol_Border] = ImVec4(255, 255, 255, 0);
            colors[ImGuiCol_FrameBg] = ImColor(25, 25, 33, 255);  // backgrond de la box
            colors[ImGuiCol_FrameBgActive] = ImColor(25, 25, 33, 255);
            colors[ImGuiCol_FrameBgHovered] = ImColor(25, 25, 33, 255);
            colors[ImGuiCol_Header] = ImColor(25, 25, 33, 255);
            colors[ImGuiCol_HeaderActive] = ImColor(25, 25, 33, 255);
            colors[ImGuiCol_HeaderHovered] = ImColor(255, 69, 0, 255);
            colors[ImGuiCol_PopupBg] = ImColor(24, 29, 59, 255);
            colors[ImGuiCol_Button] = ImColor(255, 69, 0, 255);
            colors[ImGuiCol_ButtonHovered] = ImColor(200, 69, 0, 255);
            colors[ImGuiCol_ButtonActive] = ImColor(152, 75, 191, 255);
            colors[ImGuiCol_TitleBgActive] = ImColor(24, 29, 59, 0);
            colors[ImGuiCol_WindowBg] = ImColor(24, 29, 59, 0);
            colors[ImGuiCol_Border] = ImColor(24, 29, 59, 0);

            Config.OversampleH = 1; //or 2 is the same
            Config.OversampleV = 1;
            Config.PixelSnapH = 1;


            static const ImWchar ranges[] =
            {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0x0400, 0x044F, // Cyrillic
                0,
            };


            zzzz = IO.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\ARLRDBD.ttf", 13.0f, &Config);
            //zzzz = IO.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arialbd.ttf", 13.0f, &Config);
            icons = IO.Fonts->AddFontFromMemoryTTF((void*)iconfont, sizeof(iconfont), 50.f, &Config);
            Config.GlyphRanges = IO.Fonts->GetGlyphRangesCyrillic();
            Config.RasterizerMultiply = 1.125f;
            IO.IniFilename = nullptr;     

            TCHAR name[UNLEN + 1];
            DWORD size = UNLEN + 1;
            GetUserName(name, &size);
            char usernameChar[500];
            size_t nNumCharConverted;
            #ifdef UNICODE
            wcstombs_s(&nNumCharConverted, usernameChar, 500, name, _TRUNCATE);
            #else
            strcpy_s(usernameChar, 500, name);
            #endif
            std::string usernameString = "C:\\Users\\" + std::string(usernameChar) + "\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\config1";
            Settings.dllPath = usernameString;
            std::string usernameString2 = "C:\\Users\\" + std::string(usernameChar) + "\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\config2";
            Settings.dllPath2 = usernameString2;
            std::string usernameString3 = "C:\\Users\\" + std::string(usernameChar) + "\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\config3";
            Settings.dllPath3 = usernameString3;
            std::string usernameString4 = "C:\\Users\\" + std::string(usernameChar) + "\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\config4";
            Settings.dllPath4 = usernameString4;
            std::string usernameString5 = "C:\\Users\\" + std::string(usernameChar) + "\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\config5";
            Settings.dllPath5 = usernameString5;
        }
        
        Logger::Log("[GameWindow]: 0x%llX\n", D3D.GameWindow);
        ImGui_ImplWin32_Init(D3D.GameWindow);
        if (!ImGui_ImplDX11_Init(D3D.Device, D3D.Ctx)) goto Cleanup;
        if (!ImGui_ImplDX11_CreateDeviceObjects()) goto Cleanup;
        Logger::Log("[IMGUI]: Initialized Successfully!\n");
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("##ESP", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    auto& io = ImGui::GetIO();
    Cache.WindowSizeX = io.DisplaySize.x;
    Cache.WindowSizeY = io.DisplaySize.y;
    ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
    auto DrawList = ImGui::GetCurrentWindow()->DrawList;                // TEST RADAR


    std::array<std::string, (152)> RealDinoNames = { "Achatina", "Allo", "Ammonite", "Angler", "Anky", "Ant", "Archa", "Argent", "Arthro", "Astrodelphis", "Baryonyx", "Basilisk", "Basilosaurus", "Bat", "Beaver", "Beetle", "Bigfoot", "Bloodstalker", "BrainSlug", "Bronto", "Carbonemys", "Camelsaurus", "Carno", "CaveCrab", "CaveWolf", "Chalico", "Cnidaria", "Compy", "Daeodon", "Deathworm", "Default", "Deinonychus", "DesertTitan", "Dilo", "Dimetro", "Dimorph", "Diplo", "Diplocaulus", "Direbear", "Direwolf", "Dodo", "Doed", "Dolphin", "Dragon", "Dragonfly", "Dunkle", "Eel", "Elite Carno", "Elite Raptor", "Elite Rex", "Elite Mega", "Enforcer", "Equus", "Euryp", "Flock", "ForestTitan", "Gacha", "Galli", "GasBags", "GiantTurtle", "Gigant", "Gremlin", "Griffin", "Hesperornis", "Hyaenodon", "IceTitan", "Ichthyornis", "Iguanodon", "Jerboa", "Jugbug", "Kairu", "Kangaroo", "Kaprosuchus", "Kentro", "Lamprey", "Lantern Bird", "Lantern Goat", "Lantern Lizard", "LavaLizard", "Leech", "Leedsichthys", "Lionfish Lion", "Liopleurodon", "Lystro", "Maewing", "Mammoth", "Managarmr", "Manta", "Mantis", "Megalania", "Megalosaurus", "Megapithecus", "Megatherium", "Microraptor", "MoleRat", "Monkey", "Mosasaur", "Mega", "Moschops", "Moth", "Otter", "Ovi", "Owl", "Pachy", "Para", "Paracer", "Pegomastax", "Pela", "Phiomia", "Phoenix", "Piranha", "Plesiosaur", "Ptera", "Pug", "Purlovia", "Quetz", "Raptor", "Rex", "Rhino", "RockDrake", "RockElemental", "Sabertooth", "Sarco", "Scorpion", "Scout", "Sheep", "Space Whale", "Spider", "SpineyLizard", "Spino", "Stag", "Stego", "Summoner", "Tapejara", "TekStrider", "TekWyvern", "TerrorBird", "Therizinosaurus", "Thylacoleo", "Titan", "Titanboa", "Toad", "Trike", "Troodon", "Tropeognathus", "Turtle", "Tusoteuthis", "Velonasaur", "Vulture", "Wyvern", "Xenomorph", "Yutyrannus"};
    auto DebugString = "Default";
    try
    {
        UWorld::GWorld = *reinterpret_cast<decltype(UWorld::GWorld)*>(Cache.GameBase + uWorld_Offset);
        do
        {
            
            UWorld* GWorld = UWorld::GWorld;
            if (!GWorld) break;
            void* GameState = GWorld->GameState;
            if (!GameState) break;          
            UPlayer* LocalPlayer = GWorld->OwningGameInstance->LocalPlayers[0];
            if (!LocalPlayer) break;
            Cache.LocalPlayer = GWorld->OwningGameInstance->LocalPlayers[0];
            Cache.LPC = Cache.LocalPlayer->PlayerController;
            Cache.LPFOV = Cache.LPC->PlayerCameraManager->DefaultFOV;
            Cache.LocalLocation = Cache.LPC->PlayerCameraManager->GetCameraLocation();
            Cache.NearbyEnemies = 0;

            auto Actors = UWorld::GWorld->PersistentLevel->Actors;          
            for (int i = 0; i < Actors.Count; i++)
            {
                auto Actor = Actors[i];
                if (Actor)
                {
                    bool isDead = false;
                    bool isSleeping = false;
                    bool isTribeDinoOrPlayer = false;
                    auto TargetableActor = (APrimalTargetableActor*)Actor;
                    //==============================================================  Player ESP   =================================================================
                    if (Actor->IsPlayer() && Settings.Visuals.DrawPlayers)
                    {              
                        std::string GamerTag;
                        std::string PlayerDisplayString;
                        FVector2D ActorScreenLocation;
                        FVector2D ActorScreenLocation2 = FVector2D(0, 0);
                        float TorpOffset = 13.f;
                        float DistanceOffset = -13.f;

                        ImColor PlayerColorHP;
                        int NewR1 = (int)PlayerColor1[0] * 0;
                        int NewG1 = (int)PlayerColor1[1] * 0;
                        int NewB1 = (int)PlayerColor1[2] * 0;
                        PlayerColorHP = ImColor(NewR1, NewG1, NewB1, 255);

                        ImColor PlayerColor;
                        int NewR = (int)PlayerColor1[0] * 255;
                        int NewG = (int)PlayerColor1[1] * 255;
                        int NewB = (int)PlayerColor1[2] * 255;
                        PlayerColor = ImColor(NewR, NewG, NewB, 255);
                        
                        float HealthPercent = Actor->ReplicatedCurrentHealth / Actor->ReplicatedMaxHealth;
                        std::string PlayerHP = std::string("" + std::to_string((int)Actor->ReplicatedCurrentHealth) + "   /   " + std::to_string((int)Actor->ReplicatedMaxHealth) + " HP");
                        std::string PlayerHPBar = std::string("" + std::to_string((int)Actor->ReplicatedCurrentHealth));
                        std::string PlayerTorpBar = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor));
                        std::string PlayerTorp = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor) + "   /   " + std::to_string((int)Actor->ReplicatedMaxTorpor) + " Torp");
                        std::string HealhtBar = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor) + "   /   " + std::to_string((int)Actor->ReplicatedMaxTorpor) + " Torp");

                        if (Settings.Visuals.HideSelf2)
                        {
                            if (Actor == Cache.LocalActor && Settings.Visuals.HideSelf) continue;
                        }

                        if (Settings.Visuals.DrawPlayerDistance && Settings.Visuals.PlayerTorp)
                        {
                            TorpOffset = TorpOffset * 2;
                        }
                        else if (Settings.Visuals.DrawSleepingPlayers && Settings.Visuals.DrawPlayerHP)
                        {
                            DistanceOffset = DistanceOffset * 2;
                        }
                        else if (Settings.Visuals.DrawPlayerHP && Settings.Visuals.DrawPlayerDistance)
                        {
                            DistanceOffset = DistanceOffset * 2; // HP normal
                        }

                        if (Actor->IsLocalPlayer())
                        {
                            Cache.LocalActor = Actor;
                        }

                        if (Actor->PlatformProfileName.Data)
                        {
                            GamerTag = Actor->PlatformProfileName.ToString();
                        }

                        if (Actor->IsDead())
                        {
                            isDead = true;
                            if (Settings.Visuals.DrawDeadPlayers)
                            {
                                PlayerColor = ImColor(255, 153, 8, 255);
                            }
                        }
                        if (!Actor->IsConscious() && !isDead)
                        {
                            isSleeping = true;
                            if (Settings.Visuals.DrawSleepingPlayers)
                            {
                                PlayerColor = ImColor(255, 255, 255, 255);
                            }
                        }
                        if (Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                        {
                            isTribeDinoOrPlayer = true;
                            if (!isDead && !isSleeping)
                            {
                                PlayerColor = ImColor(60, 255, 0, 255);
                            }
                        }
                        if (isTribeDinoOrPlayer && isSleeping && !isDead)
                        {
                            PlayerColor = ImColor(154, 161, 119, 255);
                        }
                        if (Actor->PlayerName.Data && Actor->MyCharacterStatusComponent)
                        {
                            PlayerDisplayString = "" + GamerTag + "     " + Actor->PlayerName.ToString() + "    lvl  " + std::to_string(Actor->MyCharacterStatusComponent->BaseCharacterLevel + Actor->MyCharacterStatusComponent->ExtraCharacterLevel) + "";
                        }

                        if (Actor->IsPlayer() && Actor->IsConscious() && !Actor->IsDead() && Actor != Cache.LocalActor)
                        {
                            if (!isTribeDinoOrPlayer)
                            {
                                std::string NewNumbPlayers;
                                Cache.NearbyEnemies += 1;
                                NewNumbPlayers = Cache.NearbyEnemies;
                            }
                        }

                        if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                        {
                            if (!Settings.Visuals.DrawDeadPlayers && isDead) continue;
                            if (!Settings.Visuals.DrawSleepingPlayers && isSleeping) continue;
                            if (Settings.Visuals.HideTeamPlayers && isTribeDinoOrPlayer) continue;
                            //if (Settings.Visuals.DrawPlayerDistance && isTribeDinoOrPlayer) continue;

                            if (Settings.Visuals.DrawDeadPlayers && isDead)
                            {
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 90), PlayerColor, "Dead", 500);
                            }
                            if (Settings.Visuals.DrawSleepingPlayers && isSleeping)
                            {
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 90), PlayerColor, "Sleeping", 500);
                            }
                            if (Settings.Visuals.DrawPlayerDistance)// || Settings.Visuals.ShowWeapons)
                            {

                                int Distance = Cache.LocalActor->RootComponent->GetWorldLocation().DistTo(Actor->RootComponent->GetWorldLocation()) * 0.01f;
                                std::string DistanceString = "" + std::to_string(Distance) + " M";
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 75), PlayerColor, DistanceString.c_str(), 500);


                                /*  // GetNameByIDFast
                                std::string WeaponInfo = std::to_string((int)Actor->CurrentWeapon);    // SHOW ID WEAPON ???
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 90), PlayerColor, WeaponInfo.c_str(), 500);
                                */

                            }

                            //    !isDead && isSleeping  =  Only Sleeping  //   !isDead && !isSleeping  = Only Alive

                            //========================================================     PLAYER TORPEUR     ==========================================================================

                            if (!isDead && !isSleeping && Settings.Visuals.PlayerTorp)
                            {
                                // Dessinez la barre de santé en Gris
                                ImColor grayColor(128, 128, 128, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X + 30, ActorScreenLocation.Y + 50 }, grayColor, 10.f);

                                //ligne Mauve
                                float HealthPercent = Actor->ReplicatedCurrentTorpor / Actor->ReplicatedMaxTorpor;

                                ImColor mauveColor(128, 0, 128, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X - 30 + (HealthPercent * 50), ActorScreenLocation.Y + 50 }, mauveColor, 10.f);

                                // Calculer la position du texte HP pour centrer le texte sur la barre de santé
                                float HPTextX = ActorScreenLocation.X - 10;
                                float HPTextY = ActorScreenLocation.Y + 45;

                                // Afficher le texte HP avec un fond noir pour le rendre plus visible
                                ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), PlayerColorHP, PlayerTorpBar.c_str(), bgColor, textColor, 500);

                               // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 35), PlayerColor, PlayerTorp.c_str(), 500);  // TEXTE NO BAR
                            }
                            if (!isDead && isSleeping && Settings.Visuals.PlayerTorp2)
                            {

                                // Dessinez la barre de santé en Gris
                                ImColor grayColor(128, 128, 128, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X + 30, ActorScreenLocation.Y + 50 }, grayColor, 10.f);

                                //ligne Mauve
                                float HealthPercent = Actor->ReplicatedCurrentTorpor / Actor->ReplicatedMaxTorpor;

                                ImColor mauveColor(128, 0, 128, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X - 30 + (HealthPercent * 50), ActorScreenLocation.Y + 50 }, mauveColor, 10.f);

                                // Calculer la position du texte HP pour centrer le texte sur la barre de santé
                                float HPTextX = ActorScreenLocation.X - 10;
                                float HPTextY = ActorScreenLocation.Y + 45;

                                // Afficher le texte HP avec un fond noir pour le rendre plus visible
                                ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), PlayerColorHP, PlayerTorpBar.c_str(), bgColor, textColor, 500);




                                //Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 35), PlayerColor, PlayerTorp.c_str(), 500);  // TEXTE NO BAR
                            }
                            /*
                            if (isDead && !isSleeping && Settings.Visuals.PlayerTorp2)
                            {
                                // Dessinez la barre de santé en Gris
                                ImColor grayColor(128, 128, 128, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X + 30, ActorScreenLocation.Y + 50 }, grayColor, 10.f);

                                //ligne Mauve
                                float HealthPercent = Actor->ReplicatedCurrentTorpor / Actor->ReplicatedMaxTorpor;

                                ImColor mauveColor(128, 0, 128, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X - 30 + (HealthPercent * 50), ActorScreenLocation.Y + 50 }, mauveColor, 10.f);

                                // Calculer la position du texte HP pour centrer le texte sur la barre de santé
                                float HPTextX = ActorScreenLocation.X - 10;
                                float HPTextY = ActorScreenLocation.Y + 45;

                                // Afficher le texte HP avec un fond noir pour le rendre plus visible
                                ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), PlayerColorHP, PlayerTorpBar.c_str(), bgColor, textColor, 500);



                               // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 35), PlayerColor, PlayerTorp.c_str(), 500);  // TEXTE NO BAR
                            }
                            */
                            //========================================================     PLAYER TORPEUR     ==========================================================================
                            if (Settings.Visuals.RenderPlayerName)
                            {
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 60), PlayerColor, PlayerDisplayString.c_str(), 500); // PLAYER NAME OFFSET
                            }
                            if (Settings.Visuals.DrawPlayerBones)
                            {
                                Renderer::Drawing::RenderPlayerSkeleton(Actor->MeshComponent, Actor->RetrievePlayerGender(Actor->Name.GetName()), PlayerColor);
                            }
                            if (Settings.Visuals.BOX)
                            {
                                //Renderer::Drawing::BOX(Actor->MeshComponent, Actor->RetrievePlayerGender(Actor->Name.GetName()), PlayerColor);
                                Renderer::Drawing::BOX(Actor->MeshComponent, Actor->RetrievePlayerGender(Actor->Name.GetName()), PlayerColor);
                            }
                            if (Settings.Visuals.HeadDot)
                            {
                                Renderer::Drawing::HeadDot(Actor->MeshComponent, Actor->RetrievePlayerGender(Actor->Name.GetName()), PlayerColor);
                            }

                            if (isDead) continue;
                            if (!Settings.Visuals.DrawPlayerHP) continue;

                            //========================================================     PLAYER HP BAR    ==========================================================================
                            // Dessinez la barre de santé en rouge
                            ImColor redColor(255, 0, 0, 255);
                            DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X + 20, ActorScreenLocation.Y + 40 }, redColor, 10.f);

                            //ligne vert
                            float HealthPercent = Actor->ReplicatedCurrentHealth / Actor->ReplicatedMaxHealth;

                            ImColor greenColor(0, 255, 0, 255);
                            DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X - 20 + (HealthPercent * 40), ActorScreenLocation.Y + 40 }, greenColor, 10.f);

                            // Calculer la position du texte HP pour centrer le texte sur la barre de santé
                            float HPTextX = ActorScreenLocation.X - 10;
                            float HPTextY = ActorScreenLocation.Y + 34;

                            // Afficher le texte HP avec un fond noir pour le rendre plus visible
                            ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                            ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                            RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), PlayerColorHP, PlayerHPBar.c_str(), bgColor, textColor, 500);


                            //Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 47), PlayerColor, PlayerHP.c_str(), 500);  // TEXTE NO BAR
                        }
                    }

                    //==============================================================   DINO ESP   =================================================================
                    if (Actor->IsDino() && Settings.Visuals.DrawWildCreatures || Actor->IsDino() && Settings.Visuals.DrawTamedCreatures) {
                        bool IsTamed = Actor->IsTamed();

                        if (Settings.Visuals.HideFish)
                        {
                            if (Actor->IsFish(Actor->DinoNameTag.GetName())) continue;
                        }
                        if (Settings.Visuals.isBee)
                        {
                            if (Actor->IsDino() && Actor->DinoNameTag.GetName() == "Bee") continue;
                        }

                        if (Settings.Misc.NoglinAlert && Actor->IsTamed() && Actor->DinoNameTag.GetName() == "Gremlin")
                        {
                            NearbyNoglin = true;
                        }
                        else
                        {
                            NearbyNoglin = false;
                        }

                        if (IsTamed && Settings.Visuals.DrawTamedCreatures)
                        {
                            FVector2D ActorScreenLocation;
                            auto DinoChar = (APrimalDinoCharacter*)Actor;
                            auto PlayerData = (FPrimalPlayerDataStruct*)Actor;
                            auto PlayerD = Cache.LocalActor->GetPlayerData();

                            ImColor DinoColor;
                            int NewR = TamedDinoColor1[0] * 255;
                            int NewG = TamedDinoColor1[1] * 255;
                            int NewB = TamedDinoColor1[2] * 255;
                            DinoColor = ImColor(NewR, NewG, NewB, 255);


                            std::string DinoHPBar = std::string("" + std::to_string((int)Actor->ReplicatedCurrentHealth));
                            std::string DinoTorp1Bar = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor));

                            std::string DinoDatazz = Actor->DinoNameTag.GetName() + "   lvl  " + std::to_string(Actor->MyCharacterStatusComponent->BaseCharacterLevel + Actor->MyCharacterStatusComponent->ExtraCharacterLevel) + "";
                            std::string DinoHP = std::string("" + std::to_string((int)Actor->ReplicatedCurrentHealth) + "  /  " + std::to_string((int)Actor->ReplicatedMaxHealth) + " HP");;
                            std::string DinoTorp1 = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor) + "  /  " + std::to_string((int)Actor->ReplicatedMaxTorpor) + " Torp");
                            /*
                            if (Settings.Visuals.TamedDinoTorp)
                            {
                                DinoTorp1 = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor) + "  /  " + std::to_string((int)Actor->ReplicatedMaxTorpor) + " Torp");
                            }*/

                            bool TeamDino = false;

                            if (Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                            {
                                isTribeDinoOrPlayer = true;
                            }
                            if (isTribeDinoOrPlayer && !isDead && !isSleeping)
                            {
                                DinoColor = ImColor(2, 125, 0, 255);
                            }
                            if (Actor->IsDead())
                            {
                                isDead = true;
                                if (Settings.Visuals.DinoDead)
                                {
                                    DinoColor = ImColor(235, 119, 0, 255);
                                }
                            }
                            if (!Actor->IsConscious() && !isDead)
                            {
                                isSleeping = true;
                                if (Settings.Visuals.DrawSleepingDinos)
                                {
                                    DinoColor = ImColor(255, 237, 237, 255);
                                }
                            }

                            if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                            {
                                if (!Settings.Visuals.DinoDead && isDead) continue;
                                if (Settings.Visuals.HideTeamDinos && isTribeDinoOrPlayer) continue;

                                if (isDead && Settings.Visuals.DinoDead)
                                {
                                    Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 90), DinoColor, "Dead", 500);
                                }
                                if (isSleeping && Settings.Visuals.DrawSleepingDinos && !isDead)
                                {
                                    Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 90), DinoColor, "Sleeping", 500);
                                }
                                if (Settings.Visuals.DrawDinoDistance && !isSleeping)
                                {
                                    int Distance = Cache.LocalActor->RootComponent->GetWorldLocation().DistTo(Actor->RootComponent->GetWorldLocation()) * 0.01f;
                                    std::string DistanceString = "" + std::to_string(Distance) + " M";
                                    Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X + 100, ActorScreenLocation.Y - 75), DinoColor, DistanceString.c_str(), 500);  // Metre
                                }
                                if (Settings.Visuals.TamedDinoTorp && !isSleeping && !isDead)
                                {

                                    //TorpBar
                                    ImColor grayColor(128, 128, 128, 255);
                                    DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X + 30, ActorScreenLocation.Y + 50 }, grayColor, 10.f);
                                    float HealthPercent = Actor->ReplicatedCurrentTorpor / Actor->ReplicatedMaxTorpor;
                                    ImColor mauveColor(128, 0, 128, 255);
                                    DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X - 30 + (HealthPercent * 50), ActorScreenLocation.Y + 50 }, mauveColor, 10.f);
                                    float HPTextX = ActorScreenLocation.X - 10;
                                    float HPTextY = ActorScreenLocation.Y + 45;
                                    ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                    ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                    RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), DinoColor, DinoTorp1Bar.c_str(), bgColor, textColor, 500);




                                    //Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X + 100, ActorScreenLocation.Y - 35), DinoColor, DinoTorp1.c_str(), 500);// Torp TESXT NO BAR
                                }
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X + 100, ActorScreenLocation.Y - 60), DinoColor, DinoDatazz.c_str(), 500);   // NAME + LVL
                                if (isDead) continue;


                                //HP BAR
                                ImColor redColor(255, 0, 0, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X + 20, ActorScreenLocation.Y + 40 }, redColor, 10.f);
                                float HealthPercent = Actor->ReplicatedCurrentHealth / Actor->ReplicatedMaxHealth;
                                ImColor greenColor(0, 255, 0, 255);
                                DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X - 20 + (HealthPercent * 40), ActorScreenLocation.Y + 40 }, greenColor, 10.f);
                                float HPTextX = ActorScreenLocation.X - 10;
                                float HPTextY = ActorScreenLocation.Y + 34;
                                ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), DinoColor, DinoHPBar.c_str(), bgColor, textColor, 500);


                               // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X + 100, ActorScreenLocation.Y - 47), DinoColor, DinoHP.c_str(), 500);      // HP  TEST NO HP BAR
                            }
                        } 
                        //==============================================================   DINO ESP   =================================================================
                        if (!IsTamed && Settings.Visuals.DrawWildCreatures)
                        {
                            FVector2D ActorScreenLocation;
                            ImColor DinoColorWild;
                            ImColor FilteredDino;

                            FilteredDino = ImColor(184, 0, 131, 255);
                            DinoColorWild = ImColor(0, 255, 255, 255);


                            int NewR = WildDinoColor1[0] * 255;
                            int NewG = WildDinoColor1[1] * 255;
                            int NewB = WildDinoColor1[2] * 255;
                            DinoColorWild = ImColor(NewR, NewG, NewB, 255);

                            int NewR1 = FilteredDinoColor1[0] * 255;
                            int NewG1 = FilteredDinoColor1[1] * 255;
                            int NewB1 = FilteredDinoColor1[2] * 255;
                            FilteredDino = ImColor(NewR1, NewG1, NewB1, 255);

                            std::string DinoHPBar = std::string("" + std::to_string((int)Actor->ReplicatedCurrentHealth));
                            std::string DinoTorp1Bar = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor));

                            std::string DinoDatazz = Actor->DinoNameTag.GetName() + "   lvl  " + std::to_string(Actor->MyCharacterStatusComponent->BaseCharacterLevel + Actor->MyCharacterStatusComponent->ExtraCharacterLevel) + "";
                            std::string DinoHP = std::string("" + std::to_string((int)Actor->ReplicatedCurrentHealth) + "  /  " + std::to_string((int)Actor->ReplicatedMaxHealth) + " HP");
                            std::string DinoTorp1 = std::string("" + std::to_string((int)Actor->ReplicatedCurrentTorpor) + "  /  " + std::to_string((int)Actor->ReplicatedMaxTorpor) + " Torp");


                            if (Settings.Visuals.WildDinoFilter)
                            {
                                CompareName = RealDinoNames[PossibleDinos];
                                if (Actor->DinoNameTag.GetName() == CompareName)
                                {
                                    if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                                    {
                                        if (Settings.Visuals.WildDinoTorp)
                                        {
                                            //HP BAR
                                            ImColor redColor(255, 0, 0, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X + 20, ActorScreenLocation.Y + 40 }, redColor, 10.f);
                                            float HealthPercent = Actor->ReplicatedCurrentHealth / Actor->ReplicatedMaxHealth;
                                            ImColor greenColor(0, 255, 0, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X - 20 + (HealthPercent * 40), ActorScreenLocation.Y + 40 }, greenColor, 10.f);
                                            float HPTextX = ActorScreenLocation.X - 10;
                                            float HPTextY = ActorScreenLocation.Y + 34;
                                            ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                            ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                            RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), DinoColorWild, DinoHPBar.c_str(), bgColor, textColor, 500);


                                            //TorpBar
                                            ImColor grayColor(128, 128, 128, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X + 30, ActorScreenLocation.Y + 50 }, grayColor, 10.f);
                                            float HealthPercent2 = Actor->ReplicatedCurrentTorpor / Actor->ReplicatedMaxTorpor;
                                            ImColor mauveColor(128, 0, 128, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X - 30 + (HealthPercent2 * 50), ActorScreenLocation.Y + 50 }, mauveColor, 10.f);
                                            float HPTextX2 = ActorScreenLocation.X - 10;
                                            float HPTextY2 = ActorScreenLocation.Y + 45;
                                            ImColor textColor2(0, 0, 0, 255); // Couleur du texte (blanc)
                                            ImColor bgColor2(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                            RenderTextWithBackground(DrawList, ImVec2(HPTextX2, HPTextY2), DinoColorWild, DinoTorp1Bar.c_str(), bgColor2, textColor2, 500);


                                            Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 75), DinoColorWild, DinoDatazz.c_str(), 500);

                                            // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 60), DinoColorWild, DinoHP.c_str(), 500);    // HP TEXT NO BAR
                                            // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 47), DinoColorWild, DinoTorp1.c_str(), 500);  // TORP TEXT NO BAR
                                        }
                                        else
                                        {
                                            Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 75), DinoColorWild, DinoDatazz.c_str(), 500);
                                        }
                                    }
                                }
                            }
                            else if (Settings.Visuals.AlphaFilter && Actor->IsAlpha(Actor->DinoNameTag.GetName()))
                            {
                                if (Actor->DinoNameTag.GetName() == "Elite Raptor" || Actor->DinoNameTag.GetName() == "Elite Mega" || Actor->DinoNameTag.GetName() == "Elite Rex" || Actor->DinoNameTag.GetName() == "Elite Carno")
                                {
                                    if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                                    {
                                        if (Settings.Visuals.WildDinoTorp)
                                        {
                                            //HP BAR
                                            ImColor redColor(255, 0, 0, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X + 20, ActorScreenLocation.Y + 40 }, redColor, 10.f);
                                            float HealthPercent = Actor->ReplicatedCurrentHealth / Actor->ReplicatedMaxHealth;
                                            ImColor greenColor(0, 255, 0, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X - 20 + (HealthPercent * 40), ActorScreenLocation.Y + 40 }, greenColor, 10.f);
                                            float HPTextX = ActorScreenLocation.X - 10;
                                            float HPTextY = ActorScreenLocation.Y + 34;
                                            ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                            ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                            RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), DinoColorWild, DinoHPBar.c_str(), bgColor, textColor, 500);


                                            //TorpBar
                                            ImColor grayColor(128, 128, 128, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X + 30, ActorScreenLocation.Y + 50 }, grayColor, 10.f);
                                            float HealthPercent2 = Actor->ReplicatedCurrentTorpor / Actor->ReplicatedMaxTorpor;
                                            ImColor mauveColor(128, 0, 128, 255);
                                            DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X - 30 + (HealthPercent2 * 50), ActorScreenLocation.Y + 50 }, mauveColor, 10.f);
                                            float HPTextX2 = ActorScreenLocation.X - 10;
                                            float HPTextY2 = ActorScreenLocation.Y + 45;
                                            ImColor textColor2(0, 0, 0, 255); // Couleur du texte (blanc)
                                            ImColor bgColor2(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                            RenderTextWithBackground(DrawList, ImVec2(HPTextX2, HPTextY2), DinoColorWild, DinoTorp1Bar.c_str(), bgColor2, textColor2, 500);


                                            Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 75), DinoColorWild, DinoDatazz.c_str(), 500);

                                            // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 60), DinoColorWild, DinoHP.c_str(), 500);    // HP TEXT NO BAR
                                            // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 47), DinoColorWild, DinoTorp1.c_str(), 500);  // TORP TEXT NO BAR
                                        }
                                        else
                                        {
                                            Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 75), DinoColorWild, DinoDatazz.c_str(), 500);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (Settings.Visuals.AlphaFilter) continue;
                                if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                                {
                                    if (Settings.Visuals.WildDinoTorp)
                                    {
                                        //HP BAR
                                        ImColor redColor(255, 0, 0, 255);
                                        DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X + 20, ActorScreenLocation.Y + 40 }, redColor, 10.f);
                                        float HealthPercent = Actor->ReplicatedCurrentHealth / Actor->ReplicatedMaxHealth;
                                        ImColor greenColor(0, 255, 0, 255);
                                        DrawList->AddLine({ ActorScreenLocation.X - 20, ActorScreenLocation.Y + 40 }, { ActorScreenLocation.X - 20 + (HealthPercent * 40), ActorScreenLocation.Y + 40 }, greenColor, 10.f);
                                        float HPTextX = ActorScreenLocation.X - 10;
                                        float HPTextY = ActorScreenLocation.Y + 34;
                                        ImColor textColor(0, 0, 0, 255); // Couleur du texte (blanc)
                                        ImColor bgColor(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                        RenderTextWithBackground(DrawList, ImVec2(HPTextX, HPTextY), DinoColorWild, DinoHPBar.c_str(), bgColor, textColor, 500);


                                        //TorpBar
                                        ImColor grayColor(128, 128, 128, 255);
                                        DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X + 30, ActorScreenLocation.Y + 50 }, grayColor, 10.f);
                                        float HealthPercent2 = Actor->ReplicatedCurrentTorpor / Actor->ReplicatedMaxTorpor;
                                        ImColor mauveColor(128, 0, 128, 255);
                                        DrawList->AddLine({ ActorScreenLocation.X - 30, ActorScreenLocation.Y + 50 }, { ActorScreenLocation.X - 30 + (HealthPercent2 * 50), ActorScreenLocation.Y + 50 }, mauveColor, 10.f);
                                        float HPTextX2 = ActorScreenLocation.X - 10;
                                        float HPTextY2 = ActorScreenLocation.Y + 45;
                                        ImColor textColor2(0, 0, 0, 255); // Couleur du texte (blanc)
                                        ImColor bgColor2(0, 0, 0, 128); // Couleur d'arrière-plan (noir semi-transparent)
                                        RenderTextWithBackground(DrawList, ImVec2(HPTextX2, HPTextY2), DinoColorWild, DinoTorp1Bar.c_str(), bgColor2, textColor2, 500);


                                        Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 75), DinoColorWild, DinoDatazz.c_str(), 500);

                                       // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 60), DinoColorWild, DinoHP.c_str(), 500);    // HP TEXT NO BAR
                                       // Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 47), DinoColorWild, DinoTorp1.c_str(), 500);  // TORP TEXT NO BAR
                                    }
                                    else
                                    {
                                        Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 75), DinoColorWild, DinoDatazz.c_str(), 500);
                                    }
                                }
                            }
                        }
                    }
                    
                    //============================================================   ESP  TURRET    ============================================================================
                    std::string TurretShortName;
                    if (Actor->IsTurret(Actor->Name.GetName(), TurretShortName) && Settings.Visuals.DrawTurrets)
                    {
                        ImColor TurretColor;
                        FVector2D ActorScreenLocation;
                        int NewR = TurretColor1[0] * 255;
                        int NewG = TurretColor1[1] * 255;
                        int NewB = TurretColor1[2] * 255;
                        TurretColor = ImColor(NewR, NewG, NewB, 255);
                        if (Settings.Visuals.HideTeamTurrets && isTribeDinoOrPlayer) continue;
                        if (!Settings.Visuals.ShowEmptyTurrets && Actor->NumBullets == 0) continue;
                        if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                        {
                            if (Settings.Visuals.DrawTurretsDistance)
                            {
                                int Distance = Cache.LocalActor->RootComponent->GetWorldLocation().DistTo(Actor->RootComponent->GetWorldLocation()) * 0.01f;
                                std::string DistanceString = "" + std::to_string(Distance) + " M";
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 13), TurretColor, DistanceString.c_str(), 500);
                            }
                            if (Settings.Visuals.DrawTurrets3)
                            {
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y), TurretColor, TurretShortName.c_str(), 500);
                            }
                            if (Settings.Visuals.ShowBulletCount && Actor->NumBullets != 0)
                            {
                                std::string BulletCount = " " + std::to_string(Actor->NumBullets) + "";
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y + 12), TurretColor, BulletCount.c_str(), 500);
                            }
                        }
                    }

                    //=========================================================   ESP  Container    ============================================================================
                    if (Actor->IsItemContainer() && Settings.Visuals.DrawContainers && Actor->DescriptiveName.Data)
                    {
                        FVector2D ActorScreenLocation;

                        ImColor ContainerColor;
                        ImColor FilteredContainer;

                        int NewR = ContainerColor1[0] * 255;
                        int NewG = ContainerColor1[1] * 255;
                        int NewB = ContainerColor1[2] * 255;
                        ContainerColor = ImColor(NewR, NewG, NewB, 255);

                        FilteredContainer = ImColor(70, 30, 40, 255);

                        if (Settings.Visuals.HideTeamContainers && isTribeDinoOrPlayer) continue;
                        if (Actor->IsExcludedContainer(Actor->DescriptiveName.ToString())) continue;
                        if (!Settings.Visuals.ShowEmptyContainers && Actor->CurrentItemCount == 0) continue;

                        std::string ContainerString = Actor->DescriptiveName.ToString();

                        if (Settings.Visuals.ContainerFilter)
                        {
                            if (ContainerString != CurrentContainer) continue;
                        }

                        if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                        {
                            if (Settings.Visuals.ContainerDistance)
                            {
                                int Distance = Cache.LocalActor->RootComponent->GetWorldLocation().DistTo(Actor->RootComponent->GetWorldLocation()) * 0.01f;
                                std::string DistanceString = "" + std::to_string(Distance) + " M";
                                Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y - 20), ContainerColor, DistanceString.c_str(), 500);
                            }
                            if (Actor->CurrentItemCount != 0) { ContainerString += " \n" + std::to_string(Actor->CurrentItemCount) + "  /  " + std::to_string(Actor->MaxItemCount) + ""; }
                            Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y), ContainerColor, ContainerString.c_str(), 500);
                        }
                    }

                    //=========================================================   ESP  Structure    ============================================================================
                    if (Actor->IsStructure() && Settings.Visuals.DrawStructure && Actor->DescriptiveName.Data)
                    {
                        FVector2D ActorScreenLocation;
                        ImColor StructureColor;

                        int NewR = StructureColor1[0] * 255;
                        int NewG = StructureColor1[1] * 255;
                        int NewB = StructureColor1[2] * 255;
                        StructureColor = ImColor(NewR, NewG, NewB, 255);

                        if (Actor->IsExcludedContainer2(Actor->DescriptiveName.ToString())) continue;
                        std::string ContainerString = Actor->DescriptiveName.ToString();


                        if (W2S(Actor->RootComponent->GetWorldLocation(), ActorScreenLocation))
                        {
                            Renderer::Drawing::RenderText(ImVec2(ActorScreenLocation.X, ActorScreenLocation.Y), StructureColor, ContainerString.c_str(), 500);
                        }
                    }//=========================================================   ESP  Structure    ============================================================================
                }              
            }  
            if (Settings.Aimbot.EnableAimbot)
            {
                AimbotThread();
            }
            if (Settings.Misc.ExtraInfo)
            {
                std::vector<std::string> DisplayStrings{};
                if (true)
                {
                    DisplayStrings.push_back("Ark Cheat");
                    std::string AimbBindZz = Settings.Keybinds.AimbotKey;
                    DisplayStrings.push_back("Aim Key: " + AimbBindZz);

                }            
                if (Settings.Misc.ShowPlayers)
                {
                    auto Players = GWorld->GameState->NumPlayerConnected;
                    DisplayStrings.push_back("Connected Players: " + std::to_string((int)Players));
                }
                if (Settings.Misc.NumTamedDinos)
                {
                    auto dino = GWorld->GameState->NumTamedDinos;



                    DisplayStrings.push_back("Dino Tamed: " + std::to_string((int)dino));
                    
                }
                if (Settings.Misc.ShowFPS)
                {
                    auto IO = ImGui::GetIO();
                    DisplayStrings.push_back("FPS: " + std::to_string((int)IO.Framerate));
                }
                if (Settings.Misc.ShowXYZ && Cache.LocalActor)
                {

                    auto XYZ = Cache.LocalActor->RootComponent->GetWorldLocation();
                    DisplayStrings.push_back("[X]: " + std::to_string(XYZ.X) + " | [Y]: " + std::to_string(XYZ.Y) + " | [Z]: " + std::to_string(XYZ.Z));
                }
                if (NearbyNoglin && Settings.Misc.NoglinAlert)
                {
                    DisplayStrings.push_back("[!] Warning Noglin");
                }
                if (Settings.Visuals.ContainerFilter && Settings.Visuals.DrawContainers)
                {
                    DisplayStrings.push_back("Container Filter: " + std::string(CurrentContainer));
                }
                if (Settings.Visuals.WildDinoFilter && Settings.Visuals.DrawWildCreatures)
                {
                    DisplayStrings.push_back("[X] Wild Dino Filder: " + std::string(CompareName));

                }
                Renderer::Drawing::RenderCollapseFriendlyDisplayList(ImVec2(10, 10), ImColor(255, 255, 255, 255), DisplayStrings, 12.f);

            }
            //=================================================================   EXTRA INFO ============================================================================
            if (Settings.Misc.NoSway && Cache.LocalActor)
            {
                if (Cache.LocalActor->CurrentWeapon)
                {
                    if (Cache.LocalActor->CurrentWeapon->AimDriftPitchFrequency) { Cache.LocalActor->CurrentWeapon->AimDriftPitchFrequency = 0; }
                    if (Cache.LocalActor->CurrentWeapon->AimDriftYawFrequency) { Cache.LocalActor->CurrentWeapon->AimDriftYawFrequency = 0; }
                }
            }
            if (Settings.Misc.NoSpread && Cache.LocalActor)
            {
                if (Cache.LocalActor->CurrentWeapon)
                {
                    Cache.LocalActor->CurrentWeapon->InstantConfig.WeaponSpread = 0;
                    Cache.LocalActor->CurrentWeapon->InstantConfig.TargetingSpreadMod = 0;
                }
            }
            if (Settings.Misc.NoShake && Cache.LocalActor)
            {
                if (Cache.LocalActor->CurrentWeapon)
                {
                    Cache.LocalActor->CurrentWeapon->bUseFireCameraShakeScale = 0;
                    Cache.LocalActor->CurrentWeapon->GlobalFireCameraShakeScale = 0;
                    Cache.LocalActor->CurrentWeapon->ReloadCameraShakeSpeedScale = 0;
                    Cache.LocalActor->CurrentWeapon->GlobalFireCameraShakeScaleTargeting = 0;
                }
            }
            if (Settings.Misc.RapidFire && Cache.LocalActor)
            {
                auto Zoom = (AShooterWeapon_Rapid*)Cache.LocalActor->CurrentWeapon;
                if (Zoom)
                {
                    Zoom->WeaponConfig.TimeBetweenShots = 0.01;
                }
            }
            if (Settings.Misc.InfiniteOrbit && Cache.LocalActor)
            {
                Cache.LocalActor->OrbitCamMaxZoomLevel = 5000;
            }
            if (Settings.Misc.LongArms && Cache.LocalActor)
            {
                auto PrimalChar = (APrimalCharacter*)Cache.LocalActor;
                if (Settings.Misc.InfiniteArms)
                {
                    PrimalChar->AdditionalMaxUseDistance = 2500000000000000000;
                }
                else
                {
                    PrimalChar->AdditionalMaxUseDistance = 250;
                }
            }
            if (Settings.Visuals.DrawCrosshair) 
            {
                ImColor CrosshairColor;
                int NewR = CrosshairColor1[0] * 255;
                int NewG = CrosshairColor1[1] * 255;
                int NewB = CrosshairColor1[2] * 255;
                CrosshairColor = ImColor(NewR, NewG, NewB, 255);
                if (CrosshairColor && Settings.Visuals.DrawCrosshair)
                {
                    Renderer::Drawing::RenderCrosshair(CrosshairColor, Settings.Visuals.CrosshairWidth);
                }
            }
            if (Settings.Visuals.DrawAimFOV) 
            {
                ImColor CrosshairFOVcolor;
                int NewR = CrosshairColor1[0] * 255;
                int NewG = CrosshairColor1[1] * 255;
                int NewB = CrosshairColor1[2] * 255;
                CrosshairFOVcolor = ImColor(NewR, NewG, NewB, 255);
                Renderer::Drawing::RenderAimFOV(CrosshairFOVcolor);
            }
            if (Settings.Misc.RhinoCharge && Cache.LocalActor)
            {
                auto RiddenDino = Cache.LocalActor->GetBasedOrSeatingOnDino();
                if (RiddenDino)
                {
                    RiddenDino->ScaleExtraRunningSpeedModifierSpeed = Settings.Misc.NewSpeed3;
                    RiddenDino->ExtraRunningSpeedModifier = Settings.Misc.NewSpeed3;
                    RiddenDino->AllowRidingMaxDistance = Settings.Misc.NewSpeed3;
                    RiddenDino->ScaleExtraRunningSpeedModifierSpeed = Settings.Misc.NewSpeed3;
                    RiddenDino->ChargeSpeedMultiplier = Settings.Misc.NewSpeed3;
                }
            }
            if (Cache.NearbyEnemies > 0) 
            {
                if (Cache.NearbyEnemies)
                {
                    std::string NearbyPlayers;
                    NearbyPlayers = " [Warning Enemies: " + std::to_string(Cache.NearbyEnemies) + "]";
                    Renderer::Drawing::RenderText2(ImVec2(925, 295), ImColor(0, 0, 0, 255), NearbyPlayers.c_str(), 10000);
                    Renderer::Drawing::RenderText2(ImVec2(925, 295), ImColor(255, 0, 0, 255), NearbyPlayers.c_str(), 10000);
                }
            }
            if (Settings.Misc.InstantDinoTurn && Cache.LocalActor)
            {
                auto RiddenDino = Cache.LocalActor->GetBasedOrSeatingOnDino();
                if (RiddenDino)
                {
                    RiddenDino->RiderFlyingRotationRateModifier = Settings.Misc.NewSpeed2;
                    RiddenDino->RiderMovementSpeedScalingRotationRatePowerMultiplier = Settings.Misc.NewSpeed2;
                    RiddenDino->WalkingRotationRateModifier = Settings.Misc.NewSpeed2;
                }
            }
            if (Settings.Visuals.Radar2D)
            {
                radar2D();
            }
            if (Settings.Visuals.DrawAimingLine)
            {
                DrawAimingLine();
            }
            if (Settings.Visuals.DrawLine)
            {
                //MireLine();
                DrawLine();
            }
            if (Settings.Misc.explorernotehack && Cache.LPC)
            {
                for (int i = 0; i < 2000; i++)
                {
                    Cache.LPC->UnlockExplorerNote(i, false);
                }
            }
            if (Settings.Misc.TekPunch)
            {
                bool TekPunchKeyDown = Cache.LPC->IsInputKeyDown(Settings.Keybinds.TekPunchKey);
                auto GloveBuff = (ABuff_TekArmor_Gloves_C*)Cache.LPC->GetPlayerCharacter()->GetBuff(ABuff_TekArmor_Gloves_C::StaticClass());
                if (GloveBuff && TekPunchKeyDown)
                {
                    GloveBuff->Server_SetPunchChargeState(E_TekGlovePunchState(3));
                }
            }
        } while (false);
    }
    catch (std::exception E)
    {
        Logger::Log("[EXECUTION_LOOP]: Execution Loop Failed! [EXCEPTION]: %s\n", E);
    }
    catch (...)
    {
        Logger::Log("[EXECUTION_LOOP]: Execution Loop Failed! [DEBUG_STRING]: %s\n", DebugString);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
    if (Settings.IsMenuOpen)
    {
        Renderer::HookInput();

        ImGui::Begin("Cheat!", &Settings.IsMenuOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
        //ImGui::SetWindowPos(ImVec2(Cache.WindowSizeX / 2 - Settings.MenuSizeX / 2, Cache.WindowSizeY / 2.5 - Settings.MenuSizeY / 2));
        ImGui::SetWindowSize(ImVec2(Settings.MenuSizeX, Settings.MenuSizeY));

        CheatMenu();

        ImGui::End();
    }
    D3D.Ctx->OMSetRenderTargets(1, &D3D.RenderTargetView, nullptr);
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return D3D.OriginalPresent(SwapChain, SyncInterval, Flags);
}
std::string RetrieveUWPFolder()
{
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath))) // Get Mod Files Path Folder
    {
        std::string data = std::string(szPath) + std::string("\\wdsadw.txt");
        return std::string(szPath) + std::string("\\wdsadw.txt");
    }
}
std::string RetrieveUWPFolder2()
{
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath))) // Get Mod Files Path Folder
    {
        std::string data = std::string(szPath) + std::string("\\details.txt");
        return std::string(szPath) + std::string("\\details.txt");
    }
}
std::string InsertDoubleSlashes(std::string CurrentPath)
{
    std::stringstream Stream;
    for (int i = 0; i < CurrentPath.length(); ++i)
    {
        if (CurrentPath[i] == '\\') Stream << "\\\\";
        else Stream << CurrentPath[i];
    }
    return Stream.str();
}
std::wstring s2ws(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
void RenderMenu()
{
    std::string zzgiwd = InsertDoubleSlashes(RetrieveUWPFolder());
    std::string zzgiwd2 = InsertDoubleSlashes(RetrieveUWPFolder2());
    std::wstring zinger = s2ws(zzgiwd);
    std::ifstream file(zzgiwd2.c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string str = buffer.str();

    uintptr_t GameBase = (uintptr_t)GetModuleHandle(nullptr);
    UWorld::GWorld = *reinterpret_cast<decltype(UWorld::GWorld)*>(GameBase + uWorld_Offset);
    Cache.GameBase = GameBase;
    Settings.OriginalGetBoneLocation = reinterpret_cast<decltype(Settings.OriginalGetBoneLocation)>(PatternScan(GameBase, "40 57 48 83 EC 70 48 C7 44 24 ? ? ? ? ? 48 89 9C 24 ? ? ? ? 48 8B DA 48 8B F9 49 8B D0 E8 ? ? ? ? 83 F8 FF"));
    Settings.GetBoneLocation = reinterpret_cast<decltype(Settings.GetBoneLocation)>(Settings.OriginalGetBoneLocation);
    Settings.OriginalInputKey = reinterpret_cast<decltype(Settings.OriginalInputKey)>(PatternScan(GameBase, "48 8B C4 48 89 50 10 56 57 41 56 48 81 EC ? ? ? ? 48 C7 44 24 ? ? ? ? ? 48 89 58 18 48 89 68 20 0F 29 70 D8"));
    Settings.InputKey = reinterpret_cast<decltype(Settings.InputKey)>(Settings.OriginalInputKey);
    if (!Logger::Init(zinger.c_str())) return;
    if (!Renderer::Init()) { return Logger::Log("[RENDERER]: Renderer Failed To Initialize!\n"); };
    if (!InitSDK()) { return Logger::Log("[SDK]: SDK Failed To Initialize!\n"); }
    Logger::Log("Get Folder: %s \n", zzgiwd2.c_str());
    Logger::Log("File Contents: %s \n", str.c_str());
    Logger::Log("[GObjects]: 0x%llX\n", UObject::GObjects);
    Logger::Log("[GNames]: 0x%llX\n", FName::GNames);
    Logger::Log("[UWorld]: 0x%llX\n", UWorld::GWorld);
    Logger::Log("[PersistentLevel]: 0x%llX\n", UWorld::GWorld->PersistentLevel);
    Logger::Log("[Key Input]: 0x%llX\n", Settings.OriginalInputKey);
    Logger::Log("[Bone Location]: 0x%llX\n", Settings.OriginalGetBoneLocation);
    Logger::Log("[Game Base]: 0x%llX\n", GameBase);
    Logger::Log("[ProcessEvents]: %p\n", reinterpret_cast<uintptr_t>(OProcessEvent)); // Working
}
FVector RotatorToVector(const FRotator& Rotator)
{
    float CP = cosf(Rotator.Pitch * DEG_TO_RAD);
    float SP = sinf(Rotator.Pitch * DEG_TO_RAD);
    float CY = cosf(Rotator.Yaw * DEG_TO_RAD);
    float SY = sinf(Rotator.Yaw * DEG_TO_RAD);
    float CR = cosf(Rotator.Roll * DEG_TO_RAD);
    float SR = sinf(Rotator.Roll * DEG_TO_RAD);

    FVector Forward;
    Forward.X = CP * CY;
    Forward.Y = CP * SY;
    Forward.Z = -SP; // Change the sign here

    return Forward;
}
void DrawAimingLine()
{
    UWorld* GWorld = UWorld::GWorld;
    auto Actors = UWorld::GWorld->PersistentLevel->Actors;
    AActor* LocalPawn = Cache.LocalActor;
    const ImU32 aimingLineColor = IM_COL32(255, 0, 0, 255); // Red color for the aiming line
    bool isTribeDinoOrPlayer = false;

    for (int i = 0; i < Actors.Count; i++)
    {
        auto Actor = Actors[i];
        if (Actor && Actor->IsPlayer() && !Actor->IsPrimalCharFriendly((APrimalCharacter*)LocalPawn))
        {
            // Enemy player conditions
            if (Actor->IsPlayer() && Actor->IsConscious() && !Actor->IsDead() && !Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
            {
                if (!isTribeDinoOrPlayer) // only enemy
                {
                    FVector PlayerLocation = Actor->K2_GetActorLocation();
                    FRotator PlayerRotation = Actor->K2_GetActorRotation();

                    // Calculate the aiming direction using the player rotation
                    FVector AimDirection = RotatorToVector(PlayerRotation);

                    // Calculate the end point of the line
                    FVector LineEndPoint = PlayerLocation + AimDirection * 2000.0f; // You can adjust the length of the line here

                    FVector2D PlayerScreenPos, LineEndPointScreenPos;

                    // Convert world coordinates to screen coordinates
                    if (W2S(PlayerLocation, PlayerScreenPos) && W2S(LineEndPoint, LineEndPointScreenPos))
                    {
                        auto DrawList = ImGui::GetWindowDrawList();
                        DrawList->AddLine(ImVec2(PlayerScreenPos.X, PlayerScreenPos.Y), ImVec2(LineEndPointScreenPos.X, LineEndPointScreenPos.Y), aimingLineColor);
                    }
                }
            }
        }
    }
}
void AimbotThread()
{
    do
    {
        try
        {
            // Get game world and local player
            UWorld* GWorld = UWorld::GWorld;
            UPlayer* LocalPlayer = GWorld->OwningGameInstance->LocalPlayers[0];
            auto PC = LocalPlayer->PlayerController;

            // Screen dimensions
            auto WinSizeX = ImGui::GetWindowSize().x;
            auto WinSizeY = ImGui::GetWindowSize().y;

            FVector2D BoneScreenLocation;
            FVector BoneWorldLocation;
            int LastDistance = WinSizeX;

            // Check if aimbot key is down
            bool AimKeyDown = Cache.LPC->IsInputKeyDown(Settings.Keybinds.AimbotKey);
            if (!AimKeyDown) 
            {
                Cache.LPC->IgnoreLookInput(false);
                Settings.Aimbot.AimLocked = false;
                Cache.AimbotTarget = nullptr;
            }

            if (!Settings.Aimbot.AimLocked) {
                auto Actors = UWorld::GWorld->PersistentLevel->Actors;
                for (int i = 0; i < Actors.Count; i++) {
                    auto Actor = Actors[i];
                    if (Actor && Actor->IsPlayer()) {
                        // Get bone location
                        Settings.GetBoneLocation(Actor->MeshComponent, &BoneWorldLocation, Actor->MeshComponent->GetBoneName(AimBone), 0);
                        // Check if bone is within aimbot FOV
                        if (!W2S(BoneWorldLocation, BoneScreenLocation) || !Renderer::Drawing::WithinAimFOV(WinSizeX / 2, WinSizeY / 2, Settings.Visuals.FOVSize, BoneScreenLocation.X, BoneScreenLocation.Y)) continue;
                        // Skip dead or local players
                        if (Actor->IsDead() || Actor->IsLocalPlayer()) continue;
                        // Skip same tribe players if targeting only enemies
                        if (Settings.Aimbot.TargetTribe && Actor->TribeName.IsValid() && Actor->TribeName.ToString() == Cache.LocalActor->TribeName.ToString()) continue;
                        // Skip sleepers if targeting only conscious players
                        if (!Settings.Aimbot.TargetSleepers && !Actor->IsConscious()) continue;
                        // Adjust aim for target and local player velocity
                        if (Cache.AimbotTarget)
                        {
                            FVector TheirVelocity = Cache.AimbotTarget->CharacterMovement->Velocity;
                            FVector MyVelocity = Cache.LocalActor->CharacterMovement->Velocity;

                            if (TheirVelocity != MyVelocity) {
                                FVector TheSpeed = MyVelocity - TheirVelocity;
                                BoneWorldLocation += BoneWorldLocation + (TheSpeed / TheirVelocity);
                            }
                        }

                        // Check if target is visible
                        if (Settings.Aimbot.VisibleOnly && !Cache.LPC->LineOfSightTo(Actor, Cache.LPC->PlayerCameraManager->GetCameraLocation(), false)) continue;

                        // Update closest target
                        int Distance = Renderer::Drawing::ReturnDistance(WinSizeX / 2, WinSizeY / 2, BoneScreenLocation.X, BoneScreenLocation.Y);
                        if (Distance < LastDistance) {
                            LastDistance = Distance;
                            Cache.AimbotTarget = Actor;
                        }
                    }
                }

                // Lock onto target
                if (AimKeyDown && Cache.AimbotTarget) {
                    Cache.LPC->IgnoreLookInput(true);
                    Settings.Aimbot.AimLocked = true;
                }
            }
            if (Settings.Aimbot.AimLocked && Cache.AimbotTarget) {
                auto Target = Cache.AimbotTarget;
                FRotator ControlRotation = Cache.LPC->PlayerCameraManager->GetCameraRotation();
                FVector CameraLoc = Cache.LPC->PlayerCameraManager->GetCameraLocation();

                if (Target) {
                    // Unlock if target is dead
                    if (Target->IsDead()) {
                        Cache.LPC->IgnoreLookInput(false);
                        Settings.Aimbot.AimLocked = false;
                        Cache.AimbotTarget = nullptr;
                    }

                    if (Target->MeshComponent) {
                        // Get target bone location
                        Settings.GetBoneLocation(Target->MeshComponent, &BoneWorldLocation, Target->MeshComponent->GetBoneName(AimBone), 0);

                        // Calculate rotation to aim at target bone
                        auto Rotator = Cache.LocalPlayer->FindLookAtRotation(CameraLoc, BoneWorldLocation);

                        if (Rotator.hasValue() && Rotator.Yaw != 0 && Rotator.Pitch != 0) {
                            Rotator.Roll = 0;
                            Cache.LPC->ControlRotation = Rotator;
                        }
                    }
                }
            }
        }
        catch (const std::exception&) {
            // Catch any exceptions and continue
        }
    } while (false);
}
bool AimKeyDown = false;
void MireLine()
{
    auto Actors = UWorld::GWorld->PersistentLevel->Actors;
    AActor* LocalPawn = Cache.LocalActor;
    bool isTribeDinoOrPlayer = false;

    for (int i = 0; i < Actors.Count; i++)
    {
        auto Actor = Actors[i];
        if (Actor && Actor->IsPlayer() && !Actor->IsPrimalCharFriendly((APrimalCharacter*)LocalPawn))
        {
            FVector LocalPawnLocation = LocalPawn->K2_GetActorLocation();
            FVector ActorLocation = Actor->K2_GetActorLocation();

            // Afficher la position du joueur à l'écran
            auto XYZ = LocalPawnLocation;
            std::vector<std::string> DisplayStrings{};
            DisplayStrings.push_back("[X]: " + std::to_string(XYZ.X) + " | [Y]: " + std::to_string(XYZ.Y) + " | [Z]: " + std::to_string(XYZ.Z));
            float lineHeight = 50.0f; // Hauteur en pixels depuis le haut de l'écran pour les lignes
            for (const auto& String : DisplayStrings)
            {
                //Renderer::Drawing::AddText(ImVec2(10.f, lineHeight), ImColor(1.0f, 1.0f, 1.0f), String.c_str(), 300);
                lineHeight += 20.0f;
            }

            isTribeDinoOrPlayer = true;
        }
    }
}
void DrawLine()
{
    auto Actors = UWorld::GWorld->PersistentLevel->Actors;
    AActor* LocalPawn = Cache.LocalActor;
    bool isTribeDinoOrPlayer = false;
    float lineHeight = 50.0f; // Hauteur en pixels depuis le haut de l'écran pour les lignes
    for (int i = 0; i < Actors.Count; i++)
    {
        auto Actor = Actors[i];
        if (Actor && Actor->IsPlayer() && !Actor->IsPrimalCharFriendly((APrimalCharacter*)LocalPawn))
        {
            FVector LocalPawnLocation = LocalPawn->K2_GetActorLocation();
            FVector ActorLocation = Actor->K2_GetActorLocation();
            FVector2D LocalPawnScreenPos, ActorScreenPos;
            const ImU32 playerColorENEMY = IM_COL32(255, 0, 0, 255);
            const ImU32 playerColorDENEMY = IM_COL32(255, 165, 0, 255);
            const ImU32 playerColorALLY = IM_COL32(60, 255, 0, 255);
            const ImU32 playerColorSALLY = IM_COL32(0, 128, 64, 255);
            const ImU32 playerColorSENEMY = IM_COL32(255, 255, 255, 255);
            // Convertir les coordonnées du monde en coordonnées d'écran
            if (W2S(LocalPawnLocation, LocalPawnScreenPos) && W2S(ActorLocation, ActorScreenPos))
            {
                if (Settings.Visuals.DEnemy)
                {
                    //Enemy Dead
                    if (Actor->IsPlayer() && Actor->IsDead() && !Actor->IsConscious() && Actor != Cache.LocalActor)
                    {
                        auto WindowDrawList = ImGui::GetWindowDrawList();

                        if (Settings.Visuals.TopScreen)
                        {
                            ImVec2 lineStart(LocalPawnScreenPos.X, lineHeight); // Modifie la position de départ des lignes
                            WindowDrawList->AddLine(lineStart, ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorDENEMY);
                        }
                        else
                        {
                            WindowDrawList->AddLine(ImVec2(LocalPawnScreenPos.X, LocalPawnScreenPos.Y), ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorDENEMY);
                        }
                    }
                }

                if (Settings.Visuals.SEnemy)
                {
                    //Enemy sleeper
                    if (Actor->IsPlayer() && !Actor->IsDead() && !Actor->IsConscious() && Actor != Cache.LocalActor)
                    {
                        auto WindowDrawList = ImGui::GetWindowDrawList();
                        if (Settings.Visuals.TopScreen)
                        {

                            ImVec2 lineStart(LocalPawnScreenPos.X, lineHeight); // Modifie la position de départ des lignes
                            WindowDrawList->AddLine(lineStart, ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorSENEMY);
                        }
                        else
                        {
                            WindowDrawList->AddLine(ImVec2(LocalPawnScreenPos.X, LocalPawnScreenPos.Y), ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorSENEMY);
                        }
                    }
                }

                if (Settings.Visuals.ASleeper)
                {
                    //Tribe Sleeper
                    if (Actor->IsPlayer() && !Actor->IsDead() && !Actor->IsConscious() && Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                    {
                        auto WindowDrawList = ImGui::GetWindowDrawList();

                        if (Settings.Visuals.TopScreen)
                        {

                            ImVec2 lineStart(LocalPawnScreenPos.X, lineHeight); // Modifie la position de départ des lignes
                            WindowDrawList->AddLine(lineStart, ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorSALLY);
                        }
                        else
                        {
                            WindowDrawList->AddLine(ImVec2(LocalPawnScreenPos.X, LocalPawnScreenPos.Y), ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorSALLY);
                        }
                    }
                }

                if (Settings.Visuals.Ally)
                {
                    //Player Ally
                    if (Actor->IsPlayer() && Actor->IsConscious() && !Actor->IsDead() && Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                    {
                        auto WindowDrawList = ImGui::GetWindowDrawList();

                        if (Settings.Visuals.TopScreen)
                        {

                            ImVec2 lineStart(LocalPawnScreenPos.X, lineHeight); // Modifie la position de départ des lignes
                            WindowDrawList->AddLine(lineStart, ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorALLY);
                        }
                        else
                        {
                            WindowDrawList->AddLine(ImVec2(LocalPawnScreenPos.X, LocalPawnScreenPos.Y), ImVec2(ActorScreenPos.X, ActorScreenPos.Y), playerColorALLY);
                        }
                    }
                }

                if (Settings.Visuals.Enemy)
                {
                    //Enemy player
                    if (Actor->IsPlayer() && Actor->IsConscious() && !Actor->IsDead() && !Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                    {
                        if (!isTribeDinoOrPlayer) // only enemy
                        {
                            // Check if target is visible and spawn to green
                            bool isVisible = Cache.LPC->LineOfSightTo(Actor, Cache.LPC->PlayerCameraManager->GetCameraLocation(), false);
                            const ImU32 playerColorVisible = IM_COL32(0, 255, 0, 255);
                            ImU32 currentLineColor = isVisible ? playerColorVisible : playerColorENEMY;

                            auto WindowDrawList = ImGui::GetWindowDrawList();

                            if (Settings.Visuals.TopScreen)
                            {
                                ImVec2 lineStart(LocalPawnScreenPos.X, lineHeight); // Modifie la position de départ des lignes
                                WindowDrawList->AddLine(lineStart, ImVec2(ActorScreenPos.X, ActorScreenPos.Y), currentLineColor);
                            }
                            else
                            {
                                WindowDrawList->AddLine(ImVec2(LocalPawnScreenPos.X, LocalPawnScreenPos.Y), ImVec2(ActorScreenPos.X, ActorScreenPos.Y), currentLineColor);
                            }
                        }
                    }
                }
            }
        }
    }
}
void radar2D()
{
    //RADAR
    auto Actors = UWorld::GWorld->PersistentLevel->Actors;
    const float radarWidth = 200.0f;
    const float radarHeight = 200.0f;
    const ImU32 backgroundColor = IM_COL32(95, 95, 95, 95);
    const ImU32 radarColor = IM_COL32(0, 0, 0, 255);
    ImGuiIO& io = ImGui::GetIO();
    auto BackgroundDrawList = ImGui::GetBackgroundDrawList();
    // Obtenir la taille de l'écran
    ImVec2 displaySize = io.DisplaySize;
    // Position en haut à droite
    float margin = 100.0f; // Ajoutez une marge pour éviter que le radar ne soit collé au bord de l'écran
    float radarPosX = displaySize.x - radarWidth - margin;
    float radarPosY = margin;
    const ImVec2 cursorPos = ImVec2(radarPosX, radarPosY);
    const float centerX = cursorPos.x + 100.0f;
    const float centerY = cursorPos.y + 100.0f;
    const ImVec2 center = ImVec2(centerX, centerY);
    BackgroundDrawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + radarWidth, cursorPos.y + radarHeight), backgroundColor); // Draw the background rectangle
    BackgroundDrawList->AddRect(cursorPos, ImVec2(cursorPos.x + radarWidth, cursorPos.y + radarHeight), radarColor); // Draw the outer rectangle of the radar
    BackgroundDrawList->AddLine(ImVec2(center.x, cursorPos.y), ImVec2(center.x, cursorPos.y + radarHeight), radarColor); // Draw the cross lines at the center of the radar
    BackgroundDrawList->AddLine(ImVec2(cursorPos.x, center.y), ImVec2(cursorPos.x + radarWidth, center.y), radarColor);
    for (int i = 0; i < Actors.Count; i++)
    {
        auto Actor = Actors[i];
        if (Actor)
        {
            bool isTribeDinoOrPlayer = false;

            if (Settings.Visuals.Radar2D)
            {
                auto WindowDrawList = ImGui::GetWindowDrawList();
                const float playerRadius = 2.0f;
                const ImU32 playerColorENEMY = IM_COL32(255, 0, 0, 255);
                const ImU32 playerColorDENEMY = IM_COL32(255, 165, 0, 255);
                const ImU32 playerColorALLY = IM_COL32(60, 255, 0, 255);
                const ImU32 playerColorSALLY = IM_COL32(0, 128, 64, 255);
                const ImU32 playerColorSENEMY = IM_COL32(255, 255, 255, 255);
                AActor* LocalPawn = Cache.LocalActor;
                FVector LocalPawnLocation = LocalPawn->K2_GetActorLocation();
                FVector ActorLocation = Actor->K2_GetActorLocation();
                FVector relativeLocation = ActorLocation - LocalPawnLocation;
                //INFO

                /*
               {
                    !isDead && isSleeping = Only Sleeping  //   !isDead && !isSleeping  = Only Alive
                    if (!isTribeDinoOrPlayer) = Only alive enemy player
                    Actor->IsPlayer() = player  !Actor->IsPlayer() = Dino
                    pas active = !
               }*/

                if (Settings.Visuals.Radar2DSelf)
                {
                    // Cacher mon Player
                    if (Actor == Cache.LocalActor && Settings.Visuals.HideSelf) continue;
                }

                //Mon player
                if (Actor->IsPlayer() && !Actor->IsDead() && Actor->IsConscious() && Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                {
                    FVector ActorLocation = Actor->K2_GetActorLocation();
                    FVector relativeLocation = ActorLocation - LocalPawnLocation;

                    // Calculer l'angle entre le joueur et le joueur local
                    float angle = std::atan2(relativeLocation.Y, relativeLocation.X) - Cache.LPC->PlayerCameraManager->GetCameraRotation().Yaw * M_PI / 180.0f;
                    while (angle < 0.0f) angle += 2 * M_PI;
                    while (angle > 2 * M_PI) angle -= 2 * M_PI;
                    if (angle >= M_PI) angle -= M_PI;
                    else angle += M_PI;

                    // Calculer la distance entre le joueur et le joueur local
                    float distance = relativeLocation.Size() / 33000.0f;

                    // Calculer la position d'écran du joueur sur le radar
                    ImVec2 radarPos(center.x + distance * radarWidth / 2 * std::sin(-angle), center.y + distance * radarHeight / 2 * std::cos(-angle));

                    // Dessinez le joueur sur le radar
                    ImVec2 dotPosition(radarPos.x, radarPos.y);
                    if (dotPosition.x >= cursorPos.x && dotPosition.x <= cursorPos.x + radarWidth && dotPosition.y >= cursorPos.y && dotPosition.y <= cursorPos.y + radarHeight)
                    {
                        WindowDrawList->AddCircleFilled(dotPosition, playerRadius, playerColorALLY);
                    }
                }

                if (Settings.Visuals.Radar2DDead)
                {
                    //Enemy Dead
                    if (Actor->IsPlayer() && Actor->IsDead() && !Actor->IsConscious() && Actor != Cache.LocalActor)
                    {

                        if (!isTribeDinoOrPlayer)
                        {
                            // Calculate the angle between the player and the local player
                            float angle = std::atan2(relativeLocation.Y, relativeLocation.X) - Cache.LPC->PlayerCameraManager->GetCameraRotation().Yaw * M_PI / 180.0f;
                            while (angle < 0.0f) angle += 2 * M_PI;
                            while (angle > 2 * M_PI) angle -= 2 * M_PI;
                            if (angle >= M_PI) angle -= M_PI;
                            else angle += M_PI;

                            // Calculate the distance between the player and the local player
                            float distance = relativeLocation.Size() / 33000.0f;

                            // Calculate the screen position of the player on the radar
                            ImVec2 radarPos(center.x + distance * radarWidth / 2 * std::sin(-angle), center.y + distance * radarHeight / 2 * std::cos(-angle));

                            // Draw the player on the radar
                            ImVec2 dotPosition(radarPos.x, radarPos.y);
                            if (dotPosition.x >= cursorPos.x && dotPosition.x <= cursorPos.x + radarWidth && dotPosition.y >= cursorPos.y && dotPosition.y <= cursorPos.y + radarHeight)
                            {
                                WindowDrawList->AddCircleFilled(dotPosition, playerRadius, playerColorDENEMY);
                            }
                        }
                    }
                }

                if (Settings.Visuals.Radar2DSleeper)
                {
                    //Enemy sleeper
                    if (Actor->IsPlayer() && !Actor->IsDead() && !Actor->IsConscious() && Actor != Cache.LocalActor)
                    {
                        FVector ActorLocation = Actor->K2_GetActorLocation();
                        FVector relativeLocation = ActorLocation - LocalPawnLocation;

                        // Calculate the angle between the player and the local player
                        float angle = std::atan2(relativeLocation.Y, relativeLocation.X) - Cache.LPC->PlayerCameraManager->GetCameraRotation().Yaw * M_PI / 180.0f;
                        while (angle < 0.0f) angle += 2 * M_PI;
                        while (angle > 2 * M_PI) angle -= 2 * M_PI;
                        if (angle >= M_PI) angle -= M_PI;
                        else angle += M_PI;

                        // Calculate the distance between the player and the local player
                        float distance = relativeLocation.Size() / 33000.0f;

                        // Calculate the screen position of the player on the radar
                        ImVec2 radarPos(center.x + distance * radarWidth / 2 * std::sin(-angle), center.y + distance * radarHeight / 2 * std::cos(-angle));

                        // Draw the player on the radar
                        ImVec2 dotPosition(radarPos.x, radarPos.y);
                        if (dotPosition.x >= cursorPos.x && dotPosition.x <= cursorPos.x + radarWidth && dotPosition.y >= cursorPos.y && dotPosition.y <= cursorPos.y + radarHeight)
                        {
                            WindowDrawList->AddCircleFilled(dotPosition, playerRadius, playerColorSENEMY);
                        }
                    }
                }

                if (Settings.Visuals.Radar2DASleeper)
                {
                    //Tribe Sleeper
                    if (Actor->IsPlayer() && !Actor->IsDead() && !Actor->IsConscious() && Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                    {
                        FVector ActorLocation = Actor->K2_GetActorLocation();
                        FVector relativeLocation = ActorLocation - LocalPawnLocation;

                        // Calculate the angle between the player and the local player
                        float angle = std::atan2(relativeLocation.Y, relativeLocation.X) - Cache.LPC->PlayerCameraManager->GetCameraRotation().Yaw * M_PI / 180.0f;
                        while (angle < 0.0f) angle += 2 * M_PI;
                        while (angle > 2 * M_PI) angle -= 2 * M_PI;
                        if (angle >= M_PI) angle -= M_PI;
                        else angle += M_PI;

                        // Calculate the distance between the player and the local player
                        float distance = relativeLocation.Size() / 33000.0f;

                        // Calculate the screen position of the player on the radar
                        ImVec2 radarPos(center.x + distance * radarWidth / 2 * std::sin(-angle), center.y + distance * radarHeight / 2 * std::cos(-angle));

                        // Draw the player on the radar
                        ImVec2 dotPosition(radarPos.x, radarPos.y);
                        if (dotPosition.x >= cursorPos.x && dotPosition.x <= cursorPos.x + radarWidth && dotPosition.y >= cursorPos.y && dotPosition.y <= cursorPos.y + radarHeight)
                        {
                            WindowDrawList->AddCircleFilled(dotPosition, playerRadius, playerColorSALLY);
                        }
                    }
                }

                if (Settings.Visuals.Radar2DAlly)
                {
                    //Player Ally
                    if (Actor->IsPlayer() && Actor->IsConscious() && !Actor->IsDead() && Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                    {
                        FVector ActorLocation = Actor->K2_GetActorLocation();
                        FVector relativeLocation = ActorLocation - LocalPawnLocation;

                        // Calculate the angle between the player and the local player
                        float angle = std::atan2(relativeLocation.Y, relativeLocation.X) - Cache.LPC->PlayerCameraManager->GetCameraRotation().Yaw * M_PI / 180.0f;
                        while (angle < 0.0f) angle += 2 * M_PI;
                        while (angle > 2 * M_PI) angle -= 2 * M_PI;
                        if (angle >= M_PI) angle -= M_PI;
                        else angle += M_PI;

                        // Calculate the distance between the player and the local player
                        float distance = relativeLocation.Size() / 33000.0f;

                        // Calculate the screen position of the player on the radar
                        ImVec2 radarPos(center.x + distance * radarWidth / 2 * std::sin(-angle), center.y + distance * radarHeight / 2 * std::cos(-angle));

                        // Draw the player on the radar
                        ImVec2 dotPosition(radarPos.x, radarPos.y);
                        if (dotPosition.x >= cursorPos.x && dotPosition.x <= cursorPos.x + radarWidth && dotPosition.y >= cursorPos.y && dotPosition.y <= cursorPos.y + radarHeight)
                        {
                            WindowDrawList->AddCircleFilled(dotPosition, playerRadius, playerColorALLY);
                        }
                    }
                }


                if (Settings.Visuals.Radar2DEnemy)
                {
                    //Enemy player
                    if (Actor->IsPlayer() && Actor->IsConscious() && !Actor->IsDead() && !Actor->IsPrimalCharFriendly((APrimalCharacter*)Cache.LocalActor))
                    {
                        if (!isTribeDinoOrPlayer) // only enemy
                        {
                            // Calculate the angle between the player and the local player
                            float angle = std::atan2(relativeLocation.Y, relativeLocation.X) - Cache.LPC->PlayerCameraManager->GetCameraRotation().Yaw * M_PI / 180.0f;
                            while (angle < 0.0f) angle += 2 * M_PI;
                            while (angle > 2 * M_PI) angle -= 2 * M_PI;
                            if (angle >= M_PI) angle -= M_PI;
                            else angle += M_PI;

                            // Calculate the distance between the player and the local player
                            float distance = relativeLocation.Size() / 33000.0f;

                            // Calculate the screen position of the player on the radar
                            ImVec2 radarPos(center.x + distance * radarWidth / 2 * std::sin(-angle), center.y + distance * radarHeight / 2 * std::cos(-angle));

                            // Draw the player on the radar
                            ImVec2 dotPosition(radarPos.x, radarPos.y);
                            if (dotPosition.x >= cursorPos.x && dotPosition.x <= cursorPos.x + radarWidth && dotPosition.y >= cursorPos.y && dotPosition.y <= cursorPos.y + radarHeight)
                            {
                                WindowDrawList->AddCircleFilled(dotPosition, playerRadius, playerColorENEMY);

                                if (Settings.Visuals.DrawLineRadar2D)
                                {
                                    // Dessiner une ligne entre moi et les autres joueurs
                                    ImVec2 localPlayerPosition(center.x, center.y); // Votre position au centre du radar

                                    // Check if target is visible
                                    bool isVisible = Cache.LPC->LineOfSightTo(Actor, Cache.LPC->PlayerCameraManager->GetCameraLocation(), false);
                                    const ImU32 lineColorVisible = IM_COL32(0, 255, 0, 255);
                                    const ImU32 lineColorNotVisible = IM_COL32(128, 128, 128, 255);
                                    ImU32 currentLineColor = isVisible ? lineColorVisible : lineColorNotVisible;

                                    WindowDrawList->AddLine(localPlayerPosition, dotPosition, currentLineColor); // Dessiner la ligne entre vous et l'autre joueur
                                }
                            }
                        }
                    }
                }

               // ImGui::End();
            }
        }
    }
}