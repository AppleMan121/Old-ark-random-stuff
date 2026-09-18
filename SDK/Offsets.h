#pragma once
#include <cstdint>

namespace Offsets {
    // ============================================
    // GLOBAL OFFSETS (from dumpspace dump)
    // ============================================
    constexpr uintptr_t GObjects = 0x4BA6578;
    constexpr uintptr_t ProcessEvent = 0x2342EB0;
    constexpr uintptr_t ProcessEventIndex = 0x3C;
    
    // NOTE: GWorld and GNames show as 0x0 in static dumps
    // These must be resolved via pattern scanning or RIP-relative addressing
    // Fallback patterns for initialization:
    // GWorld: 48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? 48 8B 01
    // GNames: 48 8B 05 ? ? ? ? 48 85 C0 75 ? 48 8B 0D ? ? ? ? 48 89 05

    // ============================================
    // UWORLD OFFSETS
    // ============================================
    namespace UWorld {
        constexpr uintptr_t PersistentLevel = 0xF8;
        constexpr uintptr_t GameState = 0x128;
        constexpr uintptr_t OwningGameInstance = 0x298;
    }

    // ============================================
    // AACTOR OFFSETS
    // ============================================
    namespace AActor {
        constexpr uintptr_t RootComponent = 0x250;  // CRITICAL FIX - was incorrect before
    }

    // ============================================
    // APRIMALCHARACTER OFFSETS
    // ============================================
    namespace APrimalCharacter {
        constexpr uintptr_t ReplicatedCurrentHealth = 0x94C;
        constexpr uintptr_t ReplicatedMaxHealth = 0x950;
        constexpr uintptr_t ReplicatedCurrentTorpor = 0x954;
        constexpr uintptr_t ReplicatedMaxTorpor = 0x958;
    }

    // ============================================
    // ASHOOTERWEAPON OFFSETS
    // ============================================
    namespace AShooterWeapon {
        constexpr uintptr_t CurrentAmmo = 0xAC0;
    }

    // ============================================
    // USKELETALMESHCOMPONENT OFFSETS
    // ============================================
    namespace USkeletalMeshComponent {
        constexpr uintptr_t BodySetup = 0xA50;
    }

    // ============================================
    // BONE INDICES (ARK Specific)
    // ============================================
    namespace Bones {
        // Male character bone indices
        namespace Male {
            constexpr int Head = 0x57;
            constexpr int Neck = 0x60;
            constexpr int UpperChest = 0x62;
            constexpr int LowerChest = 0x63;
            constexpr int Stomach = 0x64;
            constexpr int Pelvis = 0x65;
            constexpr int LeftShoulder = 0x66;
            constexpr int LeftUpperArm = 0x67;
            constexpr int LeftElbow = 0x68;
            constexpr int LeftHand = 0x69;
            constexpr int RightShoulder = 0x6A;
            constexpr int RightUpperArm = 0x6B;
            constexpr int RightElbow = 0x6C;
            constexpr int RightHand = 0x6D;
            constexpr int LeftThigh = 0x6E;
            constexpr int LeftKnee = 0x6F;
            constexpr int LeftFoot = 0x70;
            constexpr int RightThigh = 0x71;
            constexpr int RightKnee = 0x72;
            constexpr int RightFoot = 0x73;
        }

        // Female character bone indices (slightly different)
        namespace Female {
            constexpr int Head = 0x59;
            constexpr int Neck = 0x62;
            constexpr int UpperChest = 0x64;
            constexpr int LowerChest = 0x65;
            constexpr int Stomach = 0x66;
            constexpr int Pelvis = 0x67;
            constexpr int LeftShoulder = 0x68;
            constexpr int LeftUpperArm = 0x69;
            constexpr int LeftElbow = 0x6A;
            constexpr int LeftHand = 0x6B;
            constexpr int RightShoulder = 0x6C;
            constexpr int RightUpperArm = 0x6D;
            constexpr int RightElbow = 0x6E;
            constexpr int RightHand = 0x6F;
            constexpr int LeftThigh = 0x70;
            constexpr int LeftKnee = 0x71;
            constexpr int LeftFoot = 0x72;
            constexpr int RightThigh = 0x73;
            constexpr int RightKnee = 0x74;
            constexpr int RightFoot = 0x75;
        }
    }
}
