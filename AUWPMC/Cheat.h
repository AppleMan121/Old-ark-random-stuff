#pragma once
#include "MemHelper.h"
#include <TlHelp32.h>
#include <iostream>
#include <stdio.h>
#include "SDK/SDK.h"
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <Windows.h>
#include <vector>

struct
{
	HWND GameWindow;
	WNDPROC WndProcOriginal = nullptr;
	decltype(SetCursor)* SetCursorOriginal = nullptr;
	decltype(SetCursorPos)* SetCursorPosOriginal = nullptr;
	IDXGISwapChain* SwapChain;
	ID3D11Device* Device;
	ID3D11DeviceContext* Ctx;
	D3D_PRESENT_FUNCTION PresentFunc;
	HRESULT(*ResizeOriginal)(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags) = nullptr;
	HRESULT(*OriginalPresent)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags) = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
} D3D;

struct Utils
{
public:
	static inline void PE_HOOK(void* obj, UFunction* fn, void* params);
	static inline FVector* GetAdjustedAim(AShooterWeapon* Weapon, FVector* Result);
	static inline void DoFireProjectile(uintptr_t APrimalWeaponBow, FVector Origin, FVector ShootDir);
};

struct Renderer
{
public:
	static HRESULT D3D_HOOK(IDXGISwapChain* SwapChain, UINT SyncInterval, UINT Flags);
	static bool CreateView();
	static LRESULT WINAPI WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL WINAPI SetCursorPosHook(int X, int Y);
	static HCURSOR WINAPI SetCursorHook(HCURSOR hCursor);
	static void HookInput();
	static void RemoveInput();
public:
	static inline bool Init();
	static inline bool Remove();

	struct Drawing
	{
		static inline void RenderText(ImVec2 ScreenPosition, ImColor Color, const char* Text, int widthText);
		static inline void RenderText2(ImVec2 ScreenPosition, ImColor Color, const char* Text, int widthText);
		static inline void RenderCrosshair(ImColor Color, int Thickness);
		static inline void RenderAimFOV(ImColor Color);
		static inline bool WithinAimFOV(int CircleX, int CircleY, int R, int X, int Y);
		static inline int ReturnDistance(int X1, int Y1, int X2, int Y2);
		static inline AActor* ClosestTarget(int AimBone);
		static inline bool RenderPlayerSkeleton(USkeletalMeshComponent* Mesh, int Gender, ImColor Color);
		void DrawRadarDot(const ImVec2& radarCenter, float radarRadius, const FVector& playerLocation, const FVector& actorLocation, ImColor color);
		static inline void RenderCollapseFriendlyDisplayList(ImVec2 ScreenStartPosition, ImColor Color, std::vector<std::string> DisplayArray, float HeightFactor);
		static inline bool HeadDot(USkeletalMeshComponent* Mesh, int Gender, ImColor Color);
		static inline bool BOX(USkeletalMeshComponent* Mesh, int Gender, ImColor Color);
		static inline bool HP(USkeletalMeshComponent* Mesh, int Gender, ImColor Color);
	};
};

struct
{      
	uintptr_t GameBase;
	float WindowSizeX;
	float WindowSizeY;
	UPrimalPlayerData* LPData;
	ULocalPlayer* LocalPlayer;
	AActor* LocalActor;
	APlayerController* LPC;
	FVector LocalLocation;
	FString LPTribeName;
	AActor* TargetActor;
	FVector TargetBoneLocation;
	std::vector<uintptr_t> CachedPlayers;
	FVector* (*OriginalGetAdjustedAim) (AShooterWeapon*, FVector*);
	AActor* AimbotTarget;
	float LPFOV;
	int LPTribeID;
	int NearbyEnemies;

	//AShooterCharacter* localPlayer = nullptr;

	//TArray<AActor*> primalChars{};

	struct
	{
		std::vector<uintptr_t> FlakHelmets;
		std::vector<uintptr_t> FlakChestplates;
		std::vector<uintptr_t> FlakGloves;
		std::vector<uintptr_t> FlakPants;
		std::vector<uintptr_t> FlakBoots;
		std::vector<uintptr_t> TekHelmets;
		std::vector<uintptr_t> TekChestplates;
		std::vector<uintptr_t> TekGloves;
		std::vector<uintptr_t> TekPants;
		std::vector<uintptr_t> TekBoots;
		std::vector<uintptr_t> RiotHelmets;
		std::vector<uintptr_t> RiotChestplates;
		std::vector<uintptr_t> RiotGloves;
		std::vector<uintptr_t> RiotPants;
		std::vector<uintptr_t> RiotBoots;
		UPrimalItem* BestHelmet;
		UPrimalItem* BestChestplate;
		UPrimalItem* BestGloves;
		UPrimalItem* BestPants;
		UPrimalItem* BestBoots;
	} AutoArmor;
} Cache;

