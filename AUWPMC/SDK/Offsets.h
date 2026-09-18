#pragma once
#include <cstdint>

// ==========================================
// GLOBAL OFFSETS (from dumpspace.spuckwaffel.com)
// Game: ARK Survival Evolved
// Dumper-7 Version
// ==========================================

namespace Offsets {
    // Global pointers
    constexpr uintptr_t GObjects = 0x4BA6578;      // OFFSET_GOBJECTS
    constexpr uintptr_t ProcessEvent = 0x2342EB0; // OFFSET_PROCESSEVENT
    constexpr uint32_t ProcessEventIndex = 0x3C;  // INDEX_PROCESSEVENT
    
    // NOTE: GWorld and GNames are 0x0 in dump - must use pattern scanning
    // Use these patterns to find them:
    // GWorld: "48 8B 05 ? ? ? ? 48 85 C0 74 14" (mov rax, cs:GWorld)
    // GNames: "48 8D 0D ? ? ? ? E8 ? ? ? ? C6 05" (lea rcx, GNames)
}

// ==========================================
// CLASS OFFSETS
// ==========================================

namespace UWorldOffsets {
    constexpr auto PersistentLevel = 0xF8;           // ULevel*
    constexpr auto SaveGameSummary = 0x100;           // USaveGameSummary*
    constexpr auto NetDriver = 0x108;               // UNetDriver*
    constexpr auto GameState = 0x128;               // AGameState*
    constexpr auto NetworkManager = 0x130;          // AGameNetworkManager*
    constexpr auto AuthorityGameMode = 0x250;        // AGameMode*
    constexpr auto NavigationSystem = 0x258;        // UNavigationSystem*
    constexpr auto AISystem = 0x260;                // UAISystemBase*
    constexpr auto Levels = 0x270;                  // TArray<ULevel*>
    constexpr auto CurrentLevel = 0x290;            // ULevel*
    constexpr auto OwningGameInstance = 0x298;      // UGameInstance*
}

namespace AActorOffsets {
    constexpr auto PrimaryActorTick = 0x28;         // FActorTickFunction
    constexpr auto Owner = 0x98;                    // AActor*
    constexpr auto Role = 0x130;                   // ENetRole
    constexpr auto CustomTag = 0x1A0;              // FName
    constexpr auto CustomData = 0x1A8;             // int32
    constexpr auto InputComponent = 0x1D8;          // UInputComponent*
    constexpr auto TargetingTeam = 0x218;           // int32
    constexpr auto Instigator = 0x220;             // APawn*
    constexpr auto RootComponent = 0x250;           // USceneComponent* (CRITICAL - was wrong before)
    constexpr auto Children = 0x238;                // TArray<AActor*>
    constexpr auto Tags = 0x298;                   // TArray<FName>
}

namespace ULevelOffsets {
    constexpr auto OwningWorld = 0xB8;            // UWorld*
    constexpr auto Model = 0xC0;                   // UModel*
    constexpr auto LevelScriptActor = 0xD8;         // ALevelScriptActor*
    constexpr auto NavListStart = 0xE0;            // ANavigationObjectBase*
    constexpr auto NavListEnd = 0xE8;              // ANavigationObjectBase*
    constexpr auto bIsVisible = 0x2E0;             // uint8
    constexpr auto bLocked = 0x2E0;                // uint8 (same byte, different bit)
    constexpr auto MovieSceneBindingsArray = 0x310; // TArray<UObject*>
    constexpr auto ActiveRuntimeMovieScenePlayers = 0x320; // TArray<UObject*>
    constexpr auto AssetUserData = 0x330;          // TArray<UAssetUserData*>
}

// ==========================================
// UPDATED PATTERN SCANS (if needed)
// ==========================================

namespace Patterns {
    // Use these if the hardcoded offsets don't work
    // GWorld pattern - works on most ARK versions
    constexpr const char* GWorld = "48 8B 05 ? ? ? ? 48 85 C0 74 14";
    
    // GetBoneLocation pattern
    constexpr const char* GetBoneLocation = "40 57 48 83 EC 70 48 C7 44 24 ? ? ? ? ? 48 89 9C 24 ? ? ? ? 48 8B DA 48 8B F9 49 8B D0 E8 ? ? ? ? 83 F8 FF";
    
    // ProcessEvent pattern (backup)
    constexpr const char* ProcessEvent = "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ?";
    
    // GetActorBounds pattern
    constexpr const char* GetActorBounds = "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 48 8D 68 A1 48 81 EC ? ? ? ?";
}