struct
{
	const char* Insert = "Insert";
	const char* Delete = "Delete";
	const char* RightClick = "RightMouseButton";
	const char* LeftClick = "LeftMouseButton";
	const char* MouseButton1 = "ThumbMouseButton";
	const char* MouseButton2 = "ThumbMouseButton2";
	const char* B = "B";
	const char* C = "C";
	const char* E = "E";
	const char* F = "F";
	const char* G = "G";
	const char* H = "H";
	const char* I = "I";
	const char* J = "J";
	const char* K = "K";
	const char* L = "L";
	const char* M = "M";
	const char* N = "N";
	const char* O = "O";
	const char* P = "P";
	const char* Q = "Q";
	const char* T = "T";
	const char* U = "U";
	const char* V = "V";
	const char* X = "X";
	const char* Y = "Y";
	const char* Z = "Z";
	const char* Up = "Up";
	const char* Down = "Down";
	const char* Left = "Left";
	const char* Right = "Right";
	const char* F1 = "F1";
	const char* F2 = "F2";

	//Insert, Delete, RightClick, LeftClick, MouseButton1, MouseButton2, B, C, E, F, G, H, I, J, K, L, M, N, O, P , Q, T, U, V, X, Y, Z, Up, Down, Left, Right, F1, F2
} FKeyNames;

class ENUMS
{
public:
	enum EInputEvent
	{
		IE_Pressed = 0x0,
		IE_Released = 0x1,
		IE_Repeat = 0x2,
		IE_DoubleClick = 0x3,
		IE_Axis = 0x4,
		IE_MAX = 0x5,
	};
	enum HealthDisplayType
	{
		HealthText,
		HealthBar
	};
	enum AimBone
	{
		Head,
		Spine,
		Pelvis,
		Hand,
		Leg,
		FLeg
	};
	enum AimBones
	{
		HeadB = 8,
		SpineB = 4,
		PelvisB = 1,
		HandB = 38,
		LegB = 82,
		FLegB = 84
	};
	enum ECollisionChannel
	{
		ECC_WorldStatic = 0x0,
		ECC_WorldDynamic = 0x1,
		ECC_Visibility = 0x2,
	};
};

enum ECollisionChannel
{
	Visibility = 0x2,
};

struct //DataStruct
{
	HMODULE hModule;
	static inline bool IsMenuOpen = false;
	float MenuSizeX = 1000;
	float MenuSizeY = 540;
	void(*OriginalPostRender)(void* _this, void* canvas);
	void(*OriginalPE)(void* obj, UFunction* fn, void* params);
	FVector* (*OriginalGetBoneLocation)(USkeletalMeshComponent* Mesh, void* Params);
	FVector* (*GetBoneLocation)(USkeletalMeshComponent* Mesh, FVector* Result, FName BoneName, int Space);
	FVector* (*WeaponTraces)(AShooterWeapon* PoopShitRock, FHitResult* Result, FVector* StartTrace, FVector* EndTrace);
	bool(*OriginalInputKey)(APlayerController* PC, void* Params);
	bool(*InputKey)(APlayerController* PC, FKey Key, ENUMS::EInputEvent EventType, float AmountDepressed, bool bGamepad);
	bool(*OriginalActorLineTraceSingle)(AActor* Actor, void* Params);
	bool (*ActorLineTraceSingle)(AActor* Actor, FHitResult& OutHit, const FVector Start, const FVector End, ENUMS::ECollisionChannel CollisionChannel, FCollisionQueryParams Params);
	//void (*OriginalDoFireProjectile) (uintptr_t APrimalWeaponBow, FVector Origin, FVector ShootDir);


	std::string dllPath = std::string();
	std::string dllPath2 = std::string();
	std::string dllPath3 = std::string();
	std::string dllPath4 = std::string();
	std::string dllPath5 = std::string();

	struct
	{
		const char* ToggleMenuKey = FKeyNames.Delete;
		const char* AimbotKey = FKeyNames.RightClick;
		const char* KeyBindHack = FKeyNames.LeftClick;
		const char* TekPunchKey = FKeyNames.G;
	} Keybinds;
	struct
	{
		bool DrawWildCreatures = false;
		bool DrawTamedCreatures = false;
		bool DrawPlayers = true;
		bool DrawTurrets = true;
		bool DrawTurretsDistance = true;
		bool DrawTurrets3 = true;    // Turret Name
		bool DrawContainers = false;
		bool ContainerDistance = false;
		//HideDistance
		bool DrawStructure = false;
		bool DrawAimFOV = false;
		float FOVSize = 200.f;
		bool DrawCrosshair = true;
		float CrosshairWidth = 0;
		float CrosshairSize = 50;
		bool DrawPlayerHP = true;
		bool RenderPlayerName = true;
		bool HideSelf = true;
		bool DrawDeadPlayers = true;
		bool DrawSleepingPlayers = true;
		bool DrawPlayerDistance = true;
		bool DrawPlayerBones = true;
		bool BOX = true;
		bool HeadDot = false;

		//Line
		bool DrawLine = true;
		bool TopScreen = false;
		bool DEnemy = false;
		bool SEnemy = false;
		bool ASleeper = false;
		bool Ally = false;
		bool Enemy = true;

		// AIM LINE
		bool DrawAimingLine = true;
		//DrawAimingLine

		//Radar
		bool Radar2D = true;
		bool Radar2DSelf = false;
		bool Radar2DDead = false;
		bool Radar2DSleeper = true;
		bool Radar2DASleeper = true;
		bool Radar2DAlly = true;
		bool Radar2DEnemy = true;
		bool DrawLineRadar2D = true;

		bool HideSelf2 = true;
		bool HP = true;
		bool DrawLocalPlayer = true;
		bool ShowBulletCount = true;
		bool ShowEmptyTurrets = false;
		bool ShowEmptyContainers = false;
		bool ShowTribe = false;
		bool ShowHealth = true;
		bool HideFish = true;
		bool WildDinoTorp = false;
		bool TamedDinoTorp = false;
		bool PlayerTorp = false;
		bool PlayerTorp2 = false;
		bool WildDinoFilter = false;
		bool isBee = true;
		bool AlphaFilter = false;
		bool DinoDead = false;
		bool HideTeamDinos = true;
		bool ContainerFilter = false;
		bool IdkMan = true;
		bool HideTeamPlayers = false;
		bool DrawSleepingDinos = false;
		bool DrawDinoDistance = false;
		bool HideTeamTurrets = true;
		bool HideTeamContainers = true;
		bool ShowWeapons = true;
	} Visuals;
	struct
	{
		bool EnableAimbot = true;
		bool RocketCheat = false;
		bool AimLocked = false;
		bool TargetTribe = false;
		bool TargetSleepers = true;
		bool VisibleOnly = false;
		bool SilentAim = false;
		ENUMS::AimBone AimBone;
		struct
		{
			FVector Location;
			float Smoothness = 1.f;
		} Target;
	} Aimbot;
	struct
	{
		bool EnableTriggerbot = false;
		bool PlayersOnly = false;
	} Triggerbot;
	struct
	{
		//TEST
		bool NoSway = true;
		bool NoSpread = true;
		bool NoShake = true;
		bool InfiniteOrbit = true;
		bool InstantDinoTurn = true;
		bool RhinoCharge = false;
		bool RideDist = false;
		bool AutoArmor = false;
		float AutoArmorPercent = 0.15f;
		bool ExtraInfo = true;
		bool ShowFPS = true;
		bool ShowPlayers = true;
		bool NumTamedDinos = true;
		bool LimitTurretsNum = true;
		bool NoglinAlert = false;
		bool LongArms = false;
		bool ShowXYZ = false;
		bool SpeedHacks = false;
		bool InfiniteArms = false;
		float NewSpeed = 1.f;
		float NewSpeed2 = 1.f;
		float NewSpeed3 = 1.f;
		float NewSpeed4 = 1.f;
		float Ride = 1.f;
		bool RapidFire = false;

		wchar_t* wantedGT;


		bool createChar = false;
		wchar_t* charName = (wchar_t*)L"123";
		float bodyColor[4] = { 1.f, 0.f, 0.f, 1.f };
		bool rocketTurret = false;
		bool explorernotehack = false;
		bool TekPunch = false;
	} Misc;

	struct
	{
		struct
		{
			int R = 0;
			int G = 0;
			int B = 255;
		} FOVCircle;
		struct
		{
			int R = 255;
			int G = 0;
			int B = 0;
		} Crosshair;
		struct
		{
			int R = 255;
			int G = 140;
			int B = 0;
		} EnemyCountWarning;
		struct
		{
			int R = 255;
			int G = 0;
			int B = 157;
		} WildDino;
		struct
		{
			int R = 0;
			int G = 255;
			int B = 0;
		} TribeDinoPlayer;
		struct
		{
			int R = 0;
			int G = 255;
			int B = 255;
		} AlliancedDino;
		struct
		{
			int R = 255;
			int G = 0;
			int B = 0;
		} EnemyDino;
		struct
		{
			int R = 0;
			int G = 255;
			int B = 0;
		} AlliancedPlayers;

		struct
		{
			int R = 255;
			int G = 0;
			int B = 0;
		} EnemyPlayers;
		struct
		{
			int R = 255;
			int G = 255;
			int B = 0;
		} SleepingPlayers;
		struct
		{
			int R = 255;
			int G = 255;
			int B = 255;
		} Corpses;
		struct
		{
			int R = 255;
			int G = 255;
			int B = 0;
		} Containers;
		struct
		{
			int R = 255;
			int G = 0;
			int B = 0;
		} Turrets;
	} Colors;
} Settings;


void RenderMenu();
void radar2D();
void DrawAimingLine();
void DrawLine();
void MireLine();
//threads
void AimbotThread();
void RocketCheat();